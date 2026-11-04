#include "font_system.h"
#include "core/systems_manager.h"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "resources/font_resource.hpp"
#include "resources/ui_text.h"
#include "memory/linear_allocator.h"
#include "renderer/rendering_system.h"

#include <new>

// Для системных шрифтов.
#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"

struct BitmapFontInternalData {
    BitmapFontResource LoadedResource;
    // Для удобства приведен указатель на данные ресурса.
    BitmapFontResourceData* ResourceData{};
};

struct SystemFontVariantData {
    DArray<i32> codepoints{};
    f32 scale;
};

struct BitmapFontLookup {
    u16 id                     {};
    u16 ReferenceCount         {};
    BitmapFontInternalData font{};
    constexpr BitmapFontLookup() : id(INVALID::U16ID), ReferenceCount(), font() {}
};

struct SystemFontLookup {
    u16 id                       {};
    u16 ReferenceCount           {};
    DArray<FontData> SizeVariants{};
    // Для удобства у каждого сохраняется копия всего этого.
    u64 BinarySize               {};
    MString face                 {};
    void* FontBinary      {nullptr}; // ЗАДАЧА: изменить тип на u8*?
    i32 offset                   {};
    i32 index                    {};
    stbtt_fontinfo info          {};
    /*constexpr*/ SystemFontLookup() : id(INVALID::U16ID), ReferenceCount(), SizeVariants(), BinarySize(), face(), FontBinary(nullptr), offset(), index(), info() {}
};

static bool SetupFontData(FontData& font);
static void CleanupFontData(FontData& font);
static bool CreateSystemFontVariant(SystemFontLookup& lookup, u16 size, const char* FontName, FontData& OutVariant);
static bool RebuildSystemFontVariantAtlas(SystemFontLookup& lookup, FontData& variant);
static bool VerifySystemFontSizeVariant(SystemFontLookup& lookup, FontData* variant, const MString& text);

FontSystem* FontSystem::Initialize(u64& MemoryRequirement, void* memory, Config& config)
{
    if (config.MaxBitmapFontCount == 0 || config.MaxSystemFontCount == 0) {
        MFATAL("FontSystems::Initialize - config.MaxBitmapFontCount и config.MaxSystemFontCount должны быть > 0.");
        return nullptr;
    }

    // Блок памяти будет содержать структуру состояния, затем блоки для массивов, затем блоки для хэш-таблиц.
    u64 StructRequirement = sizeof(FontSystem);
    u64 BmpArrayRequirement = sizeof(BitmapFontLookup) * config.MaxBitmapFontCount;
    u64 SysArrayRequirement = sizeof(SystemFontLookup) * config.MaxSystemFontCount;
    u64 BmpHashTableRequirement = sizeof(u16) * config.MaxBitmapFontCount;
    u64 SysHashTableRequirement = sizeof(u16) * config.MaxSystemFontCount;
    MemoryRequirement = StructRequirement + BmpArrayRequirement + SysArrayRequirement + BmpHashTableRequirement + SysHashTableRequirement;

    if (!memory) {
        return nullptr;
    }

    u8* MemBlock = reinterpret_cast<u8*>(memory);

    // Блоки массивов находятся после состояния. Уже выделены, поэтому просто установите указатель.
    auto BmpArrayBlock = new(MemBlock + StructRequirement) BitmapFontLookup[config.MaxBitmapFontCount];
    auto SysArrayBlock = new(BmpArrayBlock + config.MaxBitmapFontCount) SystemFontLookup[config.MaxSystemFontCount];

    // Блоки хэш-таблиц находятся после массивов.
    u16* BmpHashtableBlock = reinterpret_cast<u16*>(SysArrayBlock + config.MaxSystemFontCount);
    u16* SysHashtableBlock = BmpHashtableBlock + config.MaxBitmapFontCount;

    state = new(MemBlock) FontSystem(config, BmpArrayBlock, SysArrayBlock, BmpHashtableBlock, SysHashtableBlock);

    // Загрузите все шрифты по умолчанию.
    // Растровые шрифты.
    for (u32 i = 0; i < state->config.DefaultBitmapFontCount; ++i) {
        if (!LoadFont(state->config.BitmapFontConfigs[i])) {
            MERROR("Не удалось загрузить растровый шрифт: %s", state->config.BitmapFontConfigs[i].name.c_str());
        }
    }

    for (u32 i = 0; i < state->config.DefaultSystemFontCount; i++) {
        if (!LoadFont(state->config.SystemFontConfigs[i])) {
            MERROR("Не удалось загрузить растровый шрифт: %s", state->config.SystemFontConfigs[i].name.c_str());
        }
    }

    return state;
}

