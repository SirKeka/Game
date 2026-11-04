#include "ui_system.h"
#include "core/event.h"
#include "core/systems_manager.h"
#include "math/geometry_utils.h"
#include "renderer/rendering_system.h"
#include "systems/texture_system.h"

bool UiElement::AddChild(UiElement *child)
{
    if (!child) {
        MERROR("Указатель на ребенка не действителен!");
        return false;
    }
    
    if (child->parent) {
        // Проверка добавлен ли уже данный ребенок
        if (child->parent == this) {
            return true;
        }

        if (!child->parent->RemoveChild(child)) {
            MERROR("Не удалось удалить дочерний элемент из родительского элемента перед сменой родительского элемента.");
            return false;
        }
    }

    children.PushBack(child);
    child->parent = this; // 
    child->xform.SetParent(&this->xform);

    return true;
}

bool UiElement::RemoveChild(UiElement *child)
{
    if (!child) {
        return false;
    }

    if (!children) {
        MERROR("Невозможно удалить дочерний элемент из родительского элемента, у которого нет дочерних элементов.");
        return false;
    }

    const u32 ChildCount = children.Length();
    for (u32 i = 0; i < ChildCount; ++i) {
        if (children[i] == child) {
            children.PopAt(i);

            child->xform.SetParent();
            return true;
        }
    }

    MERROR("Невозможно удалить дочерний элемент, который не является дочерним элементом заданного родителя.");
    return false;
}

void UiElement::SetParent(UiElement *parent)
{
    if (parent) {
        parent->AddChild(this);
    }
}

void UiElement::SetPosition(FVec3 position)
{
    xform.SetPosition(position);
}

FVec3 UiElement::GetPosition()
{
    return xform.GetPosition();
}

//static b8 standard_ui_system_move(u16 code, void* sender, void* listener_inst, event_context context) {
//    
//}

bool UiSystem::Initialize(u64 &MemoryRequirement, void *state, const Config &config)
{
    if (config.MaxControlCount == 0) {
        MFATAL("UiSystem::Initialize - config->MaxControlCount должен быть > 0.");
        return false;
    }

    // Раздел памяти: структура + массив активных элементов управления + массив неактивных элементов управления
    u64 StructRequirement = sizeof(UiSystem);
    u64 RegisteredArrayRequirement = sizeof(UiElement*) * config.MaxControlCount;
    // u64 ActiveInactiveArrayRequirement = sizeof(UiElement*) * config.MaxControlCount;
    MemoryRequirement = StructRequirement + RegisteredArrayRequirement;

    if (!state) {
        return true;
    }

    auto uisys = new (state) UiSystem(config);
    uisys->MaxControlCount = config.MaxControlCount;
    uisys->RegisteredControls = (UiElement**)((u8*)state + StructRequirement);
    // uisys->ActiveControls = (UiElement**)((u8*)uisys->Registeredontrlos + RegisteredArrayRequirement);
    // uisys->InactiveControls = &uisys->ActiveControls[config.MaxControlCount - 1]; // идет в обратную сторону

    Texture* atlas = TextureSystem::Acquire("StandardUIAtlas", true);
    if (!atlas) {
        MWARN("Не удалось загрузить текстуру атласа, используются настройки по умолчанию.");
        atlas = TextureSystem::GetDefaultTexture();
    }

    // Настроить текстурную карту.
    TextureMap* map = &uisys->UiAtlas;
    map->RepeatU = map->RepeatV = map->RepeatW = TextureRepeat::ClampToEdge;
    map->FilterMinify = map->FilterMagnify = TextureFilterMode::Nearest;
    map->texture = atlas;
    if (!SystemsManager::GetRenderingSystem()->TextureMapAcquireResources(map)) {
        MERROR("Не удалось получить ресурсы текстурной карты. Невозможно инициализировать StandardUI.");
        return false;
    }

    // Прослушивание событий ввода.
    EventSystem::Register(EventSystem::ButtonClicked,  state, Click);
    EventSystem::Register(EventSystem::MouseMoved,     state, Move);
    EventSystem::Register(EventSystem::ButtonPressed,  state, MouseDown);
    EventSystem::Register(EventSystem::ButtonReleased, state, MouseUp);

    MTRACE("Инициализация стандартной системы пользовательского интерфейса.");

    return true;
}

