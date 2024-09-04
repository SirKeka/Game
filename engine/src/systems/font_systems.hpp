#pragma once
#include "containers/mstring.hpp"

struct SystemFontConfig {
    MString name;
    u16 DefaultSize;
    MString ResourceName;
};

struct BitmapFontConfig {
    MString name;
    u16 size;
    MString ResourceName;
};

struct FontSystemConfig {
    u8 DefaultSystemFontCount;
    SystemFontConfig* SystemFontConfigs;
    u8 DefaultBitmapFontCount;
    BitmapFontConfig* BitmapFontConfigs;
    u8 MaxSystemFontCount;
    u8 MaxBitmapFontCount;
    bool AutoRelease;

    constexpr FontSystemConfig() 
    : 
    DefaultSystemFontCount(), 
    SystemFontConfigs(nullptr), 
    DefaultBitmapFontCount(), 
    BitmapFontConfigs(nullptr), 
    MaxSystemFontCount(),
    MaxBitmapFontCount(),
    AutoRelease() {}
    constexpr FontSystemConfig(u8 DefaultSystemFontCount, SystemFontConfig* SystemFontConfigs);
    constexpr FontSystemConfig(const FontSystemConfig& c)
    :
    DefaultSystemFontCount(c.DefaultSystemFontCount), 
    SystemFontConfigs(c.SystemFontConfigs), 
    DefaultBitmapFontCount(c.DefaultBitmapFontCount), 
    BitmapFontConfigs(c.BitmapFontConfigs), 
    MaxSystemFontCount(c.MaxSystemFontCount),
    MaxBitmapFontCount(c.MaxBitmapFontCount),
    AutoRelease(c.AutoRelease) {}
};

struct BitmapFontLookup;

class FontSystem
{
private:
    struct State;
    static State* state;
public:
    constexpr FontSystem();
    ~FontSystem() = default;

    static bool Initialize(FontSystemConfig& config);
    static void Shutdown();

    static bool LoadFont(SystemFontConfig& config);
    static bool LoadFont(BitmapFontConfig& config);

    /// @brief Пытается получить шрифт с указанным именем и назначить его указанному Text.
    /// @param FontName Имя шрифта для получения. Должен быть уже загруженным шрифтом.
    /// @param FontSize Размер шрифта. Игнорируется для растровых шрифтов.
    /// @param text Указатель на текстовый объект, для которого необходимо получить шрифт.
    /// @return True в случае успеха; в противном случае false.
    static bool Acquire(const char* FontName, u16 FontSize, struct Text& text);
    /// @brief Освобождает ссылки на шрифт, хранящийеся в предоставленном Text.
    /// @param text Указатель на текстовый объект, из которого следует освободить шрифт.
    /// @return True в случае успеха; в противном случае false.
    static bool Release(struct Text& text);

    static bool VerifyAtlas(struct FontData* font, const char* text);
};