void FontSystem::Shutdown()
{
    // Очистка растровых шрифтов.
    for (u16 i = 0; i < config.MaxBitmapFontCount; ++i) {
        if (BitmapFonts[i].id != INVALID::U16ID) {
            auto& data = BitmapFonts[i].font.ResourceData->data;
            CleanupFontData(data);
            BitmapFonts[i].id = INVALID::U16ID;
        }
    }

    // Очистка системных шрифтов.
    for (u16 i = 0; i < config.MaxSystemFontCount; ++i) {
        if (SystemFonts[i].id != INVALID::U16ID) {
            // Очистка каждого варианта.
            const u32 VariantCount = SystemFonts[i].SizeVariants.Length();
            for (u32 j = 0; j < VariantCount; ++j) {
                auto& data = SystemFonts[i].SizeVariants[j];
                CleanupFontData(data);
            }
            SystemFonts[i].id = INVALID::U16ID;

            // SystemFonts[i].SizeVariants.Clear();
        }
    }
}

bool FontSystem::LoadFont(SystemFontConfig &config)
{
    // Для системных шрифтов они фактически могут содержать несколько шрифтов. 
    // По этой причине копия данных ресурса будет храниться в каждом результирующем варианте, а сам ресурс будет освобожден.
    SystemFontResource LoadedResource;
    if (!ResourceSystem::Load(config.ResourceName.c_str(), nullptr, LoadedResource)) {
        MERROR("Не удалось загрузить системный шрифт.");
        return false;
    }

    // Сохраните приведенный указатель на данные ресурса для удобства.
    auto& ResourceData = LoadedResource.data;

    // Пройдитесь по начертаниям и создайте один поиск для каждого, а также вариант размера по умолчанию для каждого поиска.
    u32 FontFaceCount = ResourceData.fonts.Length();
    for (u32 i = 0; i < FontFaceCount; ++i) {
        auto& face = ResourceData.fonts[i];

        // Убедитесь, что шрифт с таким именем еще не существует.
        u16 id = INVALID::U16ID;
        if (!state->systemFontLookup.Get(face.name, &id)) {
            MERROR("Поиск в хэш-таблице не удался. Шрифт не будет загружен.");
            return false;
        }
        if (id != INVALID::U16ID) {
            MWARN("Шрифт с именем '%s' уже существует и не будет загружен снова.", config.name.c_str());
            // Не серьезная ошибка, верните успешное выполнение, так как он уже существует и может быть использован.
            return true;
        }

        // Получить новый идентификатор
        for (u16 j = 0; j < state->config.MaxSystemFontCount; ++j) {
            if (state->SystemFonts[j].id == INVALID::U16ID) {
                id = j;
                break;
            }
        }
        if (id == INVALID::U16ID) {
            MERROR("Не осталось места для выделения нового шрифта. Увеличить максимальное число, разрешенное в конфигурации системы шрифтов.");
            return false;
        }

        // Получить поиск.
        auto& lookup = state->SystemFonts[id];
        lookup.BinarySize = ResourceData.BinarySize;
        lookup.FontBinary = ResourceData.FontBinary;
        lookup.face = face.name;
        lookup.index = i;

        // Смещение
        lookup.offset = stbtt_GetFontOffsetForIndex((u8*)lookup.FontBinary, i);
        i32 result = stbtt_InitFont(&lookup.info, (u8*)lookup.FontBinary, lookup.offset);
        if (result == 0) {
            // Ноль указывает на сбой.
            MERROR("Не удалось инициализировать системный шрифт %s по индексу %i.", LoadedResource.FullPath.c_str(), i);
            return false;
        }

        // Создать вариант размера по умолчанию.
        FontData variant;
        if (!CreateSystemFontVariant(lookup, config.DefaultSize, face.name, variant)) {
            MERROR("Не удалось создать вариант: %s, индекс %i", face.name, i);
            continue;
        }

        // Также выполнить настройку для варианта
        if (!SetupFontData(variant)) {
            MERROR("Не удалось настроить данные шрифта");
            continue;
        }

        // Добавить к вариантам размера поиска.
        lookup.SizeVariants.PushBack((FontData&&)variant);

        // Установите идентификатор записи здесь в последнюю очередь перед обновлением хеш-таблицы.
        lookup.id = id;
        if (!state->systemFontLookup.Set(face.name, id)) {
            MERROR("Сбой установки хеш-таблицы при загрузке шрифта.");
            return false;
        }
    }

    return true;
}

