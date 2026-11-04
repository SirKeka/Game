#pragma once
#include "shader.h"
#include "texture.h"
#include "math/matrix4d.h"

#define MATERIAL_NAME_MAX_LENGTH 256    // Максимальная длина имени материала.

struct MaterialConfig;

/// @brief Материал, который отображает различные свойства поверхности в окружающем мире, 
/// такие как текстура, цвет, шероховатость, блеск и многое другое.
struct Material {
    enum Type : u8 {
        // Invalid.
        Unknown = 0,
        Phong = 1,
        PBR = 2,
        UI = 3,
        Terrain = 4,
        Custom = 99
    };

    struct ConfigProp {
        MString name;
        ShaderUniformType type;
        u32 size;
        // FIXME: Это кажется колоссальной тратой памяти... может быть, union или что-то получше?
        FVec4 ValueV4;
        FVec3 ValueV3;
        FVec2 ValueV2;
        f32 ValueF32;
        u32 ValueU32;
        u16 ValueU16;
        u8 ValueU8;
        i32 ValueI32;
        i16 ValueI16;
        i8 ValueI8;
        Matrix4D ValueMatrix4D;
    };

    struct Map {
        MString name;
        MString TextureName;
        TextureFilterMode FilterMin;
        TextureFilterMode FilterMag;
        TextureRepeat RepeatU;
        TextureRepeat RepeatV;
        TextureRepeat RepeatW;

        constexpr Map() : name(), TextureName(), FilterMin(), FilterMag(), RepeatU(), RepeatV(), RepeatW() {}
        // Установите разумные значения по умолчанию для старых типов материалов.
        constexpr Map(const char *name, Substring TextureName) : name(name), TextureName(TextureName), FilterMin(TextureFilterMode::Linear), FilterMag(FilterMin), RepeatU(TextureRepeat::Repeat), RepeatV(RepeatU), RepeatW(RepeatU) {}
    };

    struct PhongProperties {
        /// @brief Рассеянный цвет.
        FVec4 DiffuseColour;

        FVec3 padding;
        /// @brief Блеск материала определяет, насколько концентрированным будет зеркальное освещение.
        f32 specular;
    };

    struct UiProperties {
        /// @brief Рассеянный цвет.
        FVec4 DiffuseColour;
    };

    struct TerrainProperties {
        PhongProperties materials[4];
        FVec3 padding;
        i32 NumMaterials;
        FVec4 padding2;
    };

    char name[MATERIAL_NAME_MAX_LENGTH]; // Имя материала.
    DArray<TextureMap> maps;             // Массив текстурных карт.
    u32 id {INVALID::ID};                // Идентификатор материала.
    u32 generation;                      // Генерация(поколение) материала. Увеличивается каждый раз при изменении материала.
    u32 InternalId;                      // Внутренний идентификатор материала. Используется бэкэндом рендерера для сопоставления с внутренними ресурсами.

    u32 PropertyStructSize;              // Размер структуры свойств.
    void *properties;                    // Массив структур свойств материала, который зависит от типа материала. например, MaterialPhongProperties

    Texture* IrradianceTexture;         // Явно заданная текстура освещенности для этого материала. Должна использоваться только в ограниченных случаях. В идеале, её следует задавать через менеджер материалов сцены.
    
    // vec4 DiffuseColour;               // Рассеянный цвет.
    // f32 specular;                     // Блеск материала определяет, насколько концентрировано зеркальное освещение.
    
    Type type;                           // Тип материала.
    u32 ShaderID;

    /// @brief Синхронизируется с текущим номером кадра рендерера, когда материал был применен к этому кадру.
    u64 RenderFrameNumber;
    u64 RenderDrawIndex;

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::MaterialInstance); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::MaterialInstance); }
};

struct MaterialConfig {
    u8 version;
    MString name;
    Material::Type type;
    MString ShaderName;
    DArray<Material::ConfigProp> properties;
    DArray<Material::Map> maps;
    /// @brief Указывает, следует ли автоматически освобождать материал, если на него не осталось никаких ссылок.
    bool AutoRelease;

    void Clear() { version = 0; name.Clear(), type = Material::Type::Unknown; ShaderName.Clear(); properties.Clear(); maps.Clear(); AutoRelease = 0; }
    void Destroy() { version = 0; name.Destroy(), type = Material::Type::Unknown; ShaderName.Destroy(), properties.Destroy(); maps.Destroy(); AutoRelease = 0; }
};