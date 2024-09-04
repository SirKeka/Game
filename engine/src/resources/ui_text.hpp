#pragma once
#include "renderer/renderbuffer.hpp"
#include "math/transform.hpp"

enum class TextType {
    Bitmap,
    System
};

class Text
{
private:
    friend class FontSystem;

    TextType type;
    struct FontData* data;
    RenderBuffer VertexBuffer;
    RenderBuffer IndexBuffer;
    MString text;
    Transform transform;
    u32 InstanceID;
    u64 RenderFrameNumber;
public:
    constexpr Text(): type(), data(), VertexBuffer(), IndexBuffer(), text(), transform(), InstanceID(INVALID::ID), RenderFrameNumber(INVALID::U64ID) {}
    ~Text();

    bool Create(TextType type, const char* FontName, u16 FontSize, const char* TextContent);

    void SetPosition(FVec3 position);
    void SetText(const char* text);

    void Draw();
};
