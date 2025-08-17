#include "render_view_system.h"
#include "memory/linear_allocator.hpp"
#include "containers/hashtable.hpp"
#include "renderer/rendering_system.h"
#include "renderer/renderpass.h"

//#include <new>

struct render_view_system
{
    HashTable<u16> lookup;
    void* TableBlock;
    u32 MaxViewCount;
    RenderView** RegisteredViews; // Массив указателей на представления, принадлежащих приложению.

    constexpr render_view_system(u16 MaxViewCount, u16* TableBlock, RenderView** RegisteredViews) 
    : lookup(MaxViewCount, false, TableBlock, true, INVALID::U16ID),
      TableBlock(TableBlock),
      MaxViewCount(MaxViewCount),
      RegisteredViews(RegisteredViews) {}
};

static render_view_system* pState = nullptr;

bool RenderViewSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<RenderViewSystemConfig*>(config);
    if (pConfig->MaxViewCount == 0) {
        MFATAL("RenderViewSystem::Initialize - MaxViewCount должен быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок для хеш-таблицы, затем блок для массива.
    u64 ClassRequirement = sizeof(render_view_system);
    u64 HashtableRequirement = sizeof(u16) * pConfig->MaxViewCount;
    u64 ArrayRequirement = sizeof(RenderView*) * pConfig->MaxViewCount;
    MemoryRequirement = ClassRequirement + HashtableRequirement + ArrayRequirement;

    if (!memory) {
        return true;
    }

    u8* RVSPointer = reinterpret_cast<u8*>(memory);

    pState = new(RVSPointer) 
    render_view_system(
        pConfig->MaxViewCount, 
        reinterpret_cast<u16*>(RVSPointer + ClassRequirement), 
        reinterpret_cast<RenderView**>(RVSPointer + ClassRequirement + HashtableRequirement)
    );

    return true;
}

void RenderViewSystem::Shutdown()
{
    for (u32 i = 0; i < pState->MaxViewCount; i++) {
        auto view = pState->RegisteredViews[i];
        if (view) {
            delete view;
        }
    }
}

bool RenderViewSystem::Register(RenderView *view)
{
    if (!view) {
        MERROR("RenderViewSystem::Register требует указатель на допустимое представление.");
        return false;
    }
    
    if (view->RenderpassCount < 1) {
        MERROR("RenderViewSystem::Register: Конфигурация должна иметь хотя бы один проход отрисовки.");
        return false;
    }

    if (!view->name || MString::Length(view->name) < 1){
        MERROR("RenderViewSystem::Register: имя обязательно");
        return false;
    }

    u16 id = INVALID::U16ID;
    // Убедитесь, что запись с таким именем еще не зарегистрирована.
    pState->lookup.Get(view->name, &id);
    if (id != INVALID::U16ID) {
        MERROR("RenderViewSystem::Register: Вид с именем '%s' уже существует. Новый не будет создан.", view->name);
        return false;
    }

    // Найдите новый идентификатор.
    for (u32 i = 0; i < pState->MaxViewCount; ++i) {
        if (pState->RegisteredViews[i] == nullptr) {
            id = i;
            break;
        }
    }

    // Убедитесь, что найдена допустимая запись.
    if (id == INVALID::U16ID) {
        MERROR("RenderViewSystem::Register: Нет доступного места для нового представления. Измените конфигурацию системы, чтобы учесть больше.");
        return false;
    }

    // Обновить запись хеш-таблицы.
    pState->lookup.Set(view->name, id);

    // Установить указатель элемента массива.
    pState->RegisteredViews[id] = view;

    RegenerateRenderTargets(view);

    return true;
}

void RenderViewSystem::OnWindowResize(u32 width, u32 height)
{
    // Отправить всем видам
    for (u32 i = 0; i < pState->MaxViewCount; ++i) {
        if (pState->RegisteredViews[i]) {
            pState->RegisteredViews[i]->Resize(width, height);
        }
    }
}

RenderView *RenderViewSystem::Get(const char *name)
{
    if (pState) {
        u16 id = INVALID::U16ID;
        pState->lookup.Get(name, &id);
        if (id != INVALID::U16ID) {
            return pState->RegisteredViews[id];
        }
    }
    return nullptr;
}

bool RenderViewSystem::BuildPacket(RenderView *view, class LinearAllocator& FrameAllocator, void *data, RenderView::Packet &OutPacket)
{
    if (view) {
        return view->BuildPacket(FrameAllocator, data, OutPacket);
    }

    MERROR("RenderViewSystem::BuildPacket требует действительных указателей на представление и пакет.");
    return false;
}

bool RenderViewSystem::OnRender(RenderView *view, const RenderView::Packet &packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFramedata)
{
    if (view) {
        return view->Render(packet, FrameNumber, RenderTargetIndex, rFramedata);
    }

    MERROR("RenderViewSystem::Render требует действительный указатель на данные.");
    return false;
}

void RenderViewSystem::RegenerateRenderTargets(RenderView* view)
{
    // Создайте цели рендеринга для каждой. ЗАДАЧА: Должна быть настраиваемой.

    for (u64 r = 0; r < view->RenderpassCount; ++r) {
        auto& pass = view->passes[r];

        for (u8 i = 0; i < pass.RenderTargetCount; ++i) {
            auto& target = pass.targets[i];
            // Сначала уничтожьте старую, если она существует.
            // ЗАДАЧА: проверьте, действительно ли требуется изменение размера для этой цели.
            RenderingSystem::RenderTargetDestroy(target, false);

            for (u32 a = 0; a < target.AttachmentCount; ++a) {
                auto& attachment = target.attachments[a];
                if (attachment.source == RenderTargetAttachmentSource::Default) {
                    if (attachment.type == RenderTargetAttachmentType::Colour) {
                        attachment.texture = RenderingSystem::WindowAttachmentGet(i);
                    } else if (attachment.type == RenderTargetAttachmentType::Depth) {
                        attachment.texture = RenderingSystem::DepthAttachmentGet(i);
                    } else {
                        MFATAL("Неподдерживаемый тип вложения: 0x%x", attachment.type);
                        continue;
                    }
                } else if (attachment.source == RenderTargetAttachmentSource::View) {
                    if (!view->RegenerateAttachmentTarget()) {
                        continue;
                    } else {
                        if (!view->RegenerateAttachmentTarget(r, &attachment)) {
                            MERROR("Не удалось повторно создать целевой объект вложения для типа вложения: 0x%x", attachment.type);
                        }
                    }
                }
            }

            // Создайте цель рендеринга.
            RenderingSystem::RenderTargetCreate(
                target.AttachmentCount,
                target.attachments,
                &pass,
                // ПРИМЕЧАНИЕ: здесь мы просто отталкиваемся от размера первого вложения, но этого должно быть достаточно для большинства случаев.
                target.attachments[0].texture->width,
                target.attachments[0].texture->height,
                pass.targets[i]
            );
        }
    }
}