bool FontSystem::LoadFont(BitmapFontConfig &config)
{
    // Убедитесь, что шрифт с таким именем еще не существует.
    u16 id = INVALID::U16ID;
    if (!state->bitmapFontLookup.Get(config.name.c_str(), &id)) {
        MERROR("Поиск в хэш-таблице не удался. Шрифт не будет загружен.");
        return false;
    }
    if (id != INVALID::U16ID) {
        MWARN("Шрифт с именем '%s' уже существует и не будет загружен снова.", config.name.c_str());
        // Не серьезная ошибка, верните успех, так как он уже существует и может быть использован.
        return true;
    }

    // Получить новый идентификатор
    for (u16 i = 0; i < state->config.MaxBitmapFontCount; ++i) {
        if (state->BitmapFonts[i].id == INVALID::U16ID) {
            id = i;
            break;
        }
    }
    if (id == INVALID::U16ID) {
        MERROR("Не осталось места для размещения нового растрового шрифта. Увеличьте максимальное количество, разрешенное в конфигурации системы шрифтов.");
        return false;
    }

    // Получить поиск.
   auto& lookup = state->BitmapFonts[id];

    if (!ResourceSystem::Load(config.ResourceName.c_str(), nullptr, lookup.font.LoadedResource)) {
        MERROR("Не удалось загрузить растровый шрифт.");
        return false;
    }

    // Сохраните приведенный указатель на данные ресурса для удобства.
    lookup.font.ResourceData = &lookup.font.LoadedResource.data;

    // Получить текстуру.
    // ЗАДАЧА: на данный момент занимает только одну страницу.
    lookup.font.ResourceData->data.atlas.texture = TextureSystem::Acquire(lookup.font.ResourceData->pages[0].file, true);

    bool result = SetupFontData(lookup.font.ResourceData->data);

    // Установите идентификатор записи здесь в последнюю очередь перед обновлением хэш-таблицы.
    if (!state->bitmapFontLookup.Set(config.name.c_str(), id)) {
        MERROR("Сбой установки хэш-таблицы при загрузке шрифта.");
        return false;
    }

    lookup.id = id;

    return result;
}

