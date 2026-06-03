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
        if (!RemoveChild(child->parent)) {
            MERROR("Не удалось удалить дочерний элемент из родительского элемента перед сменой родительского элемента.");
            return false;
        }
    }

    parent->children.PushBack(child);

    child->xform.SetParent(&xform);

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
        this->parent = parent;
        xform.SetParent(&parent->xform);
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
    u64 ActiveArrayRequirement = sizeof(UiElement*) * config.MaxControlCount;
    u64 InactiveArrayRequirement = sizeof(UiElement*) * config.MaxControlCount;
    MemoryRequirement = StructRequirement + ActiveArrayRequirement + InactiveArrayRequirement;

    if (!state) {
        return true;
    }

    auto pState = (UiSystem*)state;
    pState->MaxControlCount = config.MaxControlCount;
    pState->ActiveControls = (UiElement**)((u8*)state + StructRequirement);
    MemorySystem::ZeroMem(pState->ActiveControls, sizeof(UiElement) * config.MaxControlCount);
    pState->InactiveControls = (UiElement**)((u8*)pState->ActiveControls + ActiveArrayRequirement);
    MemorySystem::ZeroMem(pState->InactiveControls, sizeof(UiElement) * config.MaxControlCount);

    Texture* atlas = TextureSystem::Acquire("StandardUIAtlas", true);
    if (!atlas) {
        MWARN("Не удалось загрузить текстуру атласа, используются настройки по умолчанию.");
        atlas = TextureSystem::GetDefaultTexture();
    }

    // Настроить текстурную карту.
    TextureMap* map = &pState->UiAtlas;
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
    auto state = (UiSystem*)self;
    EventSystem::Unregister(EventSystem::ButtonClicked,  self, Click);
    EventSystem::Unregister(EventSystem::MouseMoved,     self, Move);
    EventSystem::Unregister(EventSystem::ButtonPressed,  self, MouseDown);
    EventSystem::Unregister(EventSystem::ButtonReleased, self, MouseUp);

    // Выгрузить и уничтожить неактивные элементы управления.
    for (u32 i = 0; i < state->InactiveControlCount; ++i) {
        auto control = state->InactiveControls[i];
        control->Unload();
        control->Destroy();
    }
    // Выгрузить и уничтожить активные элементы управления.
    for (u32 i = 0; i < state->ActiveControlCount; ++i) {
        auto control = state->ActiveControls[i];
        control->Unload();
        control->Destroy();
    }

    if (state->UiAtlas.texture) {
        TextureSystem::Release(state->UiAtlas.texture->name);
        state->UiAtlas.texture = nullptr;
    }

    SystemsManager::GetRenderingSystem()->TextureMapReleaseResources(&state->UiAtlas);
}

bool UiSystem::Update(void* self, FrameData &rFrameData)
{
    auto uisys = (UiSystem*)self;
    for (u32 i = 0; i < uisys->ActiveControlCount; ++i) {
        auto control = uisys->ActiveControls[i];
        control->Update(rFrameData);
    }
    return true;
}

bool UiSystem::Render(UiElement *root, FrameData &rFrameData, UiRenderData &RenderData)
{
    RenderData.UiAtlas = &UiAtlas;

    if (!root) {
        root = &this->root;
    }

    if (!root->Render(rFrameData, RenderData)) {
        MERROR("Не удалось отобразить корневой элемент. Подробнее см. в журналах.");
        return false;
    }

    if (root->children) {
        const u32 length = root->children.Length();
        for (u32 i = 0; i < length; ++i) {
            auto control = root->children[i];
            if (!control->IsVisible) {
                continue;
            }
            if (!Render(control, rFrameData, RenderData)) {
                MERROR("Не удалось отобразить дочерний элемент. Подробнее см. в журналах.");
                return false;
            }
        }
    }

    return true;
}

bool UiSystem::UpdateActive(UiElement *control)
{
    u32& SrcLimit = control->IsActive ? InactiveControlCount : ActiveControlCount;
    u32& DstLimit = control->IsActive ? ActiveControlCount : InactiveControlCount;
    UiElement** SrcArray = control->IsActive ? InactiveControls : ActiveControls;
    UiElement** DstArray = control->IsActive ? ActiveControls : InactiveControls;

    u64 pSize = sizeof(UiElement*);
    for (u32 i = 0; i < SrcLimit; ++i) {
        if (SrcArray[i] == control) {
            auto control = SrcArray[i];
            DstArray[DstLimit] = control;
            DstLimit++;

            // Скопируйте оставшуюся часть массива внутрь.
            MemorySystem::CopyMem(((u8*)SrcArray) + (i * pSize), ((u8*)SrcArray) + ((i + 1) * pSize), pSize * (SrcLimit - i));
            SrcArray[SrcLimit] = 0;
            SrcLimit--;
            return true;
        }
    }

    MERROR("Не удалось найти элемент управления для обновления. Возможно, элемент управления не зарегистрирован?");
    return false;
}

bool UiSystem::MouseDown(u16 code, void *sender, void *ListenerInst, EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.MouseButton = (Buttons)context.data.i16[0];
    event.x = context.data.i16[1];
    event.y = context.data.i16[2];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->ActiveControls[i];
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

bool UiSystem::MouseUp(u16 code, void *sender, void *ListenerInst, EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.MouseButton = (Buttons)context.data.i16[0];
    event.x = context.data.i16[1];
    event.y = context.data.i16[2];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->ActiveControls[i];
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

bool UiSystem::Click(u16 code, void *sender, void *ListenerInst, EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.MouseButton = (Buttons)context.data.i16[0];
    event.x = context.data.i16[1];
    event.y = context.data.i16[2];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->ActiveControls[i];
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

bool UiSystem::Move(u16 code, void *sender, void *ListenerInst, EventContext &context)
{
    auto pState = (UiSystem*)ListenerInst;

    UiMouseEvent event;
    event.x = context.data.i16[0];
    event.y = context.data.i16[1];
    for (u32 i = 0; i < pState->ActiveControlCount; ++i) {
        auto control = pState->ActiveControls[i];
        if (control->OnMouseOver || control->OnMouseOut) {
            auto inv = Matrix4D::MakeInverse(control->xform.GetWorld());
            auto TransformedEvent = VectorTransform(FVec3(event.x, event.y, 0.F), 1.F, inv);
            Point TransformedPoint {(i32)TransformedEvent.x, (i32)TransformedEvent.y};
            if (Math::PointInRect2d(TransformedPoint, control->bounds)) {
                MTRACE("Курсор под кнопкой: %s", control->name.c_str());
                if (!control->IsHovered) {
                    control->IsHovered = true;
                    control->OnMouseOver(control, event);
                }

                // События перемещения запускаются только тогда, когда элемент управления фактически находится над ним.
                control->OnMouseMove(control, event);
            } else {
                if (control->IsHovered) {
                    control->IsHovered = false;
                    control->OnMouseOut(control, event);
                }
            }
        }
    }
    return false;
}

bool UiSystem::RegisterControl(UiElement *control)
{
    if (TotalControlCount >= MaxControlCount) {
        MERROR("Не удалось найти свободное место для регистрации элемента управления sui. Регистрация не удалась.");
        return false;
    }

    TotalControlCount++;
    // Нашёл свободное место, разместил там.
    InactiveControls[InactiveControlCount] = control;
    InactiveControlCount++;
    return true;
}

bool UiSystem::AddChild(UiElement *child)
{
    return root.AddChild(child);
}

void UiSystem::FocusControl(UiElement *control)
{
    focusedID = control ? control->id.UniqueID : INVALID::U64ID;
}
