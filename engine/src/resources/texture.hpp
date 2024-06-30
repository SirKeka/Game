#pragma once

#include "defines.hpp"
#include "renderer/vulkan/vulkan_image.hpp"
#include "core/mmemory.hpp"
#include "containers/mstring.hpp"

constexpr u32 TEXTURE_NAME_MAX_LENGTH = 512;

class VulkanAPI;

struct VulkanTextureData {
    VulkanImage image;
    VkSampler sampler;
    constexpr VulkanTextureData() : image(), sampler() {}
    constexpr VulkanTextureData(VulkanImage image) : image(image), sampler() {}
    constexpr VulkanTextureData(const VulkanTextureData& value) : image(value.image), sampler(value.sampler) {}
    void* operator new(u64 size) { return MMemory::Allocate(size, MemoryTag::Texture); }
    void operator delete(void* ptr, u64 size) { MMemory::Free(ptr, size, MemoryTag::Texture); }
};

enum class TextureUse {
    Unknown = 0x00,         // Неизвестное применение. Это значение по умолчанию, но его никогда не следует использовать.
    MapDiffuse = 0x01,      // Текстура используется как диффузная карта.
    MapSpecular = 0x02,     // Текстура используется как карта отражений.
    MapNormal = 0x03        // Текстура используется как карта нормалей.
};

class Texture
{
public:
    u32 id;                             // Уникальный идентификатор текстуры.
    u32 width;                          // Ширина текстуры.
    u32 height;                         // Высота текстуры.
    u8 ChannelCount;                    // Количество каналов в текстуре.
    bool HasTransparency;               // Указывает, имеет ли текстура прозрачность.
    u32 generation;                     // Генерация текстур. Увеличивается каждый раз при перезагрузке данных.
    char name[TEXTURE_NAME_MAX_LENGTH]; // Имя текстуры.

    // СДЕЛАТЬ: Пока нет реализации DirectX храним указатель текстуры Vulkan.
    VulkanTextureData* Data;            // Необработанные данные текстуры (пиксели).
public:
    constexpr Texture() : id(INVALID::ID), width(0), height(0), ChannelCount(0), HasTransparency(false), generation(INVALID::ID), name(), Data(nullptr) {}
    Texture(const char* name, i32 width, i32 height, i32 ChannelCount, const u8* pixels, bool HasTransparency, VulkanAPI* VkAPI);
    ~Texture();
    Texture(const Texture& t) 
    : id(t.id), width(t.width), height(t.height), ChannelCount(t.ChannelCount), 
    HasTransparency(t.HasTransparency), generation(t.generation), name(), Data(new VulkanTextureData(*t.Data)) 
    { MMemory::CopyMem(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); }
    void Create(
        const char* name, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        VulkanAPI* VkAPI);
    void Destroy(VulkanAPI* VkAPI);


    explicit operator bool() const;
    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