FontData* FontSystem::Acquire(const char *FontName, u16 FontSize, FontType type)
{
    if (type == FontType::Bitmap) {
        u16 id = INVALID::U16ID;
        if (!state->bitmapFontLookup.Get(FontName, &id)) {
            MERROR("Поиск растрового шрифта не удался при получении.");
            return nullptr;
        }

        if (id == INVALID::U16ID) {
            MERROR("Не найден растровый шрифт с именем '%s'. Получение шрифта не удалось.", FontName);
            return nullptr;
        }

        // Получить поиск.
       auto& lookup = state->BitmapFonts[id];

        // Увеличить ссылку.
        lookup.ReferenceCount++;

        return &lookup.font.ResourceData->data;
    } else if (type == FontType::System) {
        u16 id = INVALID::U16ID;
        if (!state->systemFontLookup.Get(FontName, &id)) {
            MERROR("Поиск системного шрифта не удался при получении.");
            return nullptr;
        }

        if (id == INVALID::U16ID) {
            MERROR("Системный шрифт с именем '%s' не найден. Получение шрифта не удалось.", FontName);
            return nullptr;
        }

        // Получить поиск.
        auto& lookup = state->SystemFonts[id];

        // Поиск вариантов размера для правильного размера.
        const u32 count = lookup.SizeVariants.Length();
        for (u32 i = 0; i < count; ++i) {
            if (lookup.SizeVariants[i].size == FontSize) {
                // Увеличить ссылку.
                lookup.ReferenceCount++;
                return &lookup.SizeVariants[i];
            }
        }

        // Если мы достигли этой точки, вариант размера не существует. Создать его.
        FontData variant;
        if (!CreateSystemFontVariant(lookup, FontSize, FontName, variant)) {
            MERROR("Не удалось создать вариант: %s, индекс %i, размер %i", lookup.face.c_str(), lookup.index, FontSize);
            return nullptr;
        }

        // Также выполнить настройку для варианта
        if (!SetupFontData(variant)) {
            MERROR("Не удалось настроить данные шрифта");
        }

        // Добавить к вариантам размера поиска.
        lookup.SizeVariants.PushBack(static_cast<FontData&&>(variant));
        const u32 length = lookup.SizeVariants.Length();
        // Увеличить ссылку.
        lookup.ReferenceCount++;
        return &lookup.SizeVariants[length - 1];
    }

    MERROR("Нераспознанный тип шрифта: %d", type);
    return nullptr;
}

bool FontSystem::Release(Text &text)
{
    // ЗАДАЧА: Поиск шрифта по имени в соответствующей хеш-таблице.
    return true;
}

bool FontSystem::VerifyAtlas(FontData *font, const MString &text)
{
    if (font->type == FontType::Bitmap) {
        // Растровые изображения не нуждаются в проверке, так как они уже сгенерированы.
        return true;

    } else if (font->type == FontType::System) {
        u16 id = INVALID::U16ID;
        if (!state->systemFontLookup.Get(font->face, &id)) {
            MERROR("Поиск системного шрифта не удался при получении.");
            return false;
        }

        if (id == INVALID::U16ID) {
            MERROR("Системный шрифт с именем '%s' не найден. Проверка атласа шрифтов не удалась.", font->face);
            return false;
        }

        //Получить поиск.
        auto& lookup = state->SystemFonts[id];

        return VerifySystemFontSizeVariant(lookup, font, text);
    }

    MERROR("FontSystem::VerifyAtlas не удалось: Неизвестный тип шрифта.");
    return false;
}

