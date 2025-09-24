#pragma once
#include "renderer/renderbuffer.h"
#include "math/transform.h"
#include "mesh.h"

    enum class TextType {
        Bitmap,
        System
    };
    
    struct MAPI Text
    {
        MString name;
        u32 UniqueID;
        TextType type;
        struct FontData* data;
        RenderBuffer VertexBuffer;
        RenderBuffer IndexBuffer;
        MString text;
        Transform transform;
        u32 InstanceID;
        u64 RenderFrameNumber;
        u8 DrawIndex;

        // constexpr Text() : UniqueID(), type(), data(), VertexBuffer(), IndexBuffer(), text(), transform(), InstanceID(INVALID::ID), RenderFrameNumber(INVALID::U64ID) {}
        ~Text();
    
        bool Create(const char* name, TextType type, const char* FontName, u16 FontSize, const char* TextContent);
        void Destroy();
    
        void SetPosition(const FVec3& position);
        void SetText(const char* text);
        void SetText(const MString& text);
        void SetText(MString&& text);
        void DeleteLastSymbol();

        /// @brief Добавляет заданный текст к текущему.
        /// @param text указатель на строку.
        void Append(const char* text);
        void Append(char c);
    
        void Draw();

        explicit operator bool() const;
    private:
        void RegenerateGeometry();
    };
