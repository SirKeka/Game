#include "label.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "resources/shader.h"
#include "systems/font_system.h"
#include "systems/shader_system.h"

static bool LabelCreate(FontData* data, const MString& text, u32 InstanceID) {
    // Получите ресурсы для карты текстуры шрифта.
    // ЗАДАЧА: Должна ли быть возможность переопределения для шейдера?
    auto UiShader = ShaderSystem::GetShader("Shader.StandardUI");  // ЗАДАЧА: Текстовый шейдер.
    TextureMap* FontMaps[1] = { &data->atlas }; 
    u16 AtlasLocation = UiShader->uniforms[UiShader->InstanceSamplerIndices[0]].index;
    // Известно количество карт этого типа.
    ShaderInstanceUniformTextureConfig AtlasTexture{
        .UniformLocation = AtlasLocation,
        .TextureMapCount = 1,
        .TextureMaps = FontMaps
    };
    
    ShaderInstanceResourceConfig InstanceResourceConfig{
        .UniformConfigCount = 1,
        .UniformConfigs = &AtlasTexture
    };

    if (!SystemsManager::GetRenderingSystem()->ShaderAcquireInstanceResources(UiShader, InstanceResourceConfig, InstanceID)) {
        MFATAL("Не удалось получить ресурсы шейдера для карты текстуры шрифта.");
        return false;
    }

    // Убедитесь, что в атласе есть необходимые глифы.
    if (!FontSystem::VerifyAtlas(data, text)) {
        MERROR("Проверка атласа шрифтов не удалась.");
        return false;
    }

    return true;
}

constexpr Label::Label()
{
    MemorySystem::ZeroMem(this, sizeof(Label));
}

constexpr Label::Label(const char *name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
    : UiElement(name, parent), colour(FVec4::One()), geometry(), text(text), data(FontSystem::Acquire(FontName, FontSize, type)), type(type)
{
    LabelCreate(data, this->text, InstanceID);
}

constexpr Label::Label(MString &&name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
: UiElement((MString&&)name, parent), colour(FVec4::One()), geometry(), text(text), data(FontSystem::Acquire(FontName, FontSize, type)), type(type)
{
    LabelCreate(data, text, InstanceID);
}

bool Label::Create(const char *name, FontType type, const char *FontName, u16 FontSize, const char* text, UiElement *parent)
{
    this->name = name;
    SetParent(parent);
    colour = FVec4::One();
    this->text = text;
    data = FontSystem::Acquire(FontName, FontSize, type);
    this->type = type;
    return LabelCreate(data, text, InstanceID);
}

bool Label::Load()
{
    if (text) {
        static const u64 QuadVertexSize = (sizeof(Vertex2D) * 4);
        static const u64 QuadIndexSize = (sizeof(u32) * 6);
        u64 TextLength = text.Length();

        // Выделите место в буферах.
        auto renderer = SystemsManager::GetRenderingSystem();
        auto buffer = renderer->GetRenderbuffer(RenderBufferType::Vertex);
        if (!buffer->Allocate(QuadVertexSize * TextLength, geometry.VertexBufferOffset)) {
            MERROR("Label::Load не удалось выделить память из буфера вершин рендерера!");
            return false;
        }

        buffer = renderer->GetRenderbuffer(RenderBufferType::Index);
        if (!buffer->Allocate(QuadIndexSize * TextLength, geometry.IndexBufferOffset)) {
            MERROR("Label::Load не удалось выделить память из буфера индексов рендерера!");
            return false;
        }
    }
    // Генерация геометрии.
    RegenerateGeometry();

    return true;
}

void Label::Unload()
{
    if (text) {
        text.Destroy();
    }

    auto renderer = SystemsManager::GetRenderingSystem();
    auto buffer = renderer->GetRenderbuffer(RenderBufferType::Vertex);
    // Освободить ресурсы из буфера вершин.
    if (MaxTextLength > 0) {
       buffer->Free(sizeof(Vertex2D) * 4 * MaxTextLength, geometry.VertexBufferOffset);
    }

    // Освободить ресурсы из буфера индексов.
    if (geometry.VertexBufferOffset != INVALID::U64ID) {
        static const u64 QuadIndexSize = (sizeof(u32) * 6);
        auto buffer = renderer->GetRenderbuffer(RenderBufferType::Index);
        if (MaxTextLength > 0) {
            buffer->Free(QuadIndexSize * MaxTextLength, geometry.IndexBufferOffset);
        }
        geometry.VertexBufferOffset = INVALID::U64ID;
    }

    // Освободить ресурсы для карты текстуры шрифта.
    if (geometry.IndexBufferOffset != INVALID::U64ID) {
        auto UiShader = ShaderSystem::GetShader("Shader.StandardUI");  // ЗАДАЧА: шейдер текста.
        if (!renderer->ShaderReleaseInstanceResources(UiShader, InstanceID)) {
            MFATAL("Невозможно освободить ресурсы шейдера для карты текстуры шрифта.");
        }
        geometry.IndexBufferOffset = INVALID::U64ID;
    }
}

bool Label::Render(FrameData &rFrameData, UiRenderData &RenderData)
{
    const u32 tLength = text.Length();
    if (tLength) {
        UiRenderable renderable{};
        renderable.RenderData.UniqueID = id.UniqueID;
        geometry.VertexCount = tLength * 4;
        //geometry.VertexElementSize = sizeof(Vertex2D);
        geometry.IndexCount = tLength * 6;
        //geometry.IndexElementSize = sizeof(u32);
        renderable.RenderData.UiData = &geometry;

        // ПРИМЕЧАНИЕ: Переопределите атлас пользовательского интерфейса по умолчанию и используйте вместо него загруженный шрифт.
        renderable.AtlasOverride = &data->atlas;

        renderable.RenderData.model = xform.GetWorld();
        renderable.RenderData.DiffuseColour = colour;

        renderable.InstanceID = &InstanceID;
        renderable.FrameNumber = &FrameNumber;
        renderable.DrawIndex = &DrawIndex;

        RenderData.renderables.PushBack(renderable);
    }

    return true;
}

void Label::SetText(const char *text)
{
    // Если строки уже равны, ничего не делайте.
    if (this->text == text) {
        return;
    }

    this->text = text;

    // Убедитесь, что в атласе есть необходимые глифы.
    if (!FontSystem::VerifyAtlas(data, text)) {
        MERROR("Проверка атласа шрифтов не удалась.");
    }

    RegenerateGeometry();
}

void Label::SetText(MString &text, bool copy)
{
    // Если строки уже равны, ничего не делайте.
    if (this->text == text) {
        return;
    }

    if (copy) {
        this->text = text;
    } else {
        this->text = (MString&&)text;
    }    

    // Убедитесь, что в атласе есть необходимые глифы.
    if (!FontSystem::VerifyAtlas(data, this->text.c_str())) {
        MERROR("Проверка атласа шрифтов не удалась.");
    }

    RegenerateGeometry();
}

void Label::RegenerateGeometry()
{
    // Получите длину строки UTF-8.
    const u32& TextLengthUTF8 = text.Length();
    // Также получите длину в символах.
    const u32& CharLength = text.Size();

    bool NeedsRealloc = TextLengthUTF8 > MaxTextLength;

    // Не пытайтесь перегенерировать геометрию для объекта, в котором нет текста.
    if (TextLengthUTF8 < 1) {
        return;
    }

    // Рассчитайте размеры буферов.
    static const u64 VertsPerQuad = 4;
    static const u8 IndicesPerQuad = 6;
    u64 vsize = sizeof(Vertex2D);
    u64 u32size = sizeof(u32);
    u64 PrevVertexBufferSize = vsize * VertsPerQuad * MaxTextLength;
    u64 PrevIndexBufferSize = u32size * IndicesPerQuad * MaxTextLength;
    u64 VertexBufferSize = vsize * VertsPerQuad * TextLengthUTF8;
    u64 IndexBufferSize = u32size * IndicesPerQuad * TextLengthUTF8;

    auto renderer = SystemsManager::GetRenderingSystem();
    auto VertexBuffer = renderer->GetRenderbuffer(RenderBufferType::Vertex);
    auto IndexBuffer  = renderer->GetRenderbuffer(RenderBufferType::Index);

    if (NeedsRealloc) {
        // Перераспределите память из буфера вершин.
        if (MaxTextLength > 0) {
            if (!VertexBuffer->Free(PrevVertexBufferSize, geometry.VertexBufferOffset)) {
                MERROR("Не удалось освободить память из буфера вершин рендерера: размер=%u, смещение=%u", VertexBufferSize, geometry.VertexBufferOffset);
            }
        }
        if (!VertexBuffer->Allocate(VertexBufferSize, geometry.VertexBufferOffset)) {
            MERROR("Функции Label::RegenerateGeometry не удалось выделить память из буфера вершин рендерера!");
            return;
        }

        // Перераспределите память из буфера индексов.
        if (MaxTextLength > 0) {
            if (!IndexBuffer->Free(PrevIndexBufferSize, geometry.IndexBufferOffset)) {
                MERROR("Не удалось освободить память из буфера индексов рендерера: размер=%u, смещение=%u", IndexBufferSize, geometry.IndexBufferOffset);
            }
        }
        if (!IndexBuffer->Allocate(IndexBufferSize, geometry.IndexBufferOffset)) {
            MERROR("Функции Label::RegenerateGeometry не удалось выделить память из буфера индексов рендерера!");
            return;
        }
    }

    // Обновите максимальную длину, если строка стала длиннее.
    if (TextLengthUTF8 > MaxTextLength) {
        MaxTextLength = TextLengthUTF8;
    }

    // Сгенерировать новую геометрию для каждого символа.
    f32 x = 0;
    f32 y = 0;
    // Временные массивы для хранения данных вершин/индексов.
    auto VertexBufferData = (Vertex2D*)MemorySystem::Allocate(VertexBufferSize, Memory::Array);
    u32* IndexBufferData = (u32*)MemorySystem::Allocate(IndexBufferSize, Memory::Array);

    // Извлечь длину в символах и получить из неё правильный код.
    for (u32 c = 0, uc = 0; c < CharLength; ++c) {
        i32 codepoint = text[c];

        // Перейти на следующую строку для перевода строки.
        if (codepoint == '\n') {
            x = 0;
            y += data->LineHeight;
            // Увеличить количество символов UTF-8.
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
        if (!MString::BytesToCodepoint(text.c_str(), c, codepoint, advance)) {
            MWARN("В строке обнаружен недопустимый UTF-8, использующий неизвестный код -1.");
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
            // Если не найдено, используйте код -1.
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
            // Перевернуть ось Y для системного текста.
            if (type == FontType::System) {
                tminy = 1.f - tminy;
                tmaxy = 1.f - tmaxy;
            }

                auto p0 = Vertex2D(FVec2(minx, miny), FVec2(tminx, tminy));
                auto p1 = Vertex2D(FVec2(maxx, miny), FVec2(tmaxx, tminy));
                auto p2 = Vertex2D(FVec2(maxx, maxy), FVec2(tmaxx, tmaxy));
                auto p3 = Vertex2D(FVec2(minx, maxy), FVec2(tminx, tmaxy));

            VertexBufferData[(uc * 4) + 0] = p0;  // 0    3
            VertexBufferData[(uc * 4) + 1] = p2;  //
            VertexBufferData[(uc * 4) + 2] = p3;  //
            VertexBufferData[(uc * 4) + 3] = p1;  // 2    1

            // Попробовать найти кернинг.
            i32 kerning = 0;

            // Получить смещение следующего символа. Если смещения нет, перейти на один символ вперёд, в противном случае использовать смещение как есть.
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            if (offset < TextLengthUTF8 - 1) {
                // Получить следующую кодовую точку.
                i32 NextCodepoint = 0;
                u8 AdvanceNext = 0;

                if (!MString::BytesToCodepoint(text.c_str(), offset, NextCodepoint, AdvanceNext)) {
                    MWARN("В строке обнаружен недопустимый UTF-8, используется неизвестный код -1.");
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
            // Увеличить количество символов UTF-8.
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
        c += advance - 1;  // Вычитаем 1, поскольку цикл всегда увеличивается на один символ для однобайтовых данных.
        // Увеличиваем количество символов UTF-8.
        uc++;
    }

    // Загружаем данные.
    bool VertexLoadResult = renderer->RenderBufferLoadRange(*VertexBuffer, geometry.VertexBufferOffset, VertexBufferSize, VertexBufferData);
    bool IndexLoadResult = renderer->RenderBufferLoadRange(*IndexBuffer, geometry.IndexBufferOffset, IndexBufferSize, IndexBufferData);

    // Очищаем.
    MemorySystem::Free(VertexBufferData, VertexBufferSize, Memory::Array);
    MemorySystem::Free(IndexBufferData, IndexBufferSize, Memory::Array);

    // Проверяем результаты.
    if (!VertexLoadResult) {
        MERROR("Функции Label::RegenerateGeometry не удалось загрузить данные в диапазон буфера вершин.");
    }
    if (!IndexLoadResult) {
        MERROR("Функции Label::RegenerateGeometry не удалось загрузить данные в диапазон буфера индексов.");
    }
}
