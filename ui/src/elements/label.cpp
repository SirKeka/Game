#include "label.h"
#include "core/asserts.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "resources/shader.h"
#include "systems/font_system.h"
#include "systems/shader_system.h"

static bool LabelCreate(FontData* data, const MString& text, u32& InstanceID) {
    // Получите ресурсы для карты текстуры шрифта.
    // ЗАДАЧА: Должна ли быть возможность переопределения для шейдера?
    auto UiShader = ShaderSystem::GetShader("Shader.StandardUI");  // ЗАДАЧА: Текстовый шейдер.
    TextureMap* FontMaps[1] = { &data->atlas }; 
    u16 AtlasLocation = UiShader->uniforms[UiShader->InstanceSamplerIndices[0]].index;
    // Известно количество карт этого типа.
    ShaderInstanceUniformTextureConfig AtlasTexture {
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

constexpr Label::Label(const char *name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
    : UiElement(name, parent), size(), colour(FVec4::One()), 
    VertexBufferOffset(INVALID::U64ID), IndexBufferOffset(INVALID::U64ID), VertexBufferSize(INVALID::U64ID), IndexBufferSize(INVALID::U64ID), 
    text(), data(FontSystem::Acquire(FontName, FontSize, type)), type(type), MaxTextLength()
{
    LabelCreate(data, this->text, InstanceID);
    SetText(text);
}

constexpr Label::Label(MString &&name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
: UiElement((MString&&)name, parent), size(), colour(FVec4::One()), 
VertexBufferOffset(INVALID::U64ID), IndexBufferOffset(INVALID::U64ID), VertexBufferSize(INVALID::U64ID), IndexBufferSize(INVALID::U64ID),
text(), data(FontSystem::Acquire(FontName, FontSize, type)), type(type), MaxTextLength()
{
    LabelCreate(data, this->text, InstanceID);
    SetText(text);
}

bool Label::Create(const char *name, FontType type, const char *FontName, u16 FontSize, const char* text, UiElement *parent)
{
    this->name = name;
    if (parent) parent->AddChild(this);
    colour = FVec4::One();
    data = FontSystem::Acquire(FontName, FontSize, type);
    SetText(text);
    this->type = type;
    return LabelCreate(data, this->text, InstanceID);
}

bool Label::Create(MString &name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
{
    this->name = (MString&&)name;
    if (parent) parent->AddChild(this);
    colour = FVec4::One();
    data = FontSystem::Acquire(FontName, FontSize, type);
    SetText(text);
    this->type = type;
    return LabelCreate(data, this->text, InstanceID);
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
       buffer->Free(sizeof(Vertex2D) * 4 * MaxTextLength, VertexBufferOffset);
    }

    // Освободить ресурсы из буфера индексов.
    if (VertexBufferOffset != INVALID::U64ID) {
        static const u64 QuadIndexSize = (sizeof(u32) * 6);
        auto buffer = renderer->GetRenderbuffer(RenderBufferType::Index);
        if (MaxTextLength > 0) {
            buffer->Free(QuadIndexSize * MaxTextLength, IndexBufferOffset);
        }
        VertexBufferOffset = INVALID::U64ID;
    }

    // Освободить ресурсы для карты текстуры шрифта.
    if (IndexBufferOffset != INVALID::U64ID) {
        auto UiShader = ShaderSystem::GetShader("Shader.StandardUI");  // ЗАДАЧА: шейдер текста.
        if (!renderer->ShaderReleaseInstanceResources(UiShader, InstanceID)) {
            MFATAL("Невозможно освободить ресурсы шейдера для карты текстуры шрифта.");
        }
        IndexBufferOffset = INVALID::U64ID;
    }

    // Освободить ресурсы, используемые для текстурной карты шрифта.
    Shader* UI_Shader = ShaderSystem::GetShader("Shader.StandardUI");  // ЗАДАЧА: шейдер текста.
    if (!renderer->ShaderReleaseInstanceResources(UI_Shader, InstanceID)) {
        MFATAL("Не удалось освободить шейдерные ресурсы для текстурной карты шрифта.");
    }
    InstanceID = INVALID::ID;
}

void Label::RenderPrepare(const FrameData &rFrameData)
{
    if (IsDirty) {
        RegenerateGeometry();
        
        u64 OldVertexSize = VertexBufferSize;
        u64 OldVertexOffset = VertexBufferOffset;
        u64 OldIndexSize = IndexBufferSize;
        u64 OldIndexOffset = IndexBufferOffset;
        
        // Используйте новые смещения, если только не требуется перераспределение памяти.
        u64 NewVertexSize = pendingData.VertexBufferSize;
        u64 NewVertexOffset = OldVertexOffset;
        u64 NewIndexSize = pendingData.IndexBufferSize;
        u64 NewIndexOffset = OldIndexOffset;
        
        // Перераспределение памяти необходимо, если текст стал длиннее, чем был прежде.
        auto renderer = SystemsManager::GetRenderingSystem();
        RenderBuffer* VertexBuffer = renderer->GetRenderbuffer(RenderBufferType::Vertex);
        RenderBuffer* IndexBuffer  = renderer->GetRenderbuffer(RenderBufferType::Index);
        bool NeedsRealloc = pendingData.NewUTF8Length > MaxTextLength;
        if (NeedsRealloc) {
            if (!VertexBuffer->Allocate(NewVertexSize, pendingData.VertexBufferOffset)) {
                MERROR("Label::RenderPrepare: не удалось выделить память из вершинного буфера визуализатора: размер=%u, смещение=%u",
                    NewVertexSize,
                    pendingData.VertexBufferOffset);
                return;
            }
            NewVertexOffset = pendingData.VertexBufferOffset;

            if (!IndexBuffer->Allocate(NewIndexSize, pendingData.IndexBufferOffset)) {
                MERROR("Label::RenderPrepare: не удалось выделить память из индексного буфера рендерера: size=%u, offset=%u",
                    NewIndexSize,
                    pendingData.IndexBufferOffset);
                return;
            }
            NewIndexOffset = pendingData.IndexBufferOffset;
        }

        // Загрузите данные, если есть что загружать.
        if (pendingData.VertexBufferData) {
            if (!renderer->RenderBufferLoadRange(*VertexBuffer, NewVertexOffset, NewVertexSize, pendingData.VertexBufferData, true)) {
                MERROR("Label::RenderPrepare: не удалось загрузить данные в диапазон вершинного буфера: размер=%u, смещение=%u", NewVertexSize, NewVertexOffset);
            }
        }

        if (pendingData.IndexBufferData) {
            if (!renderer->RenderBufferLoadRange(*IndexBuffer, NewIndexOffset, NewIndexSize, pendingData.IndexBufferData, true)) {
                MERROR("Label::RenderPrepare: не удалось загрузить данные в диапазон индексного буфера: size=%u, offset=%u", NewIndexSize, NewIndexOffset);
            }
        }

        if (NeedsRealloc) {
            // Освободите старые данные вершин/индексов из буферов и обновите размеры/смещения.
            if (OldVertexOffset != INVALID::U64ID && OldVertexSize != INVALID::U64ID) {
                if (!VertexBuffer->Free(OldVertexSize, OldVertexOffset)) {
                    MERROR("Не удалось освободить память в вершинном буфере рендерера: размер=%u, смещение=%u", OldVertexSize, OldVertexOffset);
                }
            }

            // Удалите старые данные из буфера индексов.
            if (OldIndexOffset != INVALID::U64ID && OldIndexSize != INVALID::U64ID) {
                if (!IndexBuffer->Free(OldIndexSize, OldIndexOffset)) {
                    MERROR("Не удалось освободить память в индексном буфере рендерера: размер=%u, смещение=%u", OldIndexSize, OldIndexOffset);
                }
            }

            // Обновите смещения буфера, используя ожидающие применения значения.
            VertexBufferOffset = NewVertexOffset;
            VertexBufferSize = NewVertexSize;
            IndexBufferOffset = NewIndexOffset;
            IndexBufferSize = NewIndexSize;
        }

        // Обновите максимальную длину, если строка теперь стала длиннее.
        if (pendingData.NewUTF8Length > MaxTextLength) {
            MaxTextLength = pendingData.NewUTF8Length;
        }

        pendingData.Reset();

        IsDirty = false;
    }
}

bool Label::Render(FrameData &rFrameData, UiRenderData &RenderData)
{
    const u32 tLength = text.Length();
    if (tLength && VertexBufferOffset != INVALID::U64ID) {
        UiRenderable* renderable = RenderData.renderables.PushBack();
        MemorySystem::ZeroMem(renderable, sizeof(UiRenderable));
        renderable->InstanceID = &InstanceID;
        renderable->FrameNumber = &FrameNumber;
        renderable->DrawIndex = &DrawIndex;
        // ПРИМЕЧАНИЕ: Переопределите атлас пользовательского интерфейса по умолчанию и используйте вместо него загруженный шрифт.
        renderable->AtlasOverride = &data->atlas;

        GeometryRenderData& grd = renderable->RenderData;
        grd.UniqueID            = id.UniqueID;
        grd.VertexCount         = tLength * 4;
        grd.VertexBufferOffset  = VertexBufferOffset;
        grd.VertexElementSize   = sizeof(Vertex2D);
        grd.IndexCount          = tLength * 6;
        grd.IndexBufferOffset   = IndexBufferOffset;
        grd.IndexElementSize    = sizeof(u32);

        grd.model = xform.GetWorld();
        grd.DiffuseColour = colour;
    }

    return true;
}

void Label::RemovePart(u32 position, u32 length)
{
    text.RemovePart(position, length);
    RegenerateGeometry();
}

void Label::SetText(const char *text, u32 size)
{
    // Если строки уже равны, ничего не делайте.
    if (this->text == text) {
        return;
    }

    this->text = text;

    // Убедитесь, что в атласе есть необходимые глифы.
    if (!FontSystem::VerifyAtlas(data, this->text)) {
        MERROR("Проверка атласа шрифтов не удалась.");
    }

    IsDirty = true;
}

void Label::SetText(MString &text, bool copy)
{
    // Если строки уже равны, ничего не делайте.
    if (this->text == text) {

    } else if (copy) {
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
    u32 TextLengthUTF8 = text.Length();
    // Также получите длину в символах.
    u32 CharLength = text.Size();
    CharLength = CharLength > 0 ? CharLength - 1 : CharLength;

    // Не пытайтесь перегенерировать геометрию для объекта, в котором нет текста.
    if (TextLengthUTF8 < 1) {
        return;
    }

    // Рассчитайте размеры буферов.
    static const u64 VertsPerQuad = 4;
    static const u8 IndicesPerQuad = 6;

    // Сохраните данные в промежуточную структуру.
    // pendingData.PrevVertexBufferSize = sizeof(Vertex2D) * VertsPerQuad * MaxTextLength;
    // pendingData.PrevIndexBufferSize = sizeof(u32) * IndicesPerQuad * MaxTextLength;
    pendingData.NewLength = CharLength;
    pendingData.NewUTF8Length = TextLengthUTF8;
    pendingData.VertexBufferSize = sizeof(Vertex2D) * VertsPerQuad * TextLengthUTF8;
    pendingData.IndexBufferSize = sizeof(u32) * IndicesPerQuad * TextLengthUTF8;
    // Временные массивы для хранения данных вершин/индексов.
    CreateArray(pendingData.VertexBufferData, pendingData.VertexBufferSize, true);
    CreateArray(pendingData.IndexBufferData, pendingData.IndexBufferSize, true);

    // Сгенерировать новую геометрию для каждого символа.
    f32 x = 0;
    f32 y = 0;

    const char* text = this->text.c_str();
    // Извлечь длину в символах и получить из неё правильный код.
    for (u32 c = 0, uc = 0; c < CharLength && text[c]; ++c) {
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
        if (!MString::BytesToCodepoint(text, c, codepoint, advance)) {
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

            pendingData.VertexBufferData[(uc * 4) + 0] = p0;  // 0    3
            pendingData.VertexBufferData[(uc * 4) + 1] = p2;  //
            pendingData.VertexBufferData[(uc * 4) + 2] = p3;  //
            pendingData.VertexBufferData[(uc * 4) + 3] = p1;  // 2    1

            // Попробовать найти кернинг.
            i32 kerning = 0;

            // Получить смещение следующего символа. Если смещения нет, перейти на один символ вперёд, в противном случае использовать смещение как есть.
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            if (offset < TextLengthUTF8 - 1) {
                // Получить следующую кодовую точку.
                i32 NextCodepoint = 0;
                u8 AdvanceNext = 0;

                if (!MString::BytesToCodepoint(text, offset, NextCodepoint, AdvanceNext)) {
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
        pendingData.IndexBufferData[(uc * 6) + 0] = (uc * 4) + 2;
        pendingData.IndexBufferData[(uc * 6) + 1] = (uc * 4) + 1;
        pendingData.IndexBufferData[(uc * 6) + 2] = (uc * 4) + 0;
        pendingData.IndexBufferData[(uc * 6) + 3] = (uc * 4) + 3;
        pendingData.IndexBufferData[(uc * 6) + 4] = (uc * 4) + 0;
        pendingData.IndexBufferData[(uc * 6) + 5] = (uc * 4) + 1;

        // Теперь переходим к c
        c += advance - 1;  // Вычитаем 1, поскольку цикл всегда увеличивается на один символ для однобайтовых данных.
        // Увеличиваем количество символов UTF-8.
        uc++;
    }

    // Отметьте метку как «грязную». Эту работу следует перенести на следующий этап подготовки.
    IsDirty = true;
}

void Label::PendingData::Reset()
{
    // Удалите ожидающие данные, если они есть.
    if (VertexBufferData) {
        MASSERT_MSG(VertexBufferSize, "Предпринята попытка освободить ожидающие обработки вершинные данные, размер которых равен 0. Указатель на данные существует, но размер является недопустимым.");
        MASSERT_MSG(VertexBufferSize != INVALID::U64ID, "Предпринята попытка освободить ожидающие обработки вершинные данные, размер которых равен INVALID::U64ID. Указатель на данные существует, но размер является недопустимым.");

        DestroyArray(VertexBufferData, VertexBufferSize);
        VertexBufferData = nullptr;
    }
    if (IndexBufferData) {
        MASSERT_MSG(IndexBufferSize, "Предпринята попытка освободить ожидающие обработки индексные данные размером 0. Указатель на данные существует, но размер является недопустимым.");
        MASSERT_MSG(IndexBufferSize != INVALID::U64ID, "Предпринята попытка освободить ожидающие индексации данные, размер которых равен INVALID::U64ID. Указатель на данные существует, но значение размера является недопустимым.");

        DestroyArray(IndexBufferData, IndexBufferSize);
        IndexBufferData = nullptr;
    }
    VertexBufferOffset = VertexBufferSize = IndexBufferOffset = IndexBufferSize = INVALID::U64ID;
    NewLength = NewUTF8Length = 0;
}
