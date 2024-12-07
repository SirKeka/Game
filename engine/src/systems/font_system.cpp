#include "font_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "resources/font_resource.hpp"
#include "resources/ui_text.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/rendering_system.hpp"

#include <new>

// Для системных шрифтов.
#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"

struct BitmapFontInternalData {
    BitmapFontResource LoadedResource;
    // Для удобства приведен указатель на данные ресурса.
    BitmapFontResourceData* ResourceData;
    constexpr BitmapFontInternalData() : LoadedResource(), ResourceData(nullptr) {}
};

struct SystemFontVariantData {
    DArray<i32> codepoints;
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

bool SetupFontData(FontData& font);
void CleanupFontData(FontData& font);
bool CreateSystemFontVariant(SystemFontLookup& lookup, u16 size, const char* FontName, FontData& OutVariant);
bool RebuildSystemFontVariantAtlas(SystemFontLookup& lookup, FontData& variant);
bool VerifySystemFontSizeVariant(SystemFontLookup& lookup, FontData* variant, const char* text);

FontSystem* FontSystem::state;

bool FontSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<FontSystemConfig*>(config);
    if (pConfig->MaxBitmapFontCount == 0 || pConfig->MaxSystemFontCount == 0) {
        MFATAL("FontSystems::Initialize - config.MaxBitmapFontCount и config.MaxSystemFontCount должны быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блоки для массивов, затем блоки для хэш-таблиц.
    u64 StructRequirement = sizeof(FontSystem);
    u64 BmpArrayRequirement = sizeof(BitmapFontLookup) * pConfig->MaxBitmapFontCount;
    u64 SysArrayRequirement = sizeof(SystemFontLookup) * pConfig->MaxSystemFontCount;
    u64 BmpHashTableRequirement = sizeof(u16) * pConfig->MaxBitmapFontCount;
    u64 SysHashTableRequirement = sizeof(u16) * pConfig->MaxSystemFontCount;
    MemoryRequirement = StructRequirement + BmpArrayRequirement + SysArrayRequirement + BmpHashTableRequirement + SysHashTableRequirement;

    if (!memory) {
        return true;
    }

    u8* MemBlock = reinterpret_cast<u8*>(memory);

    // Блоки массивов находятся после состояния. Уже выделены, поэтому просто установите указатель.
    auto BmpArrayBlock = new(MemBlock + StructRequirement) BitmapFontLookup[pConfig->MaxBitmapFontCount];
    auto SysArrayBlock = new(BmpArrayBlock + pConfig->MaxBitmapFontCount) SystemFontLookup[pConfig->MaxSystemFontCount];

    // Блоки хэш-таблиц находятся после массивов.
    u16* BmpHashtableBlock = reinterpret_cast<u16*>(SysArrayBlock + pConfig->MaxSystemFontCount);
    u16* SysHashtableBlock = BmpHashtableBlock + pConfig->MaxBitmapFontCount;

    state = new(MemBlock) FontSystem(*pConfig, BmpArrayBlock, SysArrayBlock, BmpHashtableBlock, SysHashtableBlock);

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

    return true;
}

void FontSystem::Shutdown()
{
    if (state) {
        // Очистка растровых шрифтов.
        for (u16 i = 0; i < state->config.MaxBitmapFontCount; ++i) {
            if (state->BitmapFonts[i].id != INVALID::U16ID) {
                auto& data = state->BitmapFonts[i].font.ResourceData->data;
                CleanupFontData(data);
                state->BitmapFonts[i].id = INVALID::U16ID;
            }
        }

        // Очистка системных шрифтов.
        for (u16 i = 0; i < state->config.MaxSystemFontCount; ++i) {
            if (state->SystemFonts[i].id != INVALID::U16ID) {
                // Очистка каждого варианта.
                const auto& VariantCount = state->SystemFonts[i].SizeVariants.Length();
                for (u32 j = 0; j < VariantCount; ++j) {
                    auto& data = state->SystemFonts[i].SizeVariants[j];
                    CleanupFontData(data);
                }
                state->SystemFonts[i].id = INVALID::U16ID;

                // state->SystemFonts[i].SizeVariants.Clear();
            }
        }
    }
}

bool FontSystem::LoadFont(SystemFontConfig &config)
{
    // Для системных шрифтов они фактически могут содержать несколько шрифтов. 
    // По этой причине копия данных ресурса будет храниться в каждом результирующем варианте, а сам ресурс будет освобожден.
    SystemFontResource LoadedResource;
    if (!ResourceSystem::Load(config.ResourceName.c_str(), eResource::Type::SystemFont, nullptr, LoadedResource)) {
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
        lookup.offset = stbtt_GetFontOffsetForIndex(reinterpret_cast<u8*>(lookup.FontBinary), i);
        i32 result = stbtt_InitFont(&lookup.info, reinterpret_cast<u8*>(lookup.FontBinary), lookup.offset);
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
        lookup.SizeVariants.PushBack(static_cast<FontData&&>(variant));

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

    if (!ResourceSystem::Load(config.ResourceName.c_str(), eResource::Type::BitmapFont, nullptr, lookup.font.LoadedResource)) {
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

bool FontSystem::Acquire(const char *FontName, u16 FontSize, Text &text)
{
    if (text.type == TextType::Bitmap) {
        u16 id = INVALID::U16ID;
        if (!state->bitmapFontLookup.Get(FontName, &id)) {
            MERROR("Поиск растрового шрифта не удался при получении.");
            return false;
        }

        if (id == INVALID::U16ID) {
            MERROR("Не найден растровый шрифт с именем '%s'. Получение шрифта не удалось.", FontName);
            return false;
        }

        // Получить поиск.
       auto& lookup = state->BitmapFonts[id];

        // Назначить данные, увеличить ссылку.
        text.data = &lookup.font.ResourceData->data;
        lookup.ReferenceCount++;

        return true;
    } else if (text.type == TextType::System) {
        u16 id = INVALID::U16ID;
        if (!state->systemFontLookup.Get(FontName, &id)) {
            MERROR("Поиск системного шрифта не удался при получении.");
            return false;
        }

        if (id == INVALID::U16ID) {
            MERROR("Системный шрифт с именем '%s' не найден. Получение шрифта не удалось.", FontName);
            return false;
        }

        // Получить поиск.
        auto& lookup = state->SystemFonts[id];

        // Поиск вариантов размера для правильного размера.
        const auto& count = lookup.SizeVariants.Length();
        for (u32 i = 0; i < count; ++i) {
            if (lookup.SizeVariants[i].size == FontSize) {
                // Назначить данные, увеличить ссылку.
                text.data = &lookup.SizeVariants[i];
                lookup.ReferenceCount++;
                return true;
            }
        }

        // Если мы достигли этой точки, вариант размера не существует. Создать его.
        FontData variant;
        if (!CreateSystemFontVariant(lookup, FontSize, FontName, variant)) {
            MERROR("Не удалось создать вариант: %s, индекс %i, размер %i", lookup.face.c_str(), lookup.index, FontSize);
            return false;
        }

        // Также выполнить настройку для варианта
        if (!SetupFontData(variant)) {
            MERROR("Не удалось настроить данные шрифта");
        }

        // Добавить к вариантам размера поиска.
        lookup.SizeVariants.PushBack(static_cast<FontData&&>(variant));
        const auto& length = lookup.SizeVariants.Length();
        // Назначить данные, увеличить ссылку.
        text.data = &lookup.SizeVariants[length - 1];
        lookup.ReferenceCount++;
        return true;
    }

    MERROR("Нераспознанный тип шрифта: %d", text.type);
    return false;
}

bool FontSystem::Release(Text &text)
{
    // ЗАДАЧА: Поиск шрифта по имени в соответствующей хеш-таблице.
    return true;
}

bool FontSystem::VerifyAtlas(FontData *font, const char *text)
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

bool SetupFontData(FontData &font)
{
    // Создать ресурсы карты
    font.atlas.FilterMagnify = font.atlas.FilterMinify = TextureFilter::ModeLinear;
    font.atlas.RepeatU = font.atlas.RepeatV = font.atlas.RepeatW = TextureRepeat::ClampToEdge;
    font.atlas.use = TextureUse::MapDiffuse;
    if (!RenderingSystem::TextureMapAcquireResources(&font.atlas)) {
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

void CleanupFontData(FontData &font)
{
    // Освободите ресурсы карты текстуры.
    RenderingSystem::TextureMapReleaseResources(&font.atlas);

    // Если это растровый шрифт, освободите ссылку на текстуру.
    if (font.type == FontType::Bitmap && font.atlas.texture) {
        TextureSystem::Release(font.atlas.texture->name);
    }
    font.atlas.texture = nullptr;
}

bool CreateSystemFontVariant(SystemFontLookup &lookup, u16 size, const char *FontName, FontData &OutVariant)
{
    OutVariant.AtlasSizeX = 1024;  // ЗАДАЧА: настраевыемый размер
    OutVariant.AtlasSizeY = 1024;
    OutVariant.size = size;
    OutVariant.type = FontType::System;
    MString::Copy(OutVariant.face, FontName, 255);
    OutVariant.InternalDataSize = sizeof(SystemFontVariantData);
    OutVariant.InternalData = MemorySystem::Allocate(OutVariant.InternalDataSize, Memory::SystemFont, true);

    auto InternalData = reinterpret_cast<SystemFontVariantData*>(OutVariant.InternalData);

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
    OutVariant.atlas.texture = TextureSystem::AquireWriteable(FontTexName, OutVariant.AtlasSizeX, OutVariant.AtlasSizeY, 4, true);

    // Получить некоторые метрики
    InternalData->scale = stbtt_ScaleForPixelHeight(&lookup.info, (f32)size);
    i32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&lookup.info, &ascent, &descent, &line_gap);
    OutVariant.LineHeight = (ascent - descent + line_gap) * InternalData->scale;

    return RebuildSystemFontVariantAtlas(lookup, OutVariant);
}

bool RebuildSystemFontVariantAtlas(SystemFontLookup &lookup, FontData &variant)
{
    auto InternalData = reinterpret_cast<SystemFontVariantData*>(variant.InternalData);

    u32 PackImageSize = variant.AtlasSizeX * variant.AtlasSizeY * sizeof(u8);
    u8* pixels = MemorySystem::TAllocate<u8>(Memory::Array, PackImageSize);
    const auto& CodepointCount = InternalData->codepoints.Length();
    auto PackedChars = MemorySystem::TAllocate<stbtt_packedchar>(Memory::Array, CodepointCount);

    // Начните упаковывать все известные символы в атлас. 
    // Это создаст одноканальное изображение с визуализированными глифами заданного размера.
    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, pixels, variant.AtlasSizeX, variant.AtlasSizeY, 0, 1, 0)) {
        MERROR("stbtt_PackBegin failed");
        return false;
    }

    // Поместите все кодовые точки в один диапазон для упаковки.
    stbtt_pack_range range;
    range.first_unicode_codepoint_in_range = 0;
    range.font_size = variant.size;
    range.num_chars = CodepointCount;
    range.chardata_for_range = PackedChars;
    range.array_of_unicode_codepoints = InternalData->codepoints.Data();
    if (!stbtt_PackFontRanges(&context, reinterpret_cast<u8*>(lookup.FontBinary), lookup.index, &range, 1)) {
        MERROR("stbtt_PackFontRanges failed");
        return false;
    }

    stbtt_PackEnd(&context);
    // Упаковка завершена.

    // Конвертируйте из одноканального в RGBA или pack_image_size * 4.
    u8* RgbaPixels = MemorySystem::TAllocate<u8>(Memory::Array, PackImageSize * 4);
    for (i32 j = 0; j < PackImageSize; ++j) {
        RgbaPixels[(j * 4) + 0] = pixels[j];
        RgbaPixels[(j * 4) + 1] = pixels[j];
        RgbaPixels[(j * 4) + 2] = pixels[j];
        RgbaPixels[(j * 4) + 3] = pixels[j];
    }

    // Запишите данные текстуры в атлас.
    TextureSystem::WriteData(variant.atlas.texture, 0, PackImageSize * 4, RgbaPixels);

    // Освободите данные пикселей/rgba_pixel.
    MemorySystem::Free(pixels, PackImageSize, Memory::Array);
    MemorySystem::Free(RgbaPixels, PackImageSize * 4, Memory::Array);

    // Регенерируйте глифы
    if (variant.glyphs && variant.GlyphCount) {
        MemorySystem::Free(variant.glyphs, sizeof(FontGlyph) * variant.GlyphCount, Memory::Array);
    }
    variant.GlyphCount = CodepointCount;
    variant.glyphs = MemorySystem::TAllocate<FontGlyph>(Memory::Array, CodepointCount);
    for (u16 i = 0; i < variant.GlyphCount; ++i) {
        stbtt_packedchar* pc = &PackedChars[i];
        auto& g = variant.glyphs[i];
        g.codepoint = InternalData->codepoints[i];
        g.PageID = 0;
        g.xOffset = pc->xoff;
        g.yOffset = pc->yoff;
        g.x = pc->x0;  // xmin;
        g.y = pc->y0;
        g.width = pc->x1 - pc->x0;
        g.height = pc->y1 - pc->y0;
        g.xAdvance = pc->xadvance;
    }

    // Регенерируйте кернинги
    if (variant.kernings && variant.KerningCount) {
        MemorySystem::Free(variant.kernings, sizeof(FontKerning) * variant.KerningCount, Memory::Array);
    }
    variant.KerningCount= stbtt_GetKerningTableLength(&lookup.info);
    if (variant.KerningCount) {
        variant.kernings = MemorySystem::TAllocate<FontKerning>(Memory::Array, variant.KerningCount);
        // Получите таблицу кернинга для текущего шрифта.
        stbtt_kerningentry* KerningTable = MemorySystem::TAllocate<stbtt_kerningentry>(Memory::Array, variant.KerningCount);
        i32 EntryCount = stbtt_GetKerningTable(&lookup.info, KerningTable, variant.KerningCount);
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
    } else {
        variant.kernings = nullptr;
    }

    return true;
}

bool VerifySystemFontSizeVariant(SystemFontLookup &lookup, FontData *variant, const char *text)
{
    auto InternalData = reinterpret_cast<SystemFontVariantData*>(variant->InternalData);

    u32 CharLength = MString::Length(text);
    u32 AddedCodepointCount = 0;
    for (u32 i = 0; i < CharLength;) {
        i32 codepoint;
        u8 advance;
        if (!MString::BytesToCodepoint(text, i, codepoint, advance)) {
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
            const auto& CodepointCount = InternalData->codepoints.Length();
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
