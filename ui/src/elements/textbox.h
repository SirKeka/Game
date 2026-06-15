#pragma once

#include <math/nine_slice.h>

#include "label.h"
#include "panel.h"

// ЗАДАЧА: Элементы текстового поля
// - Возможность выделять текст, а затем добавлять/удалять/перезаписывать выделенный текст.
struct MAPI Textbox : public UiElement {
    Point size;
    FVec4 colour;
    NineSlice nslice;
    Label ContentLabel;
    Panel cursor;
    Panel HighlightBox;
    Range32 HighlightRange;
    u32 CursorPosition;
    f32 TextViewOffset;
    UiClipMask ClipMask;

    constexpr Textbox(UiElement* parent = nullptr);
    Textbox(const char* name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);

    bool Create(const char* name, FontType type, const char* FontName, u16 FontSize, const char* text, UiElement* parent = nullptr);

    bool SetHeight(i32 height) { return SetSize(size.x, height); }
    bool SetSize(i32 width, i32 height);
    bool SetWidth(i32 width) { return SetSize(width, size.y); }

    bool Load() override;

    void Unload() override;

    bool Update(FrameData& rFrameData) override { return true; }

    bool Render(FrameData& rFrameData, UiRenderData& RenderData) override;

    const MString& GetText() { return ContentLabel.GetText(); }
    void SetText(const char* text);
    void SetText(MString& text, bool copy = true);
    static void OnMouseDown(UiElement* self, UiMouseEvent event);
    static void OnMouseUp  (UiElement* self, UiMouseEvent event);
    void OnKey(UiKeyboardEvent event) override;

private:
    static bool OnKey(u16 code, void* sender, void* ListenerInst, EventContext& context);
    f32 CalculateCursorOffset(u32 StringPosition, MString& FullString, FontData* font);
    void UpdateCursorPosition();
    void UpdateHighlightBox();
};

constexpr int bkhad = sizeof(Textbox);