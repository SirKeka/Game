#include "texture.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "renderer/vulkan/vulkan_utils.hpp"

Texture::Texture(const char* name, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, bool HasTransparency, bool IsWriteable, VulkanAPI *VkAPI) 
    : 
    id(), 
    width(width), 
    height(height), 
    ChannelCount(ChannelCount), 
    HasTransparency(HasTransparency), 
    IsWriteable(IsWriteable),
    generation(INVALID::ID), 
    name(),
    Data(new VulkanTextureData(VulkanImage(
        VkAPI,
        VK_IMAGE_TYPE_2D,
        width,
        height,
        VK_FORMAT_R8G8B8A8_UNORM, // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит.
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT))) 
{
    MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH);

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
}

Texture::~Texture()
{
    delete Data;
    Data = nullptr;
}

Texture::Texture(const Texture &t)
    : 
    id(t.id), 
    width(t.width), 
    height(t.height), 
    ChannelCount(t.ChannelCount), 
    HasTransparency(t.HasTransparency), 
    IsWriteable(t.IsWriteable), 
    generation(t.generation), 
    name(), 
    Data(new VulkanTextureData(*t.Data)) { 
    MString::nCopy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
}

void Texture::Create(const char* name, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, bool HasTransparency, bool IsWriteable, VulkanAPI *VkAPI)
{
    this->width = width;
    this->height = height;
    this->ChannelCount = ChannelCount;
    this->generation = INVALID::ID;
    MString::nCopy(this->name, name, TEXTURE_NAME_MAX_LENGTH); // this->name = name;

    // Создание внутренних данных.
    VkDeviceSize ImageSize = width * height * ChannelCount;

    // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит.
    VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // Создание промежуточного буфера и загрузка в него данных.
    //VkBufferUsageFlagsBits usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags MemoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging {};
    staging.Create(VkAPI, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryPropFlags, true, false);

    staging.LoadData(VkAPI, 0, ImageSize, 0, pixels);

    // ПРИМЕЧАНИЕ. Здесь много предположений, для разных типов текстур потребуются разные параметры.
    Data = new VulkanTextureData (VulkanImage(
        VkAPI,
        VK_IMAGE_TYPE_2D,
        width,
        height,
        ImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT));

    VulkanCommandBuffer TempBuffer{};
    const VkCommandPool& pool = VkAPI->Device.GraphicsCommandPool;
    const VkQueue& queue = VkAPI->Device.GraphicsQueue;
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

    this->HasTransparency = HasTransparency;
    this->IsWriteable = IsWriteable;
    this->generation++;
}

void Texture::Destroy(VulkanAPI *VkAPI)
{
    vkDeviceWaitIdle(VkAPI->Device.LogicalDevice);
    //vulkan_texture_data* Data = (vulkan_texture_data*)texture->internal_data;
    if(Data) {
        Data->image.Destroy(VkAPI);
        MMemory::ZeroMem(&Data->image, sizeof(VulkanImage));

    //MMemory::Free(Data, sizeof(VulkanTextureData), MemoryTag::Texture);
    }
}

Texture::operator bool() const
{
    if (id != 0 && width  != 0 && height != 0 && ChannelCount != 0 && HasTransparency != 0 && generation != 0 && Data != nullptr) {
        return true;
    }
    return false;
}

void *Texture::operator new(u64 size)
{
    return MMemory::Allocate(size, MemoryTag::Texture);
}

void Texture::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, MemoryTag::Texture);
}
