#pragma once

#include <vulkan/vulkan.h>
#include "resources/texture.h"

class VulkanAPI;
struct VulkanCommandBuffer;

/// @brief Представление изображения Vulkan. Его можно рассматривать как текстуру. Также содержит вид и память, используемые внутренним изображением.
class VulkanImage
{
public:
    /// @brief Структура содержащая параметры для создания экземмпляра изображения Vulkan
    /// @param VkAPI указатель на контекст Vulkan.
    /// @param name имя изображения
    /// @param width ширина изображения. Для кубических карт это для каждой стороны куба.
    /// @param height высота изображения. Для кубических карт это для каждой стороны куба.
    /// @param CreateView указывает, следует ли создавать представление с изображением.
    /// @param format формат изображения.
    /// @param tiling режим тайлинга изображения.
    /// @param usage использование изображения.
    /// @param MemoryFlags флаги памяти для памяти, используемой изображением.
    /// @param ViewAspectFlags флаги аспекта, которые следует использовать при создании представления, если применимо.
    /// @param LayerCount количество слоев
    /// @param type тип текстуры. Дает подсказки по созданию.
    /// @param MipLevels количество уровней детализации(разрешения) изображения
    struct Config {
        VulkanAPI* VkAPI;
        MString name;
        u32 width;
        u32 height;
        b32 CreateView;
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags usage;
        VkMemoryPropertyFlags MemoryFlags;
        VkImageAspectFlags ViewAspectFlags;
        u16 LayerCount{1};
        TextureType type;
        u8 MipLevels{1};
    };
    
    VkImage handle{};                        // Дескриптор внутреннего объекта изображения.
    VkDeviceMemory memory{};                 // Память, используемая изображением.
    VkImageView view{};                      // Вид для изображения, который используется для доступа к изображению.
    VkImageView* LayerViews{};               // Если слоев несколько, здесь отображается только одно представление для каждого слоя.
    MString name;                            // Название изображения.
    VkMemoryRequirements MemoryRequirements; // Требования к памяти графического процессора для этого изображения.
    VkMemoryPropertyFlags MemoryFlags;       // Флаги свойств памяти 
    VkFormat format;                         // Формат изображения.
    u32 width = 0;                           // Ширина изображения.
    u32 height = 0;                          // Высота изображения.
    u16 LayerCount;                          // Количество слоев на этом изображении.
    u8 MipLevels{1};                         // Количество уровней детализации изображения, которые необходимо сгенерировать для данного ресурса изображения. Необходимо передать в текстуру, использующую его. Всегда должно быть не менее 1.
public:
    constexpr VulkanImage() = default;
    /// @brief Создает новое изображение Vulkan.
    /// @param config конфигурация изображения Vulkan
    VulkanImage(Config& config);
    ~VulkanImage();

    void Create(Config& config);
    
    /// @brief Создает представление для заданного изображения.
    /// @param config конфигурация изображения Vulkan
    void ViewCreate(Config &config, i32 LayerIndex, VkImageView& view);

    /// @brief Преобразует предоставленное изображение из OldLayout в NewLayout. 
    /// @param VkAPI указатель на контекст Vulkan.
    /// @param CommandBuffer указатель на буфер команд, который будет использоваться.
    /// @param format формат изображения.
    /// @param OldLayout старый макет.
    /// @param NewLayout новый макет.
    void TransitionLayout(
        VulkanAPI* VkAPI,
        VulkanCommandBuffer& CommandBuffer,
        VkFormat format,
        VkImageLayout OldLayout,
        VkImageLayout NewLayout
    );

    /// @brief Копирует данные из буфера в предоставленное изображение.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param image изображение, в которое нужно скопировать данные буфера.
    /// @param buffer буфер, данные которого будут скопированы.
    /// @param offset смещение в байтах от начала буфера.
    /// @param CommandBuffer 
    void CopyFromBuffer(VulkanAPI* VkAPI, VkBuffer buffer, u64 offset, VulkanCommandBuffer& CommandBuffer);

    /// @brief Копирует данные из предоставленного изображения в указанный буфер.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param buffer буфер для копирования.
    /// @param CommandBuffer буфер команд, который будет использоваться для копирования.
    void CopyToBuffer(VulkanAPI* VkAPI, VkBuffer buffer, VulkanCommandBuffer& CommandBuffer);

    /// @brief Копирует данные одного пикселя из указанного изображения в предоставленный буфер.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param buffer Буфер для копирования.
    /// @param x х-координата пикселя для копирования.
    /// @param y у-координата пикселя для копирования.
    /// @param CommandBuffer буфер команд, который будет использоваться для копирования.
    void CopyPixelToBuffer(VulkanAPI* VkAPI, VkBuffer buffer, u32 x, u32 y, VulkanCommandBuffer& CommandBuffer);

    /// @brief Функция генерирует уровни детализации изображения.
    /// @param VkAPI указатель на объект отрисовщика типа VulkanAPI.
    /// @param CommandBuffer ссылка на буфер команд, который будет использоваться.
    bool GenerateMipMaps(VulkanAPI* VkAPI, VulkanCommandBuffer& CommandBuffer);

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};
