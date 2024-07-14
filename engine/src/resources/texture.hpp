#pragma once

#include "defines.hpp"
#include "renderer/vulkan/vulkan_image.hpp"
#include "core/mmemory.hpp"
#include "containers/mstring.hpp"

constexpr u32 TEXTURE_NAME_MAX_LENGTH = 512;

class VulkanAPI;

struct VulkanTextureData {
    VulkanImage image;

    constexpr VulkanTextureData() : image() {}
    constexpr VulkanTextureData(VulkanImage image) : image(image) {}
    constexpr VulkanTextureData(const VulkanTextureData& value) : image(value.image) {}

    void* operator new(u64 size) { 
        return MMemory::Allocate(size, MemoryTag::Texture); 
    }
    void operator delete(void* ptr, u64 size) { 
        MMemory::Free(ptr, size, MemoryTag::Texture); 
    }
};

enum class TextureUse {
    Unknown = 0x00,         // Неизвестное применение. Это значение по умолчанию, но его никогда не следует использовать.
    MapDiffuse = 0x01,      // Текстура используется как диффузная карта.
    MapSpecular = 0x02,     // Текстура используется как карта отражений.
    MapNormal = 0x03        // Текстура используется как карта нормалей.
};

/// @brief Представляет поддерживаемые режимы фильтрации текстур.
enum class TextureFilter {
    ModeNearest = 0x0,      // Фильтрация ближайших соседей.
    ModeLinear = 0x1        // Линейная (т.е. билинейная) фильтрация.
};

enum class TextureRepeat {
    Repeat = 0x1,
    MirroredRepeat = 0x2,
    ClampToEdge = 0x3,
    ClampToBorder = 0x4
};

class Texture
{
public:
    u32 id;                             // Уникальный идентификатор текстуры.
    u32 width;                          // Ширина текстуры.
    u32 height;                         // Высота текстуры.
    u8 ChannelCount;                    // Количество каналов в текстуре.
    bool HasTransparency;               // Указывает, имеет ли текстура прозрачность.
    bool IsWriteable;                   // Указывает, можно ли записать (отобразить) текстуру.
    u32 generation;                     // Генерация текстур. Увеличивается каждый раз при перезагрузке данных.
    char name[TEXTURE_NAME_MAX_LENGTH]; // Имя текстуры.

    // ЗАДАЧА: Пока нет реализации DirectX храним указатель текстуры Vulkan.
    VulkanTextureData* Data;            // Необработанные данные текстуры (пиксели).
public:
    constexpr Texture() 
    : id(INVALID::ID), width(0), height(0), ChannelCount(0), HasTransparency(false), IsWriteable(false), generation(INVALID::ID), name(), Data(nullptr) {}
    Texture(const char* name, i32 width, i32 height, i32 ChannelCount, const u8* pixels, bool HasTransparency, bool IsWriteable, VulkanAPI* VkAPI);
    ~Texture();
    Texture(const Texture& t);
    constexpr Texture(Texture&& t) : id(t.id), width(t.width), height(t.height), ChannelCount(t.ChannelCount), 
    HasTransparency(t.HasTransparency), IsWriteable(t.IsWriteable), generation(t.generation), name(), Data(t.Data) { 
        MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
        t.id = 0;
        t.width = 0;
        t.height = 0;
        t.ChannelCount = 0;
        t.HasTransparency = 0;
        t.IsWriteable = 0;
        t.generation = 0;
        MString::Zero(t.name);
        t.Data = nullptr;
    }
    Texture& operator= (const Texture& t) {
        id = t.id;
        width = t.width;
        height = t.height;
        ChannelCount = t.ChannelCount;
        HasTransparency = t.HasTransparency;
        IsWriteable = t.IsWriteable;
        generation = t.generation;
        MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH);
        Data = new VulkanTextureData(*t.Data);
        return *this;
    }
    Texture& operator= (Texture&& t) {
        id = t.id;
        width = t.width;
        height = t.height;
        ChannelCount = t.ChannelCount;
        HasTransparency = t.HasTransparency;
        IsWriteable = t.IsWriteable;
        generation = t.generation;
        MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH);
        Data = t.Data;

        t.id = 0;
        t.width = 0;
        t.height = 0;
        t.ChannelCount = 0;
        t.HasTransparency = 0;
        t.IsWriteable = 0;
        t.generation = 0;
        MString::Zero(t.name);
        t.Data = nullptr;

        return *this;
    }
    void Create(
        const char* name, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        bool IsWriteable,
        VulkanAPI* VkAPI);
    void Destroy(VulkanAPI* VkAPI);


    explicit operator bool() const;
    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
