#include "render_view_system.hpp"
#include "memory/linear_allocator.hpp"
#include "containers/hashtable.hpp"
#include "renderer/rendering_system.hpp"

#include <new>

// ЗАДАЧА: временно - создайте фабрику и зарегистрируйтесь.
#include "renderer/views/render_view_world.h"
#include "renderer/views/render_view_ui.h"
#include "renderer/views/render_view_skybox.h"
#include "renderer/views/render_view_pick.h"

RenderViewSystem* RenderViewSystem::state = nullptr;

constexpr RenderViewSystem::RenderViewSystem(u16 MaxViewCount, u16* TableBlock, RenderView** RegisteredViews)
:
lookup(MaxViewCount, false, TableBlock, true, INVALID::U16ID),
TableBlock(TableBlock),
MaxViewCount(MaxViewCount),
RegisteredViews(RegisteredViews) {}

bool RenderViewSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<RenderViewSystemConfig*>(config);
    if (pConfig->MaxViewCount == 0) {
        MFATAL("RenderViewSystem::Initialize - MaxViewCount должен быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок для хеш-таблицы, затем блок для массива.
    u64 ClassRequirement = sizeof(RenderViewSystem);
    u64 HashtableRequirement = sizeof(u16) * pConfig->MaxViewCount;
    u64 ArrayRequirement = sizeof(RenderView*) * pConfig->MaxViewCount;
    MemoryRequirement = ClassRequirement + HashtableRequirement + ArrayRequirement;

    if (!memory) {
        return true;
    }

    u8* RVSPointer = reinterpret_cast<u8*>(memory);

    state = new(RVSPointer) RenderViewSystem(pConfig->MaxViewCount, reinterpret_cast<u16*>(RVSPointer + ClassRequirement), reinterpret_cast<RenderView**>(RVSPointer + ClassRequirement + HashtableRequirement));

    if (!state) {
        return false;
    }
    return true;
}

void RenderViewSystem::Shutdown()
{
    for (u32 i = 0; i < state->MaxViewCount; i++) {
        auto view = state->RegisteredViews[i];
        if (view) {
            delete view;
        }
    }
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
    state->lookup.Get(config.name, &id);
    if (id != INVALID::U16ID) {
        MERROR("RenderViewSystem::Create - Вид с именем '%s' уже существует. Новый не будет создан.", config.name);
        return false;
    }

    // Найдите новый идентификатор.
    for (u32 i = 0; i < state->MaxViewCount; ++i) {
        if (state->RegisteredViews[i] == nullptr) {
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
        if (!(state->RegisteredViews[id] = new RenderViewWorld(id, config))) {
            MERROR("Не удалось создать RenderViewWorld.");
        }
        break;
    case RenderView::KnownTypeUI:
        if (!(state->RegisteredViews[id] = new RenderViewUI(id, config))) {
            MERROR("Не удалось создать RenderViewUI.");
        }
        break;
    case RenderView::KnownTypeSkybox:
        if (!(state->RegisteredViews[id] = new RenderViewSkybox(id, config))) {
            MERROR("Не удалось создать RenderViewSkybox.");
        }
        break;
    case RenderView::KnownTypePick:
        if (!(state->RegisteredViews[id] = new RenderViewPick(id, config))) {
            MERROR("Не удалось создать RenderViewPick.")
        }
        break;
    default:
        break;
    }

    auto view = state->RegisteredViews[id];

    RegenerateRenderTargets(view);

    // Обновите запись хэш-таблицы.
    state->lookup.Set(view->name.c_str(), id);

    return true;
}

void RenderViewSystem::OnWindowResize(u32 width, u32 height)
{
    // Отправить всем видам
    for (u32 i = 0; i < state->MaxViewCount; ++i) {
        if (state->RegisteredViews[i] != nullptr && state->RegisteredViews[i]->id != INVALID::U16ID) {
            state->RegisteredViews[i]->Resize(width, height);
        }
    }
}

RenderView *RenderViewSystem::Get(const char *name)
{
    if (state) {
        u16 id = INVALID::U16ID;
        state->lookup.Get(name, &id);
        if (id != INVALID::U16ID) {
            return state->RegisteredViews[id];
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
