#include "ui_text.hpp"
#include "systems/font_systems.hpp"
#include "systems/shader_system.hpp"
#include "renderer/renderer.hpp"
#include "math/vertex.hpp"
#include "resources/texture_map.hpp"
#include "resources/font_resource.hpp"

void RegenerateGeometry(Text& text);

Text::~Text()
{
    /*if (text->text) {
        u32 TextLength = string_length(text->text);
        kfree(text->text, sizeof(char) * TextLength, MEMORY_TAG_STRING);
        text->text = 0;
    }

    // Уничтожить буферы.
    renderer_renderbuffer_destroy(&text->vertex_buffer);
    renderer_renderbuffer_destroy(&text->index_buffer);

    // Освободить ресурсы для карты текстуры шрифта.
    shader* ui_shader = shader_system_get(BUILTIN_SHADER_NAME_UI);  // ЗАДАЧА: Текстовый шейдер.
    if (!renderer_shader_release_instance_resources(ui_shader, text->instance_id)) {
        KFATAL("Unable to release shader resources for font texture map.");
    }
    kzero_memory(text, sizeof(ui_text));*/
}

bool Text::Create(TextType type, const char *FontName, u16 FontSize, const char *TextContent)
{
    if (!FontName || !TextContent) {
        MERROR("Text::Text требует допустимый указатель на FontName, TextContent.");
        return false;
    }

    // Сначала назначьте тип
    this->type = type;

    // Получить шрифт правильного типа и назначить его внутренние данные.
    // Это также получает текстуру атласа.
    if (!FontSystem::Acquire(FontName, FontSize, *this)) {
        MERROR("Невозможно получить шрифт: '%s'. текст пользовательского интерфейса не может быть создан.", FontName);
        return false;
    }

    text = TextContent;

    static const u64 QuadSize = (sizeof(Vertex2D) * 4);

    u32 TextLength = text.Length();
    // В случае пустой строки невозможно создать пустой буфер, поэтому просто создайте достаточно, чтобы хранить его на данный момент.
    if (TextLength < 1) {
        TextLength = 1;
    }

    // Получите ресурсы для карты текстуры шрифта.
    auto UiShader = ShaderSystem::GetInstance()->GetShader(BUILTIN_SHADER_NAME_UI);  // ЗАДАЧА: Текстовый шейдер.
    TextureMap* FontMaps[1] = { &data->atlas };
    if (!Renderer::ShaderAcquireInstanceResources(UiShader, FontMaps, InstanceID)) {
        MFATAL("Не удалось получить ресурсы шейдера для карты текстуры шрифта.");
        return false;
    }

    // Сгенерируйте буфер вершин.
    if (!Renderer::RenderBufferCreate(RenderBufferType::Vertex, TextLength * QuadSize, false, VertexBuffer)) {
        MERROR("text_create не удалось создать буфер рендеринга вершин.");
        return false;
    }
    if (!Renderer::RenderBufferBind(VertexBuffer, 0)) {
        MERROR("text_create не удалось привязать буфер рендеринга вершин.");
        return false;
    }

    // Сгенерируйте буфер индексов.
    static const u8 QuadIndexSize = sizeof(u32) * 6;
    if (!Renderer::RenderBufferCreate(RenderBufferType::Index, TextLength * QuadIndexSize, false, IndexBuffer)) {
        MERROR("text_create не удалось создать буфер рендеринга индекса.");
        return false;
    }
    if (!Renderer::RenderBufferBind(IndexBuffer, 0)) {
        MERROR("text_create не удалось привязать буфер рендеринга индекса.");
        return false;
    }

    // Убедитесь, что в атласе есть необходимые глифы.
    if (!FontSystem::VerifyAtlas(data, TextContent)) {
        MERROR("Проверка атласа шрифта не удалась.");
        return false;
    }

    // Сгенерируйте геометрию.
    RegenerateGeometry(*this);

    return true;
}

void RegenerateGeometry(Text &text)
{
}
