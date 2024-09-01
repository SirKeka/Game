#pragma once
#include "defines.hpp"
#include "core/logger.hpp"

/// @brief Общая структура ресурса. Все загрузчики ресурсов загружают в них данные.

//template<typename T>
struct Resource {
    u32 LoaderID{};         // Идентификатор загрузчика, обрабатывающего этот ресурс.
    const char* name;       // Название ресурса.
    MString FullPath;       // Полный путь к файлу ресурса.
    u64 DataSize{};         // Размер данных ресурса в байтах.
    void* data{nullptr};    // Данные ресурса.
    constexpr Resource() : LoaderID(), name(), FullPath(), DataSize(), data(nullptr) {}
};

/*
 * using TextResource = Resource<char*>; // Или MString?
 * using BinaryResource = Resource<u8*>;
 * using ImageResource = Resource<ImageResourceData>;
 * using MaterialResource = Resource<MaterialConfig>;
 * using MeshResource = Resource<GeometryConfig>;
 * using ShaderResource = Resource<ShaderConfig>;
 * using BitmapFontResource = Resource<>;
 * using CustomResource = Resource<>;
*/

enum class ResourceType {
    Invalid,    // 
    Text,       // Тип ресурса Text.
    Binary,     // Тип ресурса Binary.
    Image,      // Тип ресурса Image.
    Material,   // Тип ресурса Material.
    Mesh,       // Тип ресурса Mesh (коллекция конфигураций геометрии).
    Shader,     // Тип ресурса Shader (или, точнее, конфигурация шейдера).
    BitmapFont, // Тип ресурса растрового шрифта.
    Custom      // Тип пользовательского ресурса. Используется загрузчиками вне основного движка.
};

/// @brief Магическое число, указывающее на файл как двоичный файл.
constexpr u32 RESOURCE_MAGIC = 0xcafebabe;

/// @brief Данные заголовка для типов двоичных ресурсов.
struct ResourceHeader {
    u32 MagicNumber;    // Магическое число, указывающее на файл как двоичный файл.
    u8 ResourceType;    // Тип ресурса. Сопоставляется с перечислением ResourceType.
    u8 version;         // Версия формата, используемая этим ресурсом.
    u16 reserved;       // Зарезервировано для будущих данных заголовка.
};

class ResourceLoader
{
    friend class ResourceSystem;
protected:
    u32 id{INVALID::ID};
    ResourceType type;
    MString CustomType;
    MString TypePath;
public:
    // constexpr ResourceLoader() : id(INVALID::ID), type(), CustomType(), TypePath() {}
    constexpr ResourceLoader(ResourceType type, const MString& CustomType, const MString& TypePath) : id(INVALID::ID), type(type), CustomType(CustomType), TypePath(TypePath) {}
    virtual ~ResourceLoader() {
        id = INVALID::ID;
    };
    MINLINE void Destroy() { this->~ResourceLoader(); }

    virtual bool Load(const char* name, void* params, Resource& OutResource) {
        MERROR("Вызывается не инициализированный загрузчик");
        return false;
    }
    virtual void Unload( Resource& resource){
        MERROR("Вызывается не инициализированный загрузчик");
    }
};
