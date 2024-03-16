#include "texture.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "renderer/vulkan/vulkan_utils.hpp"

Texture::Texture() /*: 
    id(0), 
    width(0), 
    height(0), 
    ChannelCount(0), 
    HasTransparency(false), 
    generation(INVALID_ID), 
    Data(nullptr) */
    {
        id = 0;
        width = 0;
        height = 0;
        ChannelCount = 0;
        HasTransparency = 0;
        generation = INVALID_ID;
        Data = nullptr;
    }

Texture::Texture(MString name, bool AutoRelease, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, bool HasTransparency, VulkanAPI *VkAPI)
{
    Create(name, AutoRelease, width, height, ChannelCount, pixels, HasTransparency, VkAPI);
}

Texture::Texture(const Texture &t) 
: 
id(t.id), 
width(t.width), 
height(t.height), 
ChannelCount(t.ChannelCount), 
HasTransparency(t.HasTransparency), 
generation(t.generation), 
Data(t.Data) {}

void Texture::Create(MString name, bool AutoRelease, i32 width, i32 height, i32 ChannelCount, const u8 *pixels, bool HasTransparency, VulkanAPI *VkAPI)
{
    this->width = width;
    this->height = height;
    this->ChannelCount = ChannelCount;
    this->generation = INVALID_ID;

    // Создание внутренних данных.
    // TODO: Используйте для этого распределитель.
    //out_texture->internal_data = (vulkan_texture_data*)kallocate(sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
    //vulkan_texture_data* Data = (vulkan_texture_data*)out_texture->internal_data;
    Data = reinterpret_cast<VulkanTextureData*>(MMemory::Allocate(sizeof(VulkanTextureData), MEMORY_TAG_TEXTURE));
    VkDeviceSize ImageSize = width * height * ChannelCount;

    // ПРИМЕЧАНИЕ: Предполагается, что на канал приходится 8 бит.
    VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // Создание промежуточного буфера и загрузка в него данных.
    //VkBufferUsageFlagsBits usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags MemoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging {};
    staging.Create(VkAPI, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryPropFlags, true);

    staging.LoadData(VkAPI, 0, ImageSize, 0, pixels);

    // ПРИМЕЧАНИЕ. Здесь много предположений, для разных типов текстур потребуются разные параметры.
    Data->image = VulkanImage(
        VkAPI,
        VK_IMAGE_TYPE_2D,
        width,
        height,
        ImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT);

    VulkanCommandBuffer TempBuffer{};
    VkCommandPool pool = VkAPI->Device.GraphicsCommandPool;
    VkQueue queue = VkAPI->Device.GraphicsQueue;
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

    // Создание сэмплера для текстуры
    VkSamplerCreateInfo SamplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    // TODO: Эти фильтры должны быть настраиваемыми.
    SamplerInfo.magFilter = VK_FILTER_LINEAR;
    SamplerInfo.minFilter = VK_FILTER_LINEAR;
    SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = 16;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.0f;
    SamplerInfo.minLod = 0.0f;
    SamplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(VkAPI->Device.LogicalDevice, &SamplerInfo, VkAPI->allocator, &Data->sampler);
    if (!VulkanResultIsSuccess(VK_SUCCESS)) {
        MERROR("Ошибка создания сэмплера текстур: %s", VulkanResultString(result, true));
        return;
    }

    this->HasTransparency = HasTransparency;
    this->generation++;
}

void Texture::Destroy(VulkanAPI *VkAPI)
{
    vkDeviceWaitIdle(VkAPI->Device.LogicalDevice);
    //vulkan_texture_data* Data = (vulkan_texture_data*)texture->internal_data;
    if(Data) {
        Data->image.Destroy(VkAPI);
        MMemory::ZeroMem(&Data->image, sizeof(VulkanImage));
        vkDestroySampler(VkAPI->Device.LogicalDevice, Data->sampler, VkAPI->allocator);
        Data->sampler = 0;

    MMemory::TFree<VulkanTextureData>(Data, 1, MEMORY_TAG_TEXTURE);
    }
    
    //MMemory::ZeroMem(Texture, sizeof(Texture));
}

void *Texture::operator new(u64 size)
{
    return MMemory::Allocate(size, MEMORY_TAG_TEXTURE);
}

void Texture::operator delete(void *ptr)
{
    MMemory::Free(ptr, sizeof(Texture), MEMORY_TAG_TEXTURE);
}
