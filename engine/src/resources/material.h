#pragma once
#include "shader.hpp"
#include "texture_map.hpp"
#include "math/matrix4d.hpp"

#define MATERIAL_NAME_MAX_LENGTH 256    // Максимальная длина имени материала.

/// @brief Материал, который отображает различные свойства поверхности в окружающем мире, 
/// такие как текстура, цвет, шероховатость, блеск и многое другое.
struct Material {
    enum Type {
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
        Shader::UniformType type;
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

        ConfigProp() : name(), type(), size(), ValueV4(), ValueV3(), ValueV2(), ValueF32(), ValueU32(), ValueU16(), ValueU8(), ValueI32(), ValueI16(), ValueI8(), ValueMatrix4D() {}
    };

    struct Map {
        MString name;
        MString TextureName;
        TextureFilter FilterMin;
        TextureFilter FilterMag;
        TextureRepeat RepeatU;
        TextureRepeat RepeatV;
        TextureRepeat RepeatW;
    };

    struct Config {
        u8 version;
        MString name;
        Type type;
        MString ShaderName;
        DArray<ConfigProp> properties;
        DArray<Map> maps;
        /// @brief Указывает, следует ли автоматически освобождать материал, если на него не осталось никаких ссылок.
        bool AutoRelease;
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


    /// @brief Идентификатор материала.
    u32 id;
    /// @brief Тип материала.
    Type type;
    /// @brief Генерация материала. Увеличивается каждый раз при изменении материала.
    u32 generation;
    /// @brief Внутренний идентификатор материала. Используется бэкэндом рендерера для сопоставления с внутренними ресурсами.
    u32 internal_id;
    /// @brief Имя материала.
    char name[MATERIAL_NAME_MAX_LENGTH];

    /// @brief Массив текстурных карт.
    TextureMap *maps;

    /// @brief Размер структуры свойств.
    u32 PropertyStructSize;

    /// @brief Массив структур свойств материала, который зависит от типа материала. например, material_phong_properties
    void *properties;

    /// @brief Рассеянный цвет.
    // vec4 DiffuseColour;

    /// @brief Блеск материала определяет, насколько концентрировано зеркальное освещение.
    // f32 specular;

    u32 shader_id;

    /// @brief Синхронизируется с текущим номером кадра рендерера, когда материал был применен к этому кадру.
    u32 RenderFrameNumber;

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::MaterialInstance); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::MaterialInstance); }
};
