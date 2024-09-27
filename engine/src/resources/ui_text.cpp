#include "ui_text.hpp"
#include "core/identifier.hpp"
#include "renderer/renderer.hpp"
#include "systems/font_system.hpp"
#include "systems/shader_system.hpp"
#include "math/vertex.hpp"
#include "resources/texture_map.hpp"
#include "resources/font_resource.hpp"

Text::~Text()
{
    /*if (text->text) {
        u32 TextLength = string_length(text->text);
        kfree(text->text, sizeof(char) * TextLength, MEMORY_TAG_STRING);
        text->text = 0;
    }*/
    // Release the unique identifier.
    Identifier::ReleaseID(UniqueID);

    // Уничтожить буферы.
    Renderer::RenderBufferDestroy(VertexBuffer);
    Renderer::RenderBufferDestroy(IndexBuffer);

    // Освободить ресурсы для карты текстуры шрифта.
    auto UiShader = ShaderSystem::GetShader("Shader.Builtin.UI");  // ЗАДАЧА: Текстовый шейдер.
    if (!Renderer::ShaderReleaseInstanceResources(UiShader, InstanceID)) {
        MFATAL("Невозможно освободить ресурсы шейдера для текстурной карты шрифта.");
    }
    MMemory::ZeroMem(this, sizeof(Text));
}

bool Text::Create(TextType type, const char *FontName, u16 FontSize, const char *TextContent)
{
    if (!FontName || !TextContent) {
        MERROR("Text::Text требует допустимый указатель на FontName, TextContent.");
        return false;
    }

    // Сначала назначьте тип
    this->type = type;
    transform = Transform();

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
    auto UiShader = ShaderSystem::GetShader("Shader.Builtin.UI");  // ЗАДАЧА: Текстовый шейдер.
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
    RegenerateGeometry();

    // Получите уникальный идентификатор для текстового объекта.
    UniqueID = Identifier::AquireNewID(this);

    return true;
}

void Text::SetPosition(const FVec3& position)
{
    transform.SetPosition(position);
}

void Text::SetText(const char *text)
{
    if (text) {
        // Если строки уже равны, ничего не делайте.
        if (this->text == text) {
            return;
        }

        this->text = text;

        // Проверьте, есть ли в атласе необходимые глифы.
        if (!FontSystem::VerifyAtlas(data, text)) {
            MERROR("Проверка атласа шрифтов не удалась.");
        }

        RegenerateGeometry();
    }
}

void Text::Draw()
{
    // ЗАДАЧА: длина utf8
    static const u64 QuadVertCount = 4;
    if (!Renderer::RenderBufferDraw(VertexBuffer, 0, text.Length() * QuadVertCount, true)) {
        MERROR("Не удалось нарисовать буфер вершин шрифта пользовательского интерфейса.");
    }

    static const u8 QuadIndexCount = 6;
    if (!Renderer::RenderBufferDraw(IndexBuffer, 0, text.Length() * QuadIndexCount, false)) {
        MERROR("Не удалось нарисовать буфер индекса шрифта пользовательского интерфейса.");
    }
}