FVec2 FontSystem::MeasureString(FontData *font, const char *text, u32 CharLength, u32 TextLengthUTF8)
{
     if (!CharLength) {
        CharLength = MString::Length(text);
    }
    
    if (!TextLengthUTF8) {
        TextLengthUTF8 = MString::Length(text, true);
    }
    
    FVec2 extents{};

    f32 x = 0;
    f32 y = 0;

    // Возьмите длину в символах и получите из неё правильный код.
    for (u32 c = 0; c < CharLength; ++c) {
        i32 codepoint = text[c];

        // Перейдите на следующую строку для перехода на новую строку.
        if (codepoint == '\n') {
            if (x > extents.x) {
                extents.x = x;
            }
            x = 0;
            y += font->LineHeight;
            continue;
        }

        if (codepoint == '\t') {
            x += font->TabXAdvance;
            continue;
        }

        // ПРИМЕЧАНИЕ: Обработка кодовых точек UTF-8.
        u8 advance = 0;
        if (!MString::BytesToCodepoint(text, c, codepoint, advance)) {
            MWARN("В строке обнаружен недопустимый UTF-8, использующий неизвестный код -1");
            codepoint = -1;
        }

        FontGlyph* glyph = nullptr;
        for (u32 i = 0; i < font->GlyphCount; ++i) {
            if (font->glyphs[i].codepoint == codepoint) {
                glyph = &font->glyphs[i];
                break;
            }
        }

        if (!glyph) {
            // Если не найдено, используйте кодовую точку -1.
            codepoint = -1;
            for (u32 i = 0; i < font->GlyphCount; ++i) {
                if (font->glyphs[i].codepoint == codepoint) {
                    glyph = &font->glyphs[i];
                    break;
                }
            }
        }

        if (glyph) {
            // Попробуйте найти кернинг.
            i32 kerning = 0;

            // Получить смещение следующего символа. Если смещения нет, перейти на один символ вперёд, иначе использовать смещение как есть.
            u32 offset = c + advance;  //(advance < 1 ? 1 : advance);
            if (offset < TextLengthUTF8 - 1) {
                // Получить следующую кодовую точку.
                i32 NextCodepoint = 0;
                u8 AdvanceNext = 0;

                if (!MString::BytesToCodepoint(text, offset, NextCodepoint, AdvanceNext)) {
                    MWARN("В строке обнаружен недопустимый UTF-8, использующий неизвестный код -1");
                    codepoint = -1;
                } else {
                    for (u32 i = 0; i < font->KerningCount; ++i) {
                        auto& kern = font->kernings[i];
                        if (kern.Codepoint0 == codepoint && kern.Codepoint1 == NextCodepoint) {
                            kerning = kern.amount;
                        }
                    }
                }
            }

            x += glyph->xAdvance + kerning;
        } else {
            MERROR("Не удалось найти неизвестную кодовую точку. Пропуск.");
            continue;
        }

        // Теперь переходим к c
        c += advance - 1;  // Вычитаем 1, так как цикл всегда увеличивается на один символ для однобайтовых символов.
    }

    // Последняя проверка на отсутствие новых строк.
    if (x > extents.x) {
        extents.x = x;
    }

    // Поскольку y начинается с нуля, нам нужно добавить ещё один символ, чтобы сделать его с 1-й строкой.
    y += font->LineHeight;
    extents.y = y;

    return extents;
}

MAPI FVec2 FontSystem::MeasureString(FontData *font, Substring text)
{
    return MeasureString(font, text.data, text.size, text.length);
}

static bool SetupFontData(FontData &font)
{
    // Создать ресурсы карты
    font.atlas.FilterMagnify = font.atlas.FilterMinify = TextureFilterMode::Linear;
    font.atlas.RepeatU = font.atlas.RepeatV = font.atlas.RepeatW = TextureRepeat::ClampToEdge;
    if (!SystemsManager::GetRenderingSystem()->TextureMapAcquireResources(&font.atlas)) {
        MERROR("Невозможно получить ресурсы для карты текстур атласа шрифтов.");
        return false;
    }

    // Проверьте наличие символа табуляции, так как он не всегда может быть экспортирован. 
    // Если он есть, сохраните его xAdvance и просто используйте его. Если его нет, создайте его на основе пробела x4
    if (!font.TabXAdvance) {
        for (u32 i = 0; i < font.GlyphCount; ++i) {
            if (font.glyphs[i].codepoint == '\t') {
                font.TabXAdvance = font.glyphs[i].xAdvance;
                break;
            }
        }
        // Если его все еще нет, используйте пробел x 4.
        if (!font.TabXAdvance) {
            for (u32 i = 0; i < font.GlyphCount; ++i) {
                // Поиск пробела
                if (font.glyphs[i].codepoint == ' ') {
                    font.TabXAdvance = font.glyphs[i].xAdvance * 4;
                    break;
                }
            }
            if (!font.TabXAdvance) {
                // Если его _все еще_ нет, значит, пробела тоже не было, поэтому просто закодируйте что-нибудь, в данном случае размер шрифта * 4.
                font.TabXAdvance = font.size * 4;
            }
        }
    }

    return true;
}

static void CleanupFontData(FontData &font)
{
    // Освободите ресурсы карты текстуры.
    SystemsManager::GetRenderingSystem()->TextureMapReleaseResources(&font.atlas);

    // Если это растровый шрифт, освободите ссылку на текстуру.
    if (font.type == FontType::Bitmap && font.atlas.texture) {
        TextureSystem::Release(font.atlas.texture->name);
    }
    font.atlas.texture = nullptr;
}

