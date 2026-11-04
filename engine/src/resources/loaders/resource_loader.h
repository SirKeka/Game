#pragma once
#include "resources/texture.h"
#include "resources/material.h"
#include "resources/font_resource.hpp"
#include "resources/geometry.h"
#include "resources/simple_scene_config.h"
#include "resources/terrain.h"

struct SimpleSceneConfig;
struct ShaderConfig;

namespace eResource
{
    enum Type : u8 {
        Text,        // Тип ресурса Text.
        Binary,      // Тип ресурса Binary.
        Image,       // Тип ресурса Image.
        Material,    // Тип ресурса Material.
        Mesh,        // Тип ресурса Mesh (коллекция конфигураций геометрии).
        Shader,      // Тип ресурса Shader (или, точнее, конфигурация шейдера).
        BitmapFont,  // Тип ресурса растрового шрифта.
        SystemFont,  // Тип ресурса системного шрифта.
        SimpleScene, // Тип ресурса простой сцены.
        Terrain,     // Тип ресурса ландшафта
        Custom,      // Тип пользовательского ресурса. Используется загрузчиками вне основного движка.

        Invalid = 255
    };
} // namespace Resource

/// @brief Общая структура ресурса. Все загрузчики ресурсов загружают в них данные.
/// @tparam T 
template<typename T, eResource::Type rt>
struct Resource {
    eResource::Type type{rt}; // Тип ресурса
    u32 LoaderID;             // Идентификатор загрузчика, обрабатывающего этот ресурс.
    const char* name;         // Название ресурса.
    MString FullPath;         // Полный путь к файлу ресурса.
    T data;                   // Данные ресурса.
};

using TextResource        = Resource<MString, eResource::Type::Text>;
using BinaryResource      = Resource<DArray<u8>, eResource::Type::Binary>;
using ImageResource       = Resource<ImageResourceData, eResource::Type::Image>;
using MaterialResource    = Resource<MaterialConfig, eResource::Type::Material>;
using MeshResource        = Resource<DArray<GeometryConfig>, eResource::Type::Mesh>;
using ShaderResource      = Resource<ShaderConfig, eResource::Type::Shader>;
using BitmapFontResource  = Resource<BitmapFontResourceData, eResource::Type::BitmapFont>;
using SystemFontResource  = Resource<SystemFontResourceData, eResource::Type::SystemFont>;
using SimpleSceneResource = Resource<SimpleSceneConfig, eResource::Type::SimpleScene>;
using TerrainResource     = Resource<Terrain::Config, eResource::Type::Terrain>;
//using CustomResource = Resource<>;

/// @brief Магическое число, указывающее на файл как двоичный файл.
constexpr u32 RESOURCE_MAGIC = 0xcafebabe;

/// @brief Данные заголовка для типов двоичных ресурсов.
struct ResourceHeader {
    u32 MagicNumber;    // Магическое число, указывающее на файл как двоичный файл.
    u8 ResourceType;    // Тип ресурса. Сопоставляется с перечислением ResourceType.
    u8 version;         // Версия формата, используемая этим ресурсом.
    u16 reserved;       // Зарезервировано для будущих данных заголовка.
};

struct MAPI ResourceLoader
{
    u32 id;
    eResource::Type type;
    MString CustomType;
    MString TypePath;

    constexpr ResourceLoader() : id(INVALID::ID), type(eResource::Type::Invalid), CustomType(), TypePath() {}
    constexpr ResourceLoader(eResource::Type type, const MString& CustomType, const MString& TypePath) : id(INVALID::ID), type(type), CustomType(CustomType), TypePath(TypePath) {}
    ~ResourceLoader() {
        id = INVALID::ID;
    };

    MINLINE void Destroy() { this->~ResourceLoader(); }

    static bool ImageQueryProperties(const char* ImageName, i32& OutWidth, i32& OutHeight, i32& OutChannels, u32& OutMipLevels);

    bool Load(const char* name, void* params,        TextResource& OutResource);
    bool Load(const char* name, void* params,      BinaryResource& OutResource);
    bool Load(const char* name, void* params,       ImageResource& OutResource);
    bool Load(const char* name, void* params,    MaterialResource& OutResource);
    bool Load(const char* name, void* params,        MeshResource& OutResource);
    bool Load(const char* name, void* params,      ShaderResource& OutResource);
    bool Load(const char* name, void* params,  BitmapFontResource& OutResource);
    bool Load(const char* name, void* params,  SystemFontResource& OutResource);
    bool Load(const char* name, void* params, SimpleSceneResource& OutResource);
    bool Load(const char* name, void* params,     TerrainResource& OutResource);
    //bool Load(const char* name, void* params, CustomResource& OutResource);
    
    void Unload(TextResource&        resource);
    void Unload(BinaryResource&      resource);
    void Unload(ImageResource&       resource);
    void Unload(MaterialResource&    resource);
    void Unload(MeshResource&        resource);
    void Unload(ShaderResource&      resource);
    void Unload(BitmapFontResource&  resource);
    void Unload(SystemFontResource&  resource);
    void Unload(SimpleSceneResource& resource);
    void Unload(TerrainResource&     resource);

    template<typename T>
    bool ResourceUnload(T &resource, Memory::Tag tag)
    {
        if (resource.FullPath) {
            resource.FullPath.Destroy();
        }

        //if (resource.data) {
            //MMemory::Free(resource.data, resource.DataSize, tag);
            //resource.data = nullptr;
            resource.LoaderID = INVALID::ID;
        //}

        return true;
    }
};