void Text::RegenerateGeometry()
{
    // Получить длину строки UTF-8
    u32 TextLengthUTF8 = text.Length();
    // Также получить длину в символах.
    u32 CharLength = text.nChar();

    // Рассчитать размеры буфера.
    static const u64 VertsPerQuad = 4;
    static const u8 IndicesPerQuad = 6;
    u64 VertexBufferSize = sizeof(Vertex2D) * VertsPerQuad * TextLengthUTF8;
    u64 IndexBufferSize = sizeof(u32) * IndicesPerQuad * TextLengthUTF8;

    // Изменить размер буфера вершин, но только если он больше.
    if (VertexBufferSize > VertexBuffer.TotalSize) {
        if (!Renderer::RenderBufferResize(VertexBuffer, VertexBufferSize)) {
            MERROR("Text::RegenerateGeometry для текста пользовательского интерфейса не удалось изменить размер буфера визуализации вершин.");
            return;
        }
    }

    // Изменить размер буфера индексов, но только если он больше.
    if (IndexBufferSize > IndexBuffer.TotalSize) {
        if (!Renderer::RenderBufferResize(IndexBuffer, IndexBufferSize)) {
            MERROR("Text::RegenerateGeometry для текста пользовательского интерфейса не удалось изменить размер индексного буфера визуализации.");
            return;
        }
    }

    // Сгенерировать новую геометрию для каждого символа.
    f32 x = 0;
    f32 y = 0;
    // Временные массивы для хранения данных вершин/индексов.
    Vertex2D* VertexBufferData = MMemory::TAllocate<Vertex2D>(Memory::Array, VertexBufferSize);
    u32* IndexBufferData = MMemory::TAllocate<u32>(Memory::Array, IndexBufferSize);

    // Возьмите длину в символах и получите из нее правильный код.
    for (u32 c = 0, uc = 0; c < CharLength; ++c) {
        i32 codepoint = text[c];

        // Продолжайте на следующей строке для новой строки.
        if (codepoint == '\n') {
            x = 0;
            y += data->LineHeight;
            // Увеличьте количество символов utf-8.
            uc++;
            continue;
        }

        if (codepoint == '\t') {
            x += data->TabXAdvance;
            uc++;
            continue;
        }

        // ПРИМЕЧАНИЕ: Обработка кодовых точек UTF-8.
        u8 advance = 0;
        if (!text.BytesToCodepoint(c, codepoint, advance)) {
            MWARN("В строке обнаружен недопустимый UTF-8, использующий неизвестный код -1");
            codepoint = -1;
        }

        FontGlyph* g = nullptr;
        for (u32 i = 0; i < data->GlyphCount; ++i) {
            if (data->glyphs[i].codepoint == codepoint) {
                g = &data->glyphs[i];
                break;
            }
        }

        if (!g) {
            // Если не найдено, используйте кодовую точку -1
            codepoint = -1;
            for (u32 i = 0; i < data->GlyphCount; ++i) {
                if (data->glyphs[i].codepoint == codepoint) {
                    g = &data->glyphs[i];
                    break;
                }
            }
        }

        if (g) {
            // Найден глиф. Сгенерировать точки.
            f32 minx = x + g->xOffset;
            f32 miny = y + g->yOffset;
            f32 maxx = minx + g->width;
            f32 maxy = miny + g->height;
            f32 tminx = (f32)g->x / data->AtlasSizeX;
            f32 tmaxx = (f32)(g->x + g->width) / data->AtlasSizeX;
            f32 tminy = (f32)g->y / data->AtlasSizeY;
            f32 tmaxy = (f32)(g->y + g->height) / data->AtlasSizeY;
            // Перевернуть ось Y для системного текста
            if (type == TextType::System) {
                tminy = 1.0f - tminy;
                tmaxy = 1.0f - tmaxy;
            }

            Vertex2D p0 { FVec2(minx, miny), FVec2(tminx, tminy) };
            Vertex2D p1 { FVec2(maxx, miny), FVec2(tmaxx, tminy) };
            Vertex2D p2 { FVec2(maxx, maxy), FVec2(tmaxx, tmaxy) };
            Vertex2D p3 { FVec2(minx, maxy), FVec2(tminx, tmaxy) };

            VertexBufferData[(uc * 4) + 0] = p0;  // 0    3
            VertexBufferData[(uc * 4) + 1] = p2;  //
            VertexBufferData[(uc * 4) + 2] = p3;  //
            VertexBufferData[(uc * 4) + 3] = p1;  // 2    1

            // Попробовать найти кернинг
            i32 kerning = 0;

            // Получить смещение следующего символа. Если нет продвижения, 
            // перейти вперед на один, в противном случае использовать продвижение как есть.
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            if (offset < TextLengthUTF8 - 1) {
                // Получить следующую кодовую точку.
                i32 NextCodepoint = 0;
                u8 AdvanceNext = 0;

                if (!text.BytesToCodepoint(offset, NextCodepoint, AdvanceNext)) {
                    MWARN("В строке обнаружен недопустимый UTF-8, использующий неизвестный код -1");
                    codepoint = -1;
                } else {
                    for (u32 i = 0; i < data->KerningCount; ++i) {
                        auto& k = data->kernings[i];
                        if (k.Codepoint0 == codepoint && k.Codepoint1 == NextCodepoint) {
                            kerning = k.amount;
                        }
                    }
                }
            }
            x += g->xAdvance + kerning;

        } else {
            MERROR("Не удалось найти неизвестный код. Пропуск.");
            // Увеличить количество символов utf-8.
            uc++;
            continue;
        }

        // Индекс данных 210301
        IndexBufferData[(uc * 6) + 0] = (uc * 4) + 2;
        IndexBufferData[(uc * 6) + 1] = (uc * 4) + 1;
        IndexBufferData[(uc * 6) + 2] = (uc * 4) + 0;
        IndexBufferData[(uc * 6) + 3] = (uc * 4) + 3;
        IndexBufferData[(uc * 6) + 4] = (uc * 4) + 0;
        IndexBufferData[(uc * 6) + 5] = (uc * 4) + 1;

        // Теперь переходим к c
        c += advance - 1;  // Вычитание 1, так как цикл всегда увеличивается один раз для однобайтовых символов.
        // Увеличить количество символов utf-8.
        uc++;
    }

    // Загрузить данные.
    bool VertexLoadResult = Renderer::RenderBufferLoadRange(VertexBuffer, 0, VertexBufferSize, VertexBufferData);
    bool IndexLoadResult = Renderer::RenderBufferLoadRange(IndexBuffer, 0, IndexBufferSize, IndexBufferData);

    // Очистить.
    MMemory::Free(VertexBufferData, VertexBufferSize * sizeof(Vertex2D), Memory::Array);
    MMemory::Free(IndexBufferData, IndexBufferSize * sizeof(u32), Memory::Array);

    // Проверить результаты.
    if (!VertexLoadResult) {
        MERROR("Text::RegenerateGeometry не удалось загрузить данные в диапазон буфера вершин.");
    }
    if (!IndexLoadResult) {
        MERROR("Text::RegenerateGeometry не удалось загрузить данные в диапазон буфера индексов.");
    }
}
