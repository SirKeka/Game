#pragma once

#include "defines.hpp"
#include "renderer/vulkan/vulkan_image.hpp"
#include "containers/mstring.hpp"

constexpr u32 TEXTURE_NAME_MAX_LENGTH = 512;

class VulkanAPI;

struct VulkanTextureData {
    VulkanImage image;
    VkSampler sampler;
};

enum class TextureUse {
    Unknown = 0x00,
    MapDiffuse = 0x01
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
    Texture();
    Texture(
        const char* name, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        VulkanAPI* VkAPI);
    ~Texture();
    Texture(const Texture& t);
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
    void operator delete(void* ptr);
};
