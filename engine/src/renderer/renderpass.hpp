#pragma once
#include "math/vector4d_fwd.hpp"

/// @brief Представляет цель рендеринга, которая используется для рендеринга в текстуру или набор текстур.
struct RenderTarget {
    bool SyncToWindowSize;              // Указывает, следует ли обновлять эту цель рендеринга при изменении размера окна.
    u8 AttachmentCount;                 // Количество вложений.
    class Texture** attachments;        // Массив вложений (указателей на текстуры).
    void* InternalFramebuffer;          // Внутренний объект буфера кадра API рендеринга.

    constexpr RenderTarget() : SyncToWindowSize(), AttachmentCount(), attachments(nullptr), InternalFramebuffer(nullptr) {}
    constexpr RenderTarget(bool SyncToWindowSize, u8 AttachmentCount) 
    : SyncToWindowSize(SyncToWindowSize), AttachmentCount(AttachmentCount), attachments(nullptr), InternalFramebuffer(nullptr) {}
    ~RenderTarget();

    void* operator new[] (u64 size);
    void operator delete[] (void* ptr, u64 size);
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
    const char* name;       // Имя этого прохода рендеринга.
    const char* PrevName;   // Имя предыдущего прохода рендеринга.
    const char* NextName;   // Имя следующего прохода рендеринга.
    FVec4 RenderArea;       // Текущая область рендеринга прохода рендеринга.
    FVec4 ClearColour;      // Чистый цвет, используемый для этого прохода рендеринга.

    u8 ClearFlags;          // Флаги очистки для этого прохода рендеринга.
    constexpr RenderpassConfig(const char* name, const char* PrevName, const char* NextName, FVec4 RenderArea, FVec4 ClearColour, u8 ClearFlags)
    : name(name), PrevName(PrevName), NextName(NextName), RenderArea(RenderArea), ClearColour(ClearColour), ClearFlags(ClearFlags) {}
};

/// @brief Представляет собой общий проход рендеринга.
struct Renderpass
{
    u16 id;                 // Идентификатор прохода рендеринга.

    FVec4 RenderArea;       // Текущая область рендеринга прохода рендеринга.
    FVec4 ClearColour;      // Чистый цвет, используемый для этого прохода рендеринга.

    u8 ClearFlags;          // Флаги очистки для этого прохода рендеринга.
    u8 RenderTargetCount;   // Количество целей рендеринга для этого прохода рендеринга.
    RenderTarget* targets;  // Массив целей рендеринга, используемых этим проходом рендеринга.

    void* InternalData;     // Внутренние данные прохода рендеринга.

    constexpr Renderpass() : id(INVALID::U16ID), RenderArea(), ClearColour(), ClearFlags(), RenderTargetCount(), targets(nullptr), InternalData(nullptr) {}
    constexpr Renderpass(FVec4 RenderArea, FVec4 ClearColour, u8 ClearFlags, u8 RenderTargetCount)
    : id(INVALID::U16ID), RenderArea(RenderArea), ClearColour(ClearColour), ClearFlags(ClearFlags), RenderTargetCount(RenderTargetCount), targets(nullptr), InternalData(nullptr) {}

    ~Renderpass();

    void* operator new[] (u64 size);
    void operator delete[] (void* ptr, u64 size);

};
