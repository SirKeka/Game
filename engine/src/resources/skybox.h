#pragma once
#include "geometry.h"

struct MAPI Sky
{
    /// @brief Конфигурация скайбокса
    /// @param const_char*_CubemapName имя кубической карты, которая будет использоваться для скайбокса.
    /// @param GeometryConfig_GConfig конфигурация геометрии скайбокса
    struct Config {
        MString CubemapName;
        GeometryConfig GConfig;
    };

    MString CubemapName;
    TextureMap cubemap;
    struct Geometry* geometry;
    u64 RenderFrameNumber;  // Синхронизируется с текущим номером кадра рендерера, когда материал был применен к этому кадру.
    u32 InstanceID;
    u8 DrawIndex;           // Синхронизируется с текущим индексом отрисовки рендерера, когда материал был применен к этому кадру.

    /// @brief Указывает на то на какую геометрию рисуется небо Skybox - куб, Skydome - сфера или полусфера
    enum Type : u8 { Skybox, Skydome } type; 

    // constexpr Skybox() = default;
    // ~Skybox();

    /// @brief Создает скайбокс
    /// @param config конфигурация скайбокса 
    /// @return true если скайбокс успешно создан и false в случае неудачи
    bool Create(Config& config);

    void Destroy();

    void GenerateHemisphere(u16 rings = 16U, u16 segments = 32U, f32 Radius = 10.F);

    bool Initialize();

    bool Load();

    bool Unload();

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Scene); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Scene); }
};