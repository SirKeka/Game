#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>
#include "resources/texture.hpp"

class VulkanAPI;
struct VulkanCommandBuffer;

/// @brief Представление изображения Vulkan. Его можно рассматривать как текстуру. Также содержит вид и память, используемые внутренним изображением.
class VulkanImage
{
public:
    VkImage handle;                             // Дескриптор внутреннего объекта изображения.
    VkDeviceMemory memory;                      // Память, используемая изображением.
    VkImageView view;                           // Вид для изображения, который используется для доступа к изображению.
    VkMemoryRequirements MemoryRequirements;    // Требования к памяти графического процессора для этого изображения.
    VkMemoryPropertyFlags MemoryFlags;          // Флаги свойств памяти
    u32 width = 0;                              // Ширина изображения.
    u32 height = 0;                             // Высота изображения.
public:
    constexpr VulkanImage() : handle(), memory(), view(), MemoryRequirements(), MemoryFlags(), width(), height() {}
    constexpr VulkanImage(const VulkanImage& vi) : handle(vi.handle), memory(vi.memory), view(vi.view), MemoryRequirements(vi.MemoryRequirements), MemoryFlags(vi.MemoryFlags), width(vi.width), height(vi.height) {}
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
        VulkanCommandBuffer& CommandBuffer,
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

    /// @brief Копирует данные из предоставленного изображения в указанный буфер.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param type тип текстуры. Предоставляет подсказки по количеству слоев.
    /// @param buffer буфер для копирования.
    /// @param CommandBuffer буфер команд, который будет использоваться для копирования.
    void CopyToBuffer(
        VulkanAPI* VkAPI,
        TextureType type,
        VkBuffer buffer,
        VulkanCommandBuffer& CommandBuffer);

    /// @brief Копирует данные одного пикселя из указанного изображения в предоставленный буфер.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param type тип текстуры. Предоставляет подсказки по количеству слоев.
    /// @param buffer Буфер для копирования.
    /// @param x х-координата пикселя для копирования.
    /// @param y у-координата пикселя для копирования.
    /// @param CommandBuffer буфер команд, который будет использоваться для копирования.
    void CopyPixelToBuffer(
        VulkanAPI* VkAPI,
        TextureType type,
        VkBuffer buffer,
        u32 x,
        u32 y,
        VulkanCommandBuffer& CommandBuffer);

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