static bool CreateSystemFontVariant(SystemFontLookup &lookup, u16 size, const char *FontName, FontData &OutVariant)
{
    OutVariant.AtlasSizeX = 1024;  // ЗАДАЧА: настраевыемый размер
    OutVariant.AtlasSizeY = 1024;
    OutVariant.size = size;
    OutVariant.type = FontType::System;
    MString::Copy(OutVariant.face, FontName, 255);
    OutVariant.InternalDataSize = sizeof(SystemFontVariantData);
    OutVariant.InternalData = MemorySystem::Allocate(OutVariant.InternalDataSize, Memory::SystemFont, true);

    auto InternalData = (SystemFontVariantData*)OutVariant.InternalData;

    // Всегда указывайте кодовые точки по умолчанию (ascii 32–127), плюс -1 для неизвестного.
    InternalData->codepoints.Reserve(96);
    InternalData->codepoints.PushBack(-1);  // push invalid char
    for (i32 i = 0; i < 95; ++i) {
        InternalData->codepoints.PushBack(i + 32);
    }
    //InternalData->codepoints.Resize(96);

    // Создать текстуру.
    char FontTexName[255];
    MString::Format(FontTexName, "__system_text_atlas_%s_i%i_sz%i__", FontName, lookup.index, size);

    TextureConfig config {
        .name            = FontTexName,
        .width           = (u32)OutVariant.AtlasSizeX,
        .height          = (u32)OutVariant.AtlasSizeY,
        .ChannelCount    = 4, 
        .HasTransparency = true
    };
    OutVariant.atlas.texture = TextureSystem::AquireWriteable(config);

    // Получить некоторые метрики
    InternalData->scale = stbtt_ScaleForPixelHeight(&lookup.info, (f32)size);
    i32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&lookup.info, &ascent, &descent, &line_gap);
    OutVariant.LineHeight = (ascent - descent + line_gap) * InternalData->scale;

    return RebuildSystemFontVariantAtlas(lookup, OutVariant);
}

