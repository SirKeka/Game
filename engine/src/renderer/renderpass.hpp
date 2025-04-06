#pragma once
#include "math/vector4d_fwd.h"
#include "containers/darray.hpp"
#include "core/mmemory.hpp"

struct Texture;

enum class RenderTargetAttachmentType {
    Colour = 0x1,
    Depth = 0x2,
    Stencil = 0x4
};

enum class RenderTargetAttachmentSource {
    Default = 0x1,
    View = 0x2
};

enum class RenderTargetAttachmentLoadOperation {
    DontCare = 0x0,
    Load = 0x1
};

enum RenderTargetAttachmentStoreOperation {
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
    RenderTargetAttachmentType type;
    RenderTargetAttachmentSource source;
    RenderTargetAttachmentLoadOperation LoadOperation;
    RenderTargetAttachmentStoreOperation StoreOperation;
    bool PresentAfter;
    Texture* texture;
};

/// @brief Представляет цель рендеринга, которая используется для рендеринга в текстуру или набор текстур.
struct RenderTarget {
    u8 AttachmentCount                        {};   // Количество вложений.
    RenderTargetAttachment* attachments{nullptr};   // Массив вложений.
    void* InternalFramebuffer          {nullptr};   // Внутренний объект буфера кадра API рендеринга.

    constexpr RenderTarget() : AttachmentCount(), attachments(nullptr), InternalFramebuffer(nullptr) {}
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
struct Renderpass
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
    constexpr Renderpass(const RenderpassConfig& config)
    : 
    id(INVALID::U16ID), 
    name(config.name),
    RenderArea(config.RenderArea), 
    ClearColour(config.ClearColour), 
    ClearFlags(config.ClearFlags), 
    RenderTargetCount(config.RenderTargetCount), 
    targets(MemorySystem::TAllocate<RenderTarget>(Memory::Array, RenderTargetCount)), 
    InternalData(nullptr) {}
    
    ~Renderpass();

    void* operator new(u64 size);
    void* operator new[] (u64 size);
    void operator delete(void* ptr, u64 size);
    void operator delete[] (void* ptr, u64 size);

};

// Явное инстанцирование
template class DArray<RenderTargetAttachmentConfig>; 
template class DArray<RenderpassConfig>;