#pragma once
#include "texture_map.hpp"
#include "geometry.h"

struct MAPI Skybox
{
    /// @brief Конфигурация скайбокса
    /// @param const_char*_CubemapName bмя кубической карты, которая будет использоваться для скайбокса.
    /// @param GeometryConfig_GConfig конфигурация геометрии скайбокса
    struct Config {
        MString CubemapName;
        GeometryConfig GConfig;
    } config;

    TextureMap cubemap;
    struct Geometry* geometry;
    u32 InstanceID;
    u64 RenderFrameNumber;  // Синхронизируется с текущим номером кадра рендерера, когда материал был применен к этому кадру.
    u8 DrawIndex;           // Синхронизируется с текущим индексом отрисовки рендерера, когда материал был применен к этому кадру.

    // constexpr Skybox() = default;
    // ~Skybox();

    /// @brief Создает скайбокс
    /// @param config конфигурация скайбокса 
    /// @return true если скайбокс успешно создан и false в случае неудачи
    bool Create(Config& config);

    void Destroy();

    bool Initialize();

    bool Load();

    bool Unload();

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