static bool RebuildSystemFontVariantAtlas(SystemFontLookup &lookup, FontData &variant)
{
    auto InternalData = (SystemFontVariantData*)variant.InternalData;

    u32 PackImageSize = variant.AtlasSizeX * variant.AtlasSizeY * sizeof(u8);
    u8* pixels = AllocateTemp<u8>(PackImageSize, true);
    const u32 CodepointCount = InternalData->codepoints.Length();
    auto PackedChars = AllocateTemp<stbtt_packedchar>(sizeof(stbtt_packedchar) * CodepointCount, true);

    // Начните упаковывать все известные символы в атлас. 
    // Это создаст одноканальное изображение с визуализированными глифами заданного размера.
    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, pixels, variant.AtlasSizeX, variant.AtlasSizeY, 0, 1, 0)) {
        MERROR("stbtt_PackBegin failed");
        return false;
    }

    // Поместите все кодовые точки в один диапазон для упаковки.
    stbtt_pack_range range {
        .font_size = (f32)variant.size,
        .first_unicode_codepoint_in_range = 0,
        .array_of_unicode_codepoints = InternalData->codepoints.Data(),
        .num_chars = (i32)CodepointCount,
        .chardata_for_range = PackedChars
    };
    if (!stbtt_PackFontRanges(&context, (u8*)lookup.FontBinary, lookup.index, &range, 1)) {
        MERROR("stbtt_PackFontRanges failed");
        return false;
    }

    stbtt_PackEnd(&context);
    // Упаковка завершена.

    // Конвертируйте из одноканального в RGBA или pack_image_size * 4.
    u8* RgbaPixels = AllocateTemp<u8>(PackImageSize * 4, true);
    for (u32 j = 0; j < PackImageSize; ++j) {
        RgbaPixels[(j * 4) + 0] = pixels[j];
        RgbaPixels[(j * 4) + 1] = pixels[j];
        RgbaPixels[(j * 4) + 2] = pixels[j];
        RgbaPixels[(j * 4) + 3] = pixels[j];
    }

    // Запишите данные текстуры в атлас.
    TextureSystem::WriteData(variant.atlas.texture, 0, PackImageSize * 4, RgbaPixels);

    // Освободите данные пикселей/rgba_pixel.
    // DestroyArray(pixels, PackImageSize);
    // DestroyArray(RgbaPixels, PackImageSize * 4);

    // Регенерируйте глифы
    if (variant.glyphs && variant.GlyphCount) {
        DestroyArray(variant.glyphs, variant.GlyphCount);
    }
    variant.GlyphCount = CodepointCount;
    CreateArray(variant.glyphs, sizeof(FontGlyph) * CodepointCount, true);
    for (u16 i = 0; i < variant.GlyphCount; ++i) {
        auto& pc = PackedChars[i];
        auto& g = variant.glyphs[i];
        g.codepoint = InternalData->codepoints[i];
        g.PageID = 0;
        g.xOffset = pc.xoff;
        g.yOffset = pc.yoff;
        g.x = pc.x0;  // xmin;
        g.y = pc.y0;
        g.width = pc.x1 - pc.x0;
        g.height = pc.y1 - pc.y0;
        g.xAdvance = pc.xadvance;
    }

    // Регенерируйте кернинги
    if (variant.kernings && variant.KerningCount) {
        DestroyArray(variant.kernings, variant.KerningCount);
    }
    variant.KerningCount= stbtt_GetKerningTableLength(&lookup.info);
    if (variant.KerningCount) {
        CreateArray(variant.kernings, sizeof(FontKerning) * variant.KerningCount, true);
        // Получите таблицу кернинга для текущего шрифта.
        u64 ktSize = sizeof(stbtt_kerningentry) * variant.KerningCount;
        auto KerningTable = AllocateTemp<stbtt_kerningentry>(ktSize, true);
        u32 EntryCount = stbtt_GetKerningTable(&lookup.info, KerningTable, variant.KerningCount);
        if (EntryCount != variant.KerningCount) {
            MERROR("Несоответствие количества записей кернинга: %i->%i", EntryCount, variant.KerningCount);
            return false;
        }

        for (u32 i = 0; i < variant.KerningCount; ++i) {
            auto k = variant.kernings[i];
            k.Codepoint0 = KerningTable[i].glyph1;
            k.Codepoint1 = KerningTable[i].glyph2;
            k.amount = KerningTable[i].advance;
        }
        // DestroyArray(KerningTable, ktSize);
    } else {
        variant.kernings = nullptr;
    }

    return true;
}

static bool VerifySystemFontSizeVariant(SystemFontLookup &lookup, FontData *variant, const MString &text)
{
    auto InternalData = (SystemFontVariantData*)variant->InternalData;

    u32 CharLength = text.Size();
    u32 AddedCodepointCount = 0;
    const char* txt = text.c_str();
    
    for (u32 i = 0; i < CharLength && txt[i];) {
        i32 codepoint;
        u8 advance;
        if (!MString::BytesToCodepoint(txt, i, codepoint, advance)) {
            MERROR("MString::BytesToCodepoint не удалось получить кодовую точку.");
            ++i;
            continue;
        } else {
            // Проверьте, содержится ли уже кодовая точка. 
            // Обратите внимание, что кодовые точки ascii всегда включены, поэтому их проверку можно пропустить.
            i += advance;
            if (codepoint < 128) {
                continue;
            }
            const u32 CodepointCount = InternalData->codepoints.Length();
            bool found = false;
            for (u32 j = 95; j < CodepointCount; ++j) {
                if (InternalData->codepoints[j] == codepoint) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                InternalData->codepoints.PushBack(codepoint);
                AddedCodepointCount++;
            }
        }
    }

    // Если были добавлены кодовые точки, перестройте атлас. 
    if (AddedCodepointCount > 0) {
        return RebuildSystemFontVariantAtlas(lookup, *variant);
    }

    // В противном случае продолжайте как обычно.
    return true;
}
