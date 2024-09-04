#include "font_systems.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "resources/font_resource.hpp"
#include "resources/ui_text.hpp"
#include "containers/hashtable.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/renderer.hpp"

#include <new>

struct BitmapFontInternalData {
    Resource LoadedResource;
    // Для удобства приведен указатель на данные ресурса.
    BitmapFontResourceData* ResourceData;
    constexpr BitmapFontInternalData() : LoadedResource(), ResourceData(nullptr) {}
};

struct BitmapFontLookup {
    u16 id;
    u16 ReferenceCount;
    BitmapFontInternalData font;
    constexpr BitmapFontLookup() : id(INVALID::U16ID), ReferenceCount(), font() {}
};

struct FontSystem::State {
    FontSystemConfig config;
    HashTable<u16> bitmapFontLookup;
    BitmapFontLookup* BitmapFonts;
    void* BitmapHashTableBlock;
    constexpr State(FontSystemConfig config, BitmapFontLookup* BitmapFonts, u16* BitmapHashTableBlock)
    : config(config), 
    bitmapFontLookup(config.MaxBitmapFontCount,  false, BitmapHashTableBlock, true, INVALID::U16ID), // Создание хэш-таблицы для поиска шрифтов и заполнить недействительными индексами.
    BitmapFonts(BitmapFonts), BitmapHashTableBlock(BitmapHashTableBlock) {}
};

bool SetupFontData(FontData& font);
void CleanupFontData(FontData& font);

FontSystem::State* FontSystem::state;

constexpr FontSystem::FontSystem() {}

bool FontSystem::Initialize(FontSystemConfig& config)
{
    if (config.MaxBitmapFontCount == 0 || config.MaxSystemFontCount == 0) {
        MFATAL("FontSystems::Initialize - config.MaxBitmapFontCount и config.MaxSystemFontCount должны быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блоки для массивов, затем блоки для хэш-таблиц.
    u64 StructRequirement = sizeof(FontSystem::State);
    u64 BmpArrayRequirement = sizeof(BitmapFontLookup) * config.MaxBitmapFontCount;
    u64 BmpHashTableRequirement = sizeof(u16) * config.MaxBitmapFontCount;
    u64 MemoryRequirement = StructRequirement + BmpArrayRequirement + BmpHashTableRequirement;

    u8* MemBlock = reinterpret_cast<u8*>(LinearAllocator::Instance().Allocate(MemoryRequirement));

    // Блоки массивов находятся после состояния. Уже выделены, поэтому просто установите указатель.
    auto BmpArrayBlock = reinterpret_cast<BitmapFontLookup*>(MemBlock + StructRequirement);

    // Блоки хэш-таблиц находятся после массивов.
    u16* BmpHashtableBlock = reinterpret_cast<u16*>(BmpArrayBlock);

    state = new(MemBlock) State(config, BmpArrayBlock, BmpHashtableBlock);

    // Сделайте недействительными все записи в обоих массивах.
    /*u32 count = state->config.MaxBitmapFontCount;
    for (u32 i = 0; i < count; ++i) {
        state->BitmapFonts[i].id = INVALID::U16ID;
        state->BitmapFonts[i].ReferenceCount = 0;
    }*/

    // Загрузите все шрифты по умолчанию.
    // Растровые шрифты.
    for (u32 i = 0; i < state->config.DefaultBitmapFontCount; ++i) {
        if (!LoadFont(state->config.BitmapFontConfigs[i])) {
            MERROR("Не удалось загрузить растровый шрифт: %s", state->config.BitmapFontConfigs[i].name.c_str());
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
    }
}

bool FontSystem::LoadFont(SystemFontConfig &config)
{
    MERROR("Системные шрифты пока не поддерживаются.");
    return false;
}

bool FontSystem::LoadFont(BitmapFontConfig &config)
{
    // Убедитесь, что шрифт с таким именем еще не существует.
    u16 id = INVALID::U16ID;
    if (!state->bitmapFontLookup.Get(config.name, &id)) {
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

    if (!ResourceSystem::Instance()->Load(config.ResourceName.c_str(), ResourceType::BitmapFont, 0, lookup.font.LoadedResource)) {
        MERROR("Не удалось загрузить растровый шрифт.");
        return false;
    }

    // Сохраните приведенный указатель на данные ресурса для удобства.
    lookup.font.ResourceData = reinterpret_cast<BitmapFontResourceData*>(lookup.font.LoadedResource.data);

    // Получить текстуру.
    // ЗАДАЧА: на данный момент занимает только одну страницу.
    lookup.font.ResourceData->data.atlas.texture = TextureSystem::Instance()->Acquire(lookup.font.ResourceData->pages[0].file, true);

    bool result = SetupFontData(lookup.font.ResourceData->data);

    // Установите идентификатор записи здесь в последнюю очередь перед обновлением хэш-таблицы.
    if (!state->bitmapFontLookup.Set(config.name, id)) {
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
        MERROR("Системные шрифты пока не поддерживаются");
        return false;
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
    if (!Renderer::TextureMapAcquireResources(&font.atlas)) {
        MERROR("Невозможно получить ресурсы для карты текстур атласа шрифтов.");
        return false;
    }

    // Проверьте наличие символа табуляции, так как он не всегда может быть экспортирован. 
    // Если он есть, сохраните его xAdvance и просто используйте его. Если его нет, создайте его на основе пробела x 4
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
    Renderer::TextureMapReleaseResources(&font.atlas);

    // Если это растровый шрифт, освободите ссылку на текстуру.
    if (font.type == FontType::Bitmap && font.atlas.texture) {
        TextureSystem::Instance()->Release(font.atlas.texture->name);
    }
    font.atlas.texture = nullptr;
}
