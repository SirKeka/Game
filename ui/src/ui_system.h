
/// @file ui_system.h
/// @author Travis Vroman (travis@kohiengine.com)
/// @brief Стандартная система пользовательского интерфейса отвечает за управление стандартными элементами пользовательского интерфейса во всем движке.
/// @version 1.0
/// @date 2023-09-21
/// @copyright Kohi Game Engine is Copyright (c) Travis Vroman 2021-2023

#pragma once

#include "containers/darray.h"
#include "core/identifier.h"
#include "core/input.h"
#include "renderer/renderer_types.h"
#include "resources/texture.h"

// FIXME: Необходимо где-то хранить список типов расширений и использовать его для извлечения информации.
#define M_SYSTEM_TYPE_STANDARD_UI_EXT 128

struct FrameData;
struct EventContext;

struct UiRenderable {
    u32* InstanceID;
    u64* FrameNumber;
    TextureMap* AtlasOverride;
    u8* DrawIndex;
    GeometryRenderData RenderData;
    GeometryRenderData* ClipMaskRenderData;
};

struct UiRenderData {
    TextureMap* UiAtlas;
    DArray<UiRenderable> renderables;
};

struct UiMouseEvent {
    Buttons MouseButton;
    i16 x;
    i16 y;
};

enum UiKeyboardEventType {
    PRESS,
    RELEASE,
};

struct UiKeyboardEvent {
    Keys key;
    UiKeyboardEventType type;
};

struct UiClipMask {
    u32 ReferenceID;
    Transform ClipXform;
    Geometry* ClipGeometry;
    GeometryRenderData RenderData;
};

class MAPI UiElement {
public:
    Identifier id;
    Transform xform{};
    MString name;
    Rect2D bounds{};
    
    UiElement* parent{};
    DArray<UiElement*> children;

    void* UserData{};
    u64 UserDataSize{};
    
    u64 FrameNumber{};
    u32 InstanceID{};
    u8 DrawIndex{};

    // ЗАДАЧА: Преобразовать во флаги.
    bool IsActive{};
    bool IsVisible{true};
    bool IsHovered{};
    bool IsPressed{};

    constexpr UiElement(UiElement* parent = nullptr)                   : name()                { id.Generate(); if (parent) parent->AddChild(this); }
    constexpr UiElement(const char* name, UiElement* parent = nullptr) : name(name)            { id.Generate(); if (parent) parent->AddChild(this); }
    constexpr UiElement(MString&& name,   UiElement* parent = nullptr) : name((MString&&)name) { id.Generate(); if (parent) parent->AddChild(this); }
    virtual ~UiElement() = default;
    
    bool AddChild(UiElement* child);
    bool RemoveChild(UiElement* child);
    
    virtual void Destroy() {}
    virtual bool Load() { return false; }
    virtual void Unload() {}

    virtual bool Update(FrameData& rFrameData) { return false; }
    virtual void RenderPrepare(const FrameData& rFrameData) {}
    virtual bool Render(FrameData& rFrameData, UiRenderData& RenederData) { return true; }

    void SetParent(UiElement* parent);

    /// @brief Устанавливает позицию заданного элемента управления.
    /// @param position Устанавливаемая позиция.
    void SetPosition(FVec3 position);

    void SetPosition(f32 x, f32 y, f32 z = 0.F) {
        xform.SetPosition(x, y, z);
    }

    /// @brief Получает позицию заданного элемента управления.
    /// @return Позиция заданного элемента управления.
    FVec3 GetPosition();

    /// Обработчик щелчков для элемента управления.
    /// @param event Событие мыши.
    /// @returns True, если событие должно распространяться на другие элементы управления; в противном случае — false.
    void (*OnClick    )(UiElement*,    UiMouseEvent) = nullptr;
    void (*OnMouseDown)(UiElement*,    UiMouseEvent) = nullptr;
    void (*OnMouseUp  )(UiElement*,    UiMouseEvent) = nullptr;
    void (*OnMouseOver)(UiElement*,    UiMouseEvent) = nullptr;
    void (*OnMouseOut )(UiElement*,    UiMouseEvent) = nullptr;
    void (*OnMouseMove)(UiElement*,    UiMouseEvent) = nullptr;
    void (*OnKey      )(UiElement*, UiKeyboardEvent) = nullptr;

    // void InternalClick    (UiElement*, UiMouseEvent event);
    // void InternalMouseOver(UiElement*, UiMouseEvent event);
    // void InternalMouseOut (UiElement*, UiMouseEvent event);
    // void InternalMouseDown(UiElement*, UiMouseEvent event);
    // void InternalMouseUp  (UiElement*, UiMouseEvent event);
    // void InternalMouseMove(UiElement*, UiMouseEvent event);
    
    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::UI); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::UI); }
};

class MAPI UiSystem {
public:
    struct Config { u64 MaxControlCount; };

    constexpr UiSystem(const Config& config) : MaxControlCount(config.MaxControlCount), root("_ROOT_"), focusedID(INVALID::U64ID) {}

    u32 GetActiveControlCount() { return ActiveControlCount; }

    /// @brief Инициализирует стандартную систему пользовательского интерфейса.
    /// Следует вызывать дважды: один раз для получения требуемого объёма памяти (передавая state=0), 
    /// а второй раз — для передачи выделенного блока памяти для фактической инициализации системы.
    /// @param MemoryRequirement A pointer to hold the memory requirement as it is calculated.
    /// @param state Блок памяти для хранения состояния или, если требуется сбор данных о памяти, 0.
    /// @param config Конфигурация (UiSystemConfig) для этой системы.
    /// @return True в случае успешного выполнения; в противном случае false.
    static bool Initialize(u64& MemoryRequirement, void* state, const Config& pConfig);

    /// @brief Закрывает стандартную систему пользовательского интерфейса.
    static void Shutdown(void* self);

    TextureMap& Atlas() { return UiAtlas; }
    const u64& FocusedID() const { return focusedID; }

    static bool Update(void* self, FrameData& rFrameData);

    static void RenderPrepareFrame(void* self, const FrameData& rFrameData);
    bool Render(UiElement* root, FrameData& rFrameData, UiRenderData& RenderData);

    bool RegisterControl(UiElement* control);

    bool AddChild(UiElement* child);

    void FocusControl(UiElement* control);

private:
    void UpdateActive();
    static bool MouseDown(u16 code, void* sender, void* ListenerInst, const EventContext& context);
    static bool MouseUp(u16 code, void* sender, void* ListenerInst, const EventContext& context);
    static bool Click(u16 code, void *sender, void *ListenerInst, const EventContext &context);
    static bool Move(u16 code, void *sender, void *ListenerInst, const EventContext &context);

    friend class UiElement;
    u64 MaxControlCount;
    // Массив указателей на элементы управления. Система ими не владеет. Приложение владеет.
    u64 TotalControlCount;
    UiElement** RegisteredControls;
    u32 ActiveControlCount;
    // UiElement** ActiveControls;
    u32 InactiveControlCount;
    // UiElement** InactiveControls;
    // void* mem; // указатель на область памяти где будут хранится элементы интерфейса
    UiElement root;
    TextureMap UiAtlas;

    u64 focusedID;

    bool IsDirty;
};


