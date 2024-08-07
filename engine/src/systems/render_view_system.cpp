#include "render_view_system.hpp"
#include "memory/linear_allocator.hpp"
#include "containers/hashtable.hpp"
#include "renderer/renderer.hpp"

// ЗАДАЧА: временно - создайте фабрику и зарегистрируйтесь.
#include "renderer/views/render_view_world.hpp"
#include "renderer/views/render_view_ui.hpp"

constexpr RenderViewSystem::RenderViewSystem(u16 MaxViewCount, void* TableBlock, RenderView* RegisteredViews)
:
lookup(MaxViewCount, false, TableBlock, true, INVALID::U16ID),
TableBlock(TableBlock),
MaxViewCount(MaxViewCount),
RegisteredViews(RegisteredViews) {}

bool RenderViewSystem::Initialize(u16 MaxViewCount)
{
    if (MaxViewCount == 0) {
        MFATAL("RenderViewSystem::Initialize - MaxViewCount должен быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок для хеш-таблицы, затем блок для массива.
    u64 ClassRequirement = sizeof(RenderViewSystem);
    u64 HashtableRequirement = sizeof(u16) * MaxViewCount;
    u64 ArrayRequirement = sizeof(RenderView) * MaxViewCount;
    u64 MemoryRequirement = ClassRequirement + HashtableRequirement + ArrayRequirement;

    u8* RVSPointer = reinterpret_cast<u8*>(LinearAllocator::Allocate(MemoryRequirement));

    state = new(RVSPointer) RenderViewSystem(MaxViewCount, RVSPointer + ClassRequirement, RVSPointer + ClassRequirement + HashtableRequirement);

    if (!state) {
        return false;
    }
    return true;
}

void RenderViewSystem::Shutdown()
{
    state = nullptr;
}

bool RenderViewSystem::Create(const RenderView::Config *config)
{
    if (!config) {
        MERROR("RenderViewSystem::Create требует указатель на допустимую конфигурацию.");
        return false;
    }

    if (config->PassCount < 1) {
        MERROR("RenderViewSystem::Create - Конфигурация должна иметь хотя бы один проход рендеринга.");
        return false;
    }

    u16 id = INVALID::U16ID;
    // Убедитесь, что запись с таким именем еще не зарегистрирована.
    lookup.Get(config->name, &id);
    if (id != INVALID::U16ID) {
        MERROR("RenderViewSystem::Create - Вид с именем '%s' уже существует. Новый не будет создан.", config->name);
        return false;
    }

    // Найдите новый идентификатор.
    for (u32 i = 0; i < MaxViewCount; ++i) {
        if (RegisteredViews[i].id == INVALID::U16ID) {
            id = i;
            break;
        }
    }

    // Убедитесь, что найдена допустимая запись.
    if (id == INVALID::U16ID) {
        MERROR("RenderViewSystem::Create - Нет доступного места для нового представления. Измените конфигурацию системы, чтобы учесть больше.");
        return false;
    }

    RenderView* view = &RegisteredViews[id];
    view->id = id;
    view->type = config->type;
    view->CustomShaderName = config->CustomShaderName;
    view->RenderpassCount = config->PassCount;
    view->passes = new Renderpass*[view->RenderpassCount];

    for (u32 i = 0; i < view->renderpass_count; ++i) {
        view->passes[i] = Renderer::GetRenderpass(config->passes[i].name);
        if (!view->passes[i]) {
            MFATAL("RenderViewSystem::Create - renderpass: '%s' не найден.", config->passes[i].name);
            return false;
        }
    }

    // TODO: Assign these function pointers to known functions based on the view type.
    // TODO: Factory pattern (with register, etc. for each type)?
    if (config->type == RENDERER_VIEW_KNOWN_TYPE_WORLD) {
        view->on_build_packet = render_view_world_on_build_packet;  // Для построения пакета
        view->on_render = render_view_world_on_render;              // Для рендеринга пакета
        view->on_create = render_view_world_on_create;
        view->on_destroy = render_view_world_on_destroy;
        view->on_resize = render_view_world_on_resize;
    } else if (config->type == RENDERER_VIEW_KNOWN_TYPE_UI) {
        view->on_build_packet = render_view_ui_on_build_packet;  // Для построения пакета
        view->on_render = render_view_ui_on_render;              // Для рендеринга пакета
        view->on_create = render_view_ui_on_create;
        view->on_destroy = render_view_ui_on_destroy;
        view->on_resize = render_view_ui_on_resize;
    }

    // Вызовите on create
    if (!view->on_create(view)) {
        MERROR("Не удалось создать представление.");
        kfree(view->passes, sizeof(renderpass*) * view->renderpass_count, MEMORY_TAG_ARRAY);
        kzero_memory(&state_ptr->registered_views[id], sizeof(render_view));
        return false;
    }

    // Обновите запись хэш-таблицы.
    hashtable_set(&state_ptr->lookup, config->name, &id);

    return true;
}