#pragma once
#include "renderer/renderbuffer.hpp"
#include "math/transform.hpp"
#include "mesh.hpp"

    enum class TextType {
        Bitmap,
        System
    };
    
    class MAPI Text
    {
    friend class FontSystem;
    friend class RenderViewUI;
    friend class RenderViewPick;
    public:
        u32 UniqueID;
    private:
        TextType type;
        struct FontData* data;
        RenderBuffer VertexBuffer;
        RenderBuffer IndexBuffer;
        MString text;
        Transform transform;
        u32 InstanceID;
        u64 RenderFrameNumber;
    public:
        constexpr Text() : UniqueID(), type(), data(), VertexBuffer(), IndexBuffer(), text(), transform(), InstanceID(INVALID::ID), RenderFrameNumber(INVALID::U64ID) {}
        ~Text();
    
        bool Create(TextType type, const char* FontName, u16 FontSize, const char* TextContent);
    
        void SetPosition(const FVec3& position);
        void SetText(const char* text);
    
        void Draw();
    private:
        void RegenerateGeometry();
    };
