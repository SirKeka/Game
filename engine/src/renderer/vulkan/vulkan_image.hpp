#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>

class VulkanAPI;
class VulkanCommandBuffer;

class VulkanImage
{
public:
    VkImage handle{};
    VkDeviceMemory memory{};
    VkImageView view{};
    u32 width = 0;
    u32 height = 0;
public:
    VulkanImage() = default;
    /// @brief Создает новое изображение Vulkan.
    /// @param VkAPI указатель на контекст Vulkan.
    /// @param type тип текстуры. Дает подсказки по созданию.
    /// @param width ширина изображения. Для кубических карт это для каждой стороны куба.
    /// @param height высота изображения. Для кубических карт это для каждой стороны куба.
    /// @param format формат изображения.
    /// @param tiling режим тайлинга изображения.
    /// @param usage использование изображения.
    /// @param MemoryFlags флаги памяти для памяти, используемой изображением.
    /// @param CreateView указывает, следует ли создавать представление с изображением.
    /// @param ViewAspectFlags флаги аспекта, которые следует использовать при создании представления, если применимо.
    VulkanImage(VulkanAPI* VkAPI,
        TextureType type,
        u32 width,
        u32 height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags MemoryFlags,
        b32 CreateView,
        VkImageAspectFlags ViewAspectFlags
    );
    ~VulkanImage();

    void Create(VulkanAPI* VkAPI,
        TextureType type,
        u32 width,
        u32 height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags MemoryFlags,
        b32 CreateView,
        VkImageAspectFlags ViewAspectFlags
    );
    /// @brief Создает представление для заданного изображения.
    /// @param VkAPI указатель на контекст Vulkan.
    /// @param type тип текстуры. Предоставляет подсказки для создания.
    /// @param format указатель на изображение, с которым нужно связать представление.
    /// @param AspectFlags флаги аспектов, которые следует использовать при создании представления, если применимо.
    void ViewCreate(VulkanAPI* VkAPI, TextureType type, VkFormat format, VkImageAspectFlags AspectFlags);

    /// @brief Преобразует предоставленное изображение из OldLayout в NewLayout. 
    /// @param VkAPI указатель на контекст Vulkan.
    /// @param type тип текстуры. Дает подсказки по созданию.
    /// @param CommandBuffer указатель на буфер команд, который будет использоваться.
    /// @param format формат изображения.
    /// @param OldLayout старый макет.
    /// @param NewLayout новый макет.
    void TransitionLayout(
        VulkanAPI* VkAPI,
        TextureType type,
        VulkanCommandBuffer* CommandBuffer,
        VkFormat format,
        VkImageLayout OldLayout,
        VkImageLayout NewLayout
    );

    /// @brief Копирует данные из буфера в предоставленное изображение.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param type тип текстуры. Дает подсказки по созданию.
    /// @param image изображение, в которое нужно скопировать данные буфера.
    /// @param buffer буфер, данные которого будут скопированы.
    /// @param CommandBuffer 
    void CopyFromBuffer(VulkanAPI* VkAPI, TextureType type, VkBuffer buffer, VulkanCommandBuffer* CommandBuffer);

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
