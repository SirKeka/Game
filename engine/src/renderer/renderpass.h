#pragma once
#include "math/vector4d_fwd.h"
#include "containers/mstring.h"
#include "containers/darray.h"

struct Texture;

enum class RenderTargetAttachmentType : u8 {
    Colour = 0x1,
    Depth = 0x2,
    Stencil = 0x4
};

MINLINE RenderTargetAttachmentType operator|(RenderTargetAttachmentType l, RenderTargetAttachmentType r) 
{
    return RenderTargetAttachmentType((i32)l | (i32)r);
}

MINLINE bool operator&(RenderTargetAttachmentType l, RenderTargetAttachmentType r) 
{
    return (i32)l & (i32)r;
}

enum class RenderTargetAttachmentSource : u8 {
    Default = 0x1,
    Self = 0x2
};

enum class RenderTargetAttachmentLoadOperation : u8 {
    DontCare = 0x0,
    Load = 0x1
};

enum RenderTargetAttachmentStoreOperation : u8 {
    DontCare = 0x0,
    Store = 0x1
};

struct RenderTargetAttachmentConfig {
    RenderTargetAttachmentType type;
    RenderTargetAttachmentSource source;
    RenderTargetAttachmentLoadOperation LoadOperation;
    RenderTargetAttachmentStoreOperation StoreOperation;
    bool PresentAfter;
};

struct RenderTargetConfig {
    u8 AttachmentCount;
    RenderTargetAttachmentConfig* attachments;
};

struct RenderTargetAttachment {
    Texture* texture;
    RenderTargetAttachmentType type;
    RenderTargetAttachmentSource source;
    RenderTargetAttachmentLoadOperation LoadOperation;
    RenderTargetAttachmentStoreOperation StoreOperation;
    bool PresentAfter;
};

/// @brief Представляет цель рендеринга, которая используется для рендеринга в текстуру или набор текстур.
struct RenderTarget {
    u8 AttachmentCount                        {};   // Количество вложений.
    RenderTargetAttachment* attachments{nullptr};   // Массив вложений.
    void* InternalFramebuffer          {nullptr};   // Внутренний объект буфера кадра API рендеринга.
};

/// @brief Типы очистки, которые необходимо выполнить на этапе рендеринга. 
/// Могут быть объединены вместе для нескольких функций очистки.
namespace RenderpassClearFlag {
    enum {
        None = 0x0,
        ColourBuffer = 0x1,
        DepthBuffer = 0x2,
        StencilBuffer = 0x4
    };
} // namespace RenderpassClear

struct RenderpassConfig {
    const char* name;           // Имя этого прохода рендеринга.
    f32 depth;  
    u32 stencil;    
    FVec4 RenderArea;           // Текущая область рендеринга прохода рендеринга.
    FVec4 ClearColour;          // Чистый цвет, используемый для этого прохода рендеринга.
    u8 ClearFlags;              // Флаги очистки для этого прохода рендеринга.
    u8 RenderTargetCount;       // Количество целей рендеринга, созданных в соответствии с конфигурацией цели рендеринга.
    RenderTargetConfig target;  // Конфигурация цели рендеринга.
};

/// @brief Представляет собой общий проход рендеринга.
struct MAPI Renderpass
{
    u16 id;                 // Идентификатор прохода рендеринга.

    MString name;

    FVec4 RenderArea;       // Текущая область рендеринга прохода рендеринга.
    FVec4 ClearColour;      // Чистый цвет, используемый для этого прохода рендеринга.

    u8 ClearFlags;          // Флаги очистки для этого прохода рендеринга.
    u8 RenderTargetCount;   // Количество целей рендеринга для этого прохода рендеринга.
    RenderTarget* targets;  // Массив целей рендеринга, используемых этим проходом рендеринга.

    void* InternalData;     // Внутренние данные прохода рендеринга.

    constexpr Renderpass() : id(INVALID::U16ID), name(), RenderArea(), ClearColour(), ClearFlags(), RenderTargetCount(), targets(nullptr), InternalData(nullptr) {}
    constexpr Renderpass(RenderpassConfig& config);

    
    ~Renderpass();

    void* operator new(u64 size);
    void* operator new[] (u64 size);
    void operator delete(void* ptr, u64 size);
    void operator delete[] (void* ptr, u64 size);

};

// Явное инстанцирование
template class DArray<RenderTargetAttachmentConfig>; 
template class DArray<RenderpassConfig>;