#pragma once
#include "containers/mstring.hpp"
#include "containers/hashtable.hpp"

struct Text;

struct SystemFontConfig {
    MString name        {};
    u16 DefaultSize     {};
    MString ResourceName{};
    constexpr SystemFontConfig(MString&& name, u16 DefaultSize, MString&& ResourceName) : name(static_cast<MString&&>(name)), DefaultSize(DefaultSize), ResourceName(static_cast<MString&&>(ResourceName)) {}
    void* operator new(u64 size)              { return MemorySystem::Allocate(size, Memory::SystemFont); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::SystemFont); }
};

struct BitmapFontConfig {
    MString name        {};
    u16 size            {};
    MString ResourceName{};
    constexpr BitmapFontConfig(MString&& name, u16 size, MString&& ResourceName) : name(name), size(size), ResourceName(ResourceName) {}

    void* operator new(u64 size)              { return MemorySystem::Allocate(size, Memory::BitmapFont); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::BitmapFont); }
};

struct FontSystemConfig {
    u8 DefaultSystemFontCount;
    SystemFontConfig* SystemFontConfigs;
    u8 DefaultBitmapFontCount;
    BitmapFontConfig* BitmapFontConfigs;
    u8 MaxSystemFontCount;
    u8 MaxBitmapFontCount;
    bool AutoRelease;
};

struct BitmapFontLookup;
struct SystemFontLookup;

class FontSystem
{
private:
    FontSystemConfig config             {};
    HashTable<u16> bitmapFontLookup     {};
    HashTable<u16> systemFontLookup     {};
    BitmapFontLookup* BitmapFonts{nullptr};
    SystemFontLookup* SystemFonts{nullptr};
    [[maybe_unused]]void* BitmapHashTableBlock   {nullptr};
    [[maybe_unused]]void* SystemHashtableBlock   {nullptr};

    static FontSystem* state;

    constexpr FontSystem(FontSystemConfig& config, BitmapFontLookup* BitmapFonts, SystemFontLookup* SystemFonts, u16* BitmapHashTableBlock, u16* SystemHashtableBlock)
    : config(config), 
    bitmapFontLookup(config.MaxBitmapFontCount, false, BitmapHashTableBlock, true, INVALID::U16ID), // Создание хэш-таблицы для поиска шрифтов и заполнить недействительными индексами.
    systemFontLookup(config.MaxBitmapFontCount, false, SystemHashtableBlock, true, INVALID::U16ID),
    BitmapFonts(BitmapFonts), SystemFonts(SystemFonts),
    BitmapHashTableBlock(BitmapHashTableBlock),
    SystemHashtableBlock(SystemHashtableBlock) {}
public:
    ~FontSystem() = default;

    static bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    static void Shutdown();

    static bool LoadFont(SystemFontConfig& config);
    static bool LoadFont(BitmapFontConfig& config);

    /// @brief Пытается получить шрифт с указанным именем и назначить его указанному Text.
    /// @param FontName Имя шрифта для получения. Должен быть уже загруженным шрифтом.
    /// @param FontSize Размер шрифта. Игнорируется для растровых шрифтов.
    /// @param text Указатель на текстовый объект, для которого необходимо получить шрифт.
    /// @return True в случае успеха; в противном случае false.
    static bool Acquire(const char* FontName, u16 FontSize, Text& text);
    /// @brief Освобождает ссылки на шрифт, хранящийеся в предоставленном Text.
    /// @param text Указатель на текстовый объект, из которого следует освободить шрифт.
    /// @return True в случае успеха; в противном случае false.
    static bool Release(Text& text);

    static bool VerifyAtlas(struct FontData* font, const char* text);
};
