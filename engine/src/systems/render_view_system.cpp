#include "render_view_system.hpp"
#include "memory/linear_allocator.hpp"
#include "containers/hashtable.hpp"
#include "renderer/renderer.hpp"

// ЗАДАЧА: временно - создайте фабрику и зарегистрируйтесь.
#include "renderer/views/render_view_world.hpp"
#include "renderer/views/render_view_ui.hpp"
#include "renderer/views/render_view_skybox.hpp"

RenderViewSystem* RenderViewSystem::state = nullptr;

constexpr RenderViewSystem::RenderViewSystem(u16 MaxViewCount, u16* TableBlock, RenderView** RegisteredViews)
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
    u64 ArrayRequirement = sizeof(RenderView*) * MaxViewCount;
    u64 MemoryRequirement = ClassRequirement + HashtableRequirement + ArrayRequirement;

    u8* RVSPointer = reinterpret_cast<u8*>(LinearAllocator::Instance().Allocate(MemoryRequirement));

    state = new(RVSPointer) RenderViewSystem(MaxViewCount, reinterpret_cast<u16*>(RVSPointer + ClassRequirement), reinterpret_cast<RenderView**>(RVSPointer + ClassRequirement + HashtableRequirement));

    if (!state) {
        return false;
    }
    return true;
}

void RenderViewSystem::Shutdown()
{
    state = nullptr;
}

bool RenderViewSystem::Create(RenderView::Config &config)
{
    if (config.PassCount < 1) {
        MERROR("RenderViewSystem::Create - Конфигурация должна иметь хотя бы один проход рендеринга.");
        return false;
    }

    if (!config.name){
        MERROR("RenderViewSystem::Create: имя обязательно");
        return false;
    }

    u16 id = INVALID::U16ID;
    // Убедитесь, что запись с таким именем еще не зарегистрирована.
    lookup.Get(config.name.c_str(), &id);
    if (id != INVALID::U16ID) {
        MERROR("RenderViewSystem::Create - Вид с именем '%s' уже существует. Новый не будет создан.", config.name.c_str());
        return false;
    }

    // Найдите новый идентификатор.
    for (u32 i = 0; i < MaxViewCount; ++i) {
        if (RegisteredViews[i] == nullptr) {
            id = i;
            break;
        }
    }

    // Убедитесь, что найдена допустимая запись.
    if (id == INVALID::U16ID) {
        MERROR("RenderViewSystem::Create - Нет доступного места для нового представления. Измените конфигурацию системы, чтобы учесть больше.");
        return false;
    }
    // ЗАДАЧА: Шаблон фабрика
    switch (config.type)
    {
    case RenderView::KnownTypeWorld:
        if (!(RegisteredViews[id] = new RenderViewWorld(id, config.name, config.type, config.PassCount, config.CustomShaderName))) {
            MERROR("Не удалось создать представление.");
        }
        break;
    case RenderView::KnownTypeUI:
        if (!(RegisteredViews[id] = new RenderViewUI(id, config.name, config.type, config.PassCount, config.CustomShaderName))) {
            MERROR("Не удалось создать представление.");
        }
        break;
    case RenderView::KnownTypeSkybox:
        if (!(RegisteredViews[id] = new RenderViewSkybox(id, config.name, config.type, config.PassCount, config.CustomShaderName))) {
            MERROR("Не удалось создать представление.");
        }
        break;
    default:
        break;
    }
    RenderView* view = RegisteredViews[id];

    for (u32 i = 0; i < view->RenderpassCount; ++i) {
        view->passes[i] = Renderer::GetRenderpass(config.passes[i].name);
        if (!view->passes[i]) {
            MFATAL("RenderViewSystem::Create - renderpass: '%s' не найден.", config.passes[i].name);
            return false;
        }
    }

    // Обновите запись хэш-таблицы.
    lookup.Set(view->name.c_str(), id);

    return true;
}

void RenderViewSystem::OnWindowResize(u32 width, u32 height)
{
    // Отправить всем видам
    for (u32 i = 0; i < MaxViewCount; ++i) {
        if (RegisteredViews[i] != nullptr && RegisteredViews[i]->id != INVALID::U16ID) {
            RegisteredViews[i]->Resize(width, height);
        }
    }
}

RenderView *RenderViewSystem::Get(const char *name)
{
    if (state) {
        u16 id = INVALID::U16ID;
        lookup.Get(name, &id);
        if (id != INVALID::U16ID) {
            return RegisteredViews[id];
        }
    }
    return nullptr;
}

bool RenderViewSystem::BuildPacket(const RenderView *view, void *data, RenderView::Packet &OutPacket)
{
    if (view) {
        return view->BuildPacket(data, OutPacket);
    }

    MERROR("RenderViewSystem::BuildPacket требует действительных указателей на представление и пакет.");
    return false;
}

bool RenderViewSystem::OnRender(const RenderView *view, const RenderView::Packet &packet, u64 FrameNumber, u64 RenderTargetIndex)
{
    if (view) {
        return view->Render(packet, FrameNumber, RenderTargetIndex);
    }

    MERROR("RenderViewSystem::Render требует действительный указатель на данные.");
    return false;
}
