#pragma once
#include "resources/texture.hpp"
#include "resources/material/material_config.hpp"
#include "resources/font_resource.hpp"
#include "resources/geometry.hpp"
#include "resources/shader.hpp"
#include "resources/simple_scene_config.h"

/// @brief Общая структура ресурса. Все загрузчики ресурсов загружают в них данные.
/// @tparam T 
template<typename T>
struct Resource {
    u32 LoaderID;       // Идентификатор загрузчика, обрабатывающего этот ресурс.
    const char* name;   // Название ресурса.
    MString FullPath;   // Полный путь к файлу ресурса.
    //u64 DataSize {};    // Размер данных ресурса в байтах.
    T data;             // Данные ресурса.
    constexpr Resource() : LoaderID(), name(nullptr), FullPath(),/* DataSize(), */data() {}
};

using TextResource        = Resource<MString>;
using BinaryResource      = Resource<DArray<u8>>;
using ImageResource       = Resource<ImageResourceData>;
using MaterialResource    = Resource<MaterialConfig>;
using MeshResource        = Resource<DArray<GeometryConfig>>;
using ShaderResource      = Resource<Shader::Config>;
using BitmapFontResource  = Resource<BitmapFontResourceData>;
using SystemFontResource  = Resource<SystemFontResourceData>;
using SimpleSceneResource = Resource<SimpleSceneConfig>;
//using CustomResource = Resource<>;

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
        SimpleScene, // Простой тип ресурса сцены.
        Custom,      // Тип пользовательского ресурса. Используется загрузчиками вне основного движка.

        Invalid = 255
    };
} // namespace Resource

/// @brief Магическое число, указывающее на файл как двоичный файл.
constexpr u32 RESOURCE_MAGIC = 0xcafebabe;

/// @brief Данные заголовка для типов двоичных ресурсов.
struct ResourceHeader {
    u32 MagicNumber;    // Магическое число, указывающее на файл как двоичный файл.
    u8 ResourceType;    // Тип ресурса. Сопоставляется с перечислением ResourceType.
    u8 version;         // Версия формата, используемая этим ресурсом.
    u16 reserved;       // Зарезервировано для будущих данных заголовка.
};

struct ResourceLoader
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

    void Create(eResource::Type type, MString&& CustomType, MString&& TypePath) {
        this->id = INVALID::ID;
        this->type = type;
        this->CustomType = static_cast<MString&&>(CustomType);
        this->TypePath = static_cast<MString&&>(TypePath);
    }

    MINLINE void Destroy() { this->~ResourceLoader(); }

    bool Load(const char* name, void* params,       TextResource& OutResource);
    MAPI bool Load(const char* name, void* params,     BinaryResource& OutResource);
    bool Load(const char* name, void* params,      ImageResource& OutResource);
    bool Load(const char* name, void* params,   MaterialResource& OutResource);
    bool Load(const char* name, void* params,       MeshResource& OutResource);
    bool Load(const char* name, void* params,     ShaderResource& OutResource);
    bool Load(const char* name, void* params, BitmapFontResource& OutResource);
    bool Load(const char* name, void* params, SystemFontResource& OutResource);
    bool Load(const char* name, void* params, SimpleSceneResource& OutResource);
    //bool Load(const char* name, void* params, CustomResource& OutResource);
    
    void Unload(TextResource&       resource);
    MAPI void Unload(BinaryResource&     resource);
    void Unload(ImageResource&      resource);
    void Unload(MaterialResource&   resource);
    void Unload(MeshResource&       resource);
    void Unload(ShaderResource&     resource);
    void Unload(BitmapFontResource& resource);
    void Unload(SystemFontResource& resource);
    void Unload(SimpleSceneResource& resource);

    template<typename T>
    bool ResourceUnload(Resource<T> &resource, Memory::Tag tag)
    {
        if (resource.FullPath) {
            resource.FullPath.Clear();
        }

        //if (resource.data) {
            //MMemory::Free(resource.data, resource.DataSize, tag);
            //resource.data = nullptr;
            resource.LoaderID = INVALID::ID;
        //}

        return true;
    }
};
