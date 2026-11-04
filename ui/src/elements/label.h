#pragma once

#include "../ui_system.h"
#include "resources/font_resource.hpp"

class MAPI Label : public UiElement  {
public:
    Point size;
    FVec4 colour;
    u64 VertexBufferOffset;
    u64 IndexBufferOffset;
    u64 VertexBufferSize;
    u64 IndexBufferSize;
    MString text;
    FontData* data;
    FontType type;
    u32 MaxTextLength;
    
    constexpr Label(UiElement* parent = nullptr) 
    : UiElement(parent), size(), colour(FVec4::One()), 
    VertexBufferOffset(INVALID::U64ID), IndexBufferOffset(INVALID::U64ID), VertexBufferSize(INVALID::U64ID), IndexBufferSize(INVALID::U64ID),
    text(), data(), type(), MaxTextLength(), pendingData(), IsDirty() {};
    constexpr Label(u32 BufferSize, UiElement* parent = nullptr) 
    : UiElement(parent), size(), colour(FVec4::One()), 
    VertexBufferOffset(INVALID::U64ID), IndexBufferOffset(INVALID::U64ID), VertexBufferSize(INVALID::U64ID), IndexBufferSize(INVALID::U64ID),
    text(BufferSize), data(), type(), MaxTextLength(), pendingData(), IsDirty() {};
    constexpr Label(const char* name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);
    constexpr Label(MString&& name,   FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);

    bool Create(const char* name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);
    bool Create(MString& name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);

    bool Load() override {
        // Пометьте его как «грязный», чтобы гарантировать обновление на следующем кадре.
        return text ? IsDirty = true : true;
    }
    void Unload() override;
    void RenderPrepare(const FrameData& rFrameData) override;
    bool Render(FrameData& rFrameData, UiRenderData& RenderData) override;
    
    void ClearText() { text.Destroy(); }

    /// @brief Удаляет часть текста
    /// @param position позиция от которой нужно начать удаление
    /// @param length длина 
    void RemovePart(u32 position, u32 length);

    /// @brief Устанавливает текст для заданного объекта метки.
    /// @param text Текст, который будет установлен.
    void SetText(const char* text, u32 size = 0);

    /// @brief Устанавливает текст для заданного объекта метки.
    /// @param text текст, который будет установлен.
    /// @param copy указывает нужно ли скопировать текст или переместить
    void SetText(MString& text, bool copy = true);
    
    MString& GetText() { return text; }
private:
    void RegenerateGeometry();

    struct PendingData {
        u64 VertexBufferSize      {INVALID::U64ID};
        u64 VertexBufferOffset    {INVALID::U64ID};
        u64 IndexBufferSize       {INVALID::U64ID};
        u64 IndexBufferOffset     {INVALID::U64ID};
        Vertex2D *VertexBufferData{    nullptr   };
        u32 *IndexBufferData      {    nullptr   };
        u32 NewUTF8Length         {              };
        u32 NewLength             {              };
        void Reset();
    } pendingData; 

    bool IsDirty;
};