void UiSystem::Shutdown(void *self)
{
    auto uisys = (UiSystem*)self;
    EventSystem::Unregister(EventSystem::ButtonClicked,  self, Click);
    EventSystem::Unregister(EventSystem::MouseMoved,     self, Move);
    EventSystem::Unregister(EventSystem::ButtonPressed,  self, MouseDown);
    EventSystem::Unregister(EventSystem::ButtonReleased, self, MouseUp);

    // Выгрузить и уничтожить неактивные элементы управления.
    for (u32 i = 0; i < uisys->TotalControlCount; ++i) {
        auto control = uisys->RegisteredControls[i];
        control->Unload();
        control->Destroy();
    }
    // Выгрузить и уничтожить активные элементы управления.
    // for (u32 i = 0; i < rControls.ActiveControlCount; ++i) {
    //     auto control = rControls.ActiveControls[i];
    //     control->Unload();
    //     control->Destroy();
    // }

    if (uisys->UiAtlas.texture) {
        TextureSystem::Release(uisys->UiAtlas.texture->name);
        uisys->UiAtlas.texture = nullptr;
    }

    SystemsManager::GetRenderingSystem()->TextureMapReleaseResources(&uisys->UiAtlas);
}

void UiSystem::UpdateActive()
{
    auto swap = [](UiElement* a, UiElement* b) {
        UiElement* temp = a;
        a = b;
        b = temp;
    };

    ActiveControlCount = 0;

    UiElement* buf = nullptr;
    UiElement* NextControl = nullptr;
    for (u32 i = 0; i < TotalControlCount - 1; ++i) {
        UiElement* control = RegisteredControls[i];
        NextControl = RegisteredControls[i + 1];
        if (control->IsActive == false && control->IsActive != NextControl->IsActive) {
            swap(buf ? buf : control, NextControl);
            InactiveControlCount++;
        } else if (control->IsActive == false && control->IsActive == NextControl->IsActive) {
            buf = control;
        } else {
            ActiveControlCount++;
        }
    }
    
    if (NextControl->IsActive == true) {
        ActiveControlCount++;
    }
    
}

bool UiSystem::Update(void* self, FrameData &rFrameData)
{
    auto uisys = (UiSystem*)self;
    // Обновляем количество активных объектов

    if (uisys->IsDirty) {
        uisys->UpdateActive();
        uisys->IsDirty = false;
    }

    for (u32 i = 0; i < uisys->ActiveControlCount; ++i) {
        auto control = uisys->RegisteredControls[i];
        control->Update(rFrameData);
    }
    return true;
}

void UiSystem::RenderPrepareFrame(void* self, const FrameData &rFrameData)
{
    auto uisys = (UiSystem*)self;
    for (u32 i = 0; i < uisys->ActiveControlCount; ++i) {
        uisys->RegisteredControls[i]->RenderPrepare(rFrameData);
    }
}

bool UiSystem::Render(UiElement *root, FrameData &rFrameData, UiRenderData &RenderData)
{
    RenderData.UiAtlas = &UiAtlas;

    if (!root) {
        root = &this->root;
    }

    for (u64 i = 0; i < ActiveControlCount; i++) {
        auto element = RegisteredControls[i];
        if (element->IsVisible) {
            if (!element->Render(rFrameData, RenderData)) {
                MERROR("Не удалось отобразить корневой элемент. Подробнее см. в журналах.");
                return false;
            }
        }
    }

    // if (!root->Render(rFrameData, RenderData)) {
    //     MERROR("Не удалось отобразить корневой элемент. Подробнее см. в журналах.");
    //     return false;
    // }
// 
    // if (root->children) {
    //     const u32 length = root->children.Length();
    //     for (u32 i = 0; i < length; ++i) {
    //         UiElement* control = root->children[i];
    //         if (!control->IsVisible) {
    //             continue;
    //         }
// 
    //         if (!Render(control, rFrameData, RenderData)) {
    //             MERROR("Не удалось отобразить дочерний элемент. Подробнее см. в журналах.");
    //             return false;
    //         }
    //     }
    // }

    return true;
}

