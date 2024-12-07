namespace Render
{
    enum DebugViewMode : u32 {
        Default = 0,
        Lighting = 1,
        Normals = 2
    };
} // namespace Render

enum RenderingConfigFlagBits {
    VsyncEnabledBit = 0x1,  // Указывает, что vsync следует включить.
    PowerSavingBit = 0x2,   // Настраивает рендерер таким образом, чтобы экономить электроэнергию, где это возможно.
};

using RendererConfigFlags = u32;

/// @brief Общая конфигурация для рендерера.
struct RenderingConfig {
    const char* ApplicationName;    // Имя приложения.
    RendererConfigFlags flags;      // Различные флаги конфигурации для настройки рендерера.
};