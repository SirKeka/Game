#include "texture.hpp"
//#include "renderer/vulkan/vulkan_api.hpp"
//#include "renderer/vulkan/vulkan_utils.hpp"
#include "renderer/vulkan/vulkan_image.hpp"

/*Texture::Texture(const char* name, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, bool HasTransparency, bool IsWriteable) 
{
    // Создание внутренних данных.
    // ЗАДАЧА: Используйте для этого распределитель.
    // Data = reinterpret_cast<VulkanTextureData*>(MMemory::Allocate(sizeof(VulkanTextureData), MemoryTag::Texture));
    VkDeviceSize ImageSize = width * height * ChannelCount;

    // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит.
    VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // Создание промежуточного буфера и загрузка в него данных.
    //VkBufferUsageFlagsBits usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags MemoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging{VkAPI, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryPropFlags, true, false};

    staging.LoadData(VkAPI, 0, ImageSize, 0, pixels);

    // ПРИМЕЧАНИЕ. Здесь много предположений, для разных типов текстур потребуются разные параметры.
    // Data->image = VulkanImage(
    //     VkAPI,
    //     VK_IMAGE_TYPE_2D,
    //     width,
    //     height,
    //     ImageFormat,
    //     VK_IMAGE_TILING_OPTIMAL,
    //     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //     true,
    //     VK_IMAGE_ASPECT_COLOR_BIT);

    VulkanCommandBuffer TempBuffer{};
    VkCommandPool& pool = VkAPI->Device.GraphicsCommandPool;
    VkQueue& queue = VkAPI->Device.GraphicsQueue;
    VulkanCommandBufferAllocateAndBeginSingleUse(VkAPI, pool, &TempBuffer);

    // Измените макет с того, какой он есть в данный момент, на оптимальный для получения данных.
    Data->image.TransitionLayout(
        VkAPI,
        &TempBuffer,
        ImageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Скопируйте данные из буфера.
    Data->image.CopyFromBuffer(VkAPI, staging.handle, &TempBuffer);

    // Переход от оптимальной для приема данных компоновки к оптимальной для шейдера, доступной только для чтения.
    Data->image.TransitionLayout(
        VkAPI,
        &TempBuffer,
        ImageFormat,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VulkanCommandBufferEndSingleUse(VkAPI, pool, &TempBuffer, queue);

    // Уничтожение промежуточного буфера
    staging.Destroy(VkAPI);

    this->generation++;
}*/

constexpr Texture::Texture(const char *name, i32 width, i32 height, i32 ChannelCount, TextureFlagBits flags)
: 
    id(), 
    width(width), 
    height(height), 
    ChannelCount(ChannelCount), 
    flags(flags),
    generation(INVALID::ID), 
    name(),
    Data(nullptr)
{
    MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH);
}

Texture::~Texture()
{
    if(Data) {
        delete Data;
    }
    Data = nullptr;
}

Texture::Texture(const Texture &t)
    : 
    id(t.id), 
    width(t.width), 
    height(t.height), 
    ChannelCount(t.ChannelCount), 
    flags(t.flags), 
    generation(t.generation), 
    name(), 
    Data(new VulkanImage(*t.Data)) { 
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
}

Texture &Texture::operator=(const Texture &t)
{
    id = t.id;
    width = t.width;
    height = t.height;
    ChannelCount = t.ChannelCount;
    flags = t.flags;
    generation = t.generation;
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH);
    Data = new VulkanImage(*t.Data);
    return *this;
}

Texture &Texture::operator=(Texture &&t)
{
    id = t.id;
    width = t.width;
    height = t.height;
    ChannelCount = t.ChannelCount;
    flags = t.flags;
    generation = t.generation;
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH);
    Data = t.Data;

    t.id = 0;
    t.width = 0;
    t.height = 0;
    t.ChannelCount = 0;
    t.flags = 0;
    t.generation = 0;
    MString::Zero(t.name);
    t.Data = nullptr;

    return *this;
}

void Texture::Create(const char* name, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, TextureFlagBits flags)
{
    this->width = width;
    this->height = height;
    this->ChannelCount = ChannelCount;
    this->generation = INVALID::ID;
    MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH); // this->name = name;
    this->flags = flags;
    this->generation++;
}

Texture::operator bool() const
{
    if (id != 0 && width  != 0 && height != 0 && ChannelCount != 0 && flags != 0 && generation != 0 && Data != nullptr) {
        return true;
    }
    return false;
}

void *Texture::operator new(u64 size)
{
    return MMemory::Allocate(size, MemoryTag::Texture);
}

void *Texture::operator new[](u64 size)
{
    return MMemory::Allocate(size, MemoryTag::Renderer);
}

void Texture::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::Texture);
}

void Texture::operator delete[](void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::Renderer);
}