bool UiSystem::MouseDown(u16 code, void *sender, void *ListenerInst, const EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.MouseButton = (Buttons)context.data.i16[0];
    event.x = context.data.i16[1];
    event.y = context.data.i16[2];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->RegisteredControls[i];
        if (control->OnMouseDown) {
            auto inv = Matrix4D::MakeInverse(control->xform.GetWorld());
            auto TransformedEvent = VectorTransform(FVec3(event.x, event.y, 0.F), 1.F, inv);
            if (Math::PointInRect2d(Point(TransformedEvent.x, TransformedEvent.y), control->bounds)) { //FVec2?
                control->IsPressed = true;
                control->OnMouseDown(control, event);
            }
        }
    }
    return false;
}

bool UiSystem::MouseUp(u16 code, void *sender, void *ListenerInst, const EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.MouseButton = (Buttons)context.data.i16[0];
    event.x = context.data.i16[1];
    event.y = context.data.i16[2];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->RegisteredControls[i];
        control->IsPressed = false;

        if (control->OnMouseUp) {
            auto inv = Matrix4D::MakeInverse(control->xform.GetWorld());
            auto TransformedEvent = VectorTransform(FVec3(event.x, event.y, 0.F), 1.F, inv);
            if (Math::PointInRect2d(Point(TransformedEvent.x, TransformedEvent.y), control->bounds)) {
                control->OnMouseUp(control, event);
            }
        }
    }
    return false;
}

bool UiSystem::Click(u16 code, void *sender, void *ListenerInst, const EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.MouseButton = (Buttons)context.data.i16[0];
    event.x = context.data.i16[1];
    event.y = context.data.i16[2];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->RegisteredControls[i];
        if (control->OnClick) {
            auto inv = Matrix4D::MakeInverse(control->xform.GetWorld());
            auto TransformedEvent = VectorTransform(FVec3(event.x, event.y, 0.F), 1.F, inv);
            if (Math::PointInRect2d(Point(TransformedEvent.x, TransformedEvent.y), control->bounds)) {
                control->OnClick(control, event);
            }
        }
    }
    return false;
}

bool UiSystem::Move(u16 code, void *sender, void *ListenerInst, const EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.x = context.data.i16[0];
    event.y = context.data.i16[1];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        UiElement* control = pState->RegisteredControls[i];
        if (control->OnMouseOver || control->OnMouseOut) {
            auto inv = Matrix4D::MakeInverse(control->xform.GetWorld());
            FVec3 TransformedEvent = VectorTransform(FVec3(event.x, event.y, 0.F), 1.F, inv);
            Point TransformedPoint {(i32)TransformedEvent.x, (i32)TransformedEvent.y};
            if (Math::PointInRect2d(TransformedPoint, control->bounds)) {
                MTRACE("Курсор под кнопкой: %s", control->name.c_str());
                if (!control->IsHovered) {
                    control->IsHovered = true;
                    if (control->OnMouseOver) control->OnMouseOver(control, event);
                }

                // События перемещения запускаются только тогда, когда элемент управления фактически находится над ним.
                if (control->OnMouseMove) control->OnMouseMove(control, event);
            } else {
                if (control->IsHovered) {
                    control->IsHovered = false;
                    if (control->OnMouseOut) control->OnMouseOut(control, event);
                }
            }
        }
    }
    return false;
}

bool UiSystem::RegisterControl(UiElement *control)
{
    if (TotalControlCount >= MaxControlCount) {
        MERROR("Не удалось найти свободное место для регистрации элемента управления UiSystem. Регистрация не удалась.");
        return false;
    }

    // Нашёл свободное место, разместил там.
    RegisteredControls[TotalControlCount] = control;
    TotalControlCount++;
    
    return (IsDirty = true);
}

bool UiSystem::AddChild(UiElement *child)
{
    return root.AddChild(child);
}

void UiSystem::FocusControl(UiElement *control)
{
    focusedID = control ? control->id.UniqueID : INVALID::U64ID;
}
