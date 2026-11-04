#pragma once
#include "containers/hashtable.hpp"
#include "resources/font_resource.hpp"

struct Text;

struct SystemFontConfig {
    MString name        {};
    u16 DefaultSize     {};
    MString ResourceName{};
    constexpr SystemFontConfig(MString&& name, u16 DefaultSize, MString&& ResourceName) : name((MString&&)name), DefaultSize(DefaultSize), ResourceName((MString&&)ResourceName) {}
    void* operator new(u64 size)              { return MemorySystem::Allocate(size, Memory::SystemFont); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::SystemFont); }
};

struct BitmapFontConfig {
    MString name        {};
    u16 size            {};
    MString ResourceName{};
    constexpr BitmapFontConfig(MString&& name, u16 size, MString&& ResourceName) : name((MString&&)name), size(size), ResourceName((MString&&)ResourceName) {}

    void* operator new(u64 size)              { return MemorySystem::Allocate(size, Memory::BitmapFont); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::BitmapFont); }
};

struct BitmapFontLookup;
struct SystemFontLookup;

class FontSystem
{
public:
    struct Config {
        u8 DefaultSystemFontCount;
        SystemFontConfig* SystemFontConfigs;
        u8 DefaultBitmapFontCount;
        BitmapFontConfig* BitmapFontConfigs;
        u8 MaxSystemFontCount;
        u8 MaxBitmapFontCount;
        bool AutoRelease;
    };

    ~FontSystem() = default;

    FontSystem(const FontSystem&) = delete;
    FontSystem& operator= (const FontSystem&) = delete;

    static FontSystem* Initialize(u64& MemoryRequirement, void* memory, Config& config);
    void Shutdown();

    MAPI static bool LoadFont(SystemFontConfig& config);
    MAPI static bool LoadFont(BitmapFontConfig& config);

    /// @brief Пытается получить шрифт с указанным именем и назначить его указанному Text.
    /// @param FontName Имя шрифта для получения. Должен быть уже загруженным шрифтом.
    /// @param FontSize Размер шрифта. Игнорируется для растровых шрифтов.
    /// @param text Указатель на текстовый объект, для которого необходимо получить шрифт.
    /// @return True в случае успеха; в противном случае false.
    MAPI static FontData* Acquire(const char* FontName, u16 FontSize, FontType type);
    /// @brief Освобождает ссылки на шрифт, хранящийеся в предоставленном Text.
    /// @param text Указатель на текстовый объект, из которого следует освободить шрифт.
    /// @return True в случае успеха; в противном случае false.
    static bool Release(Text& text);

    MAPI static bool VerifyAtlas(struct FontData* font, const MString& text);

    /// @brief Измеряет заданную строку, чтобы определить её ширину в самой широкой/высокой точке.
    /// @param font Указатель на шрифт, используемый для измерения.
    /// @param text Текст для измерения.
    MAPI static FVec2 MeasureString(FontData* font, const char* text, u32 CharLength = 0, u32 TextLengthUTF8 = 0);
    MAPI static FVec2 MeasureString(FontData* font, Substring text);
private:
    Config config                       {};
    HashTable<u16> bitmapFontLookup     {};
    HashTable<u16> systemFontLookup     {};
    BitmapFontLookup* BitmapFonts{nullptr};
    SystemFontLookup* SystemFonts{nullptr};
    [[maybe_unused]]void* BitmapHashTableBlock   {nullptr};
    [[maybe_unused]]void* SystemHashtableBlock   {nullptr};

    static inline FontSystem* state;

    constexpr FontSystem(Config& config, BitmapFontLookup* BitmapFonts, SystemFontLookup* SystemFonts, u16* BitmapHashTableBlock, u16* SystemHashtableBlock)
    : config(config), 
    bitmapFontLookup(config.MaxBitmapFontCount, false, BitmapHashTableBlock, true, INVALID::U16ID), // Создание хэш-таблицы для поиска шрифтов и заполнить недействительными индексами.
    systemFontLookup(config.MaxBitmapFontCount, false, SystemHashtableBlock, true, INVALID::U16ID),
    BitmapFonts(BitmapFonts), SystemFonts(SystemFonts),
    BitmapHashTableBlock(BitmapHashTableBlock),
    SystemHashtableBlock(SystemHashtableBlock) {}

};
