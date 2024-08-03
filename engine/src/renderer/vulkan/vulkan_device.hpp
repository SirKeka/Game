#pragma once

#include <vulkan/vulkan.h>

class VulkanAPI;

struct VulkanSwapchainSupportInfo 
{
    VkSurfaceCapabilitiesKHR capabilities;
    u32 FormatCount;
    VkSurfaceFormatKHR* formats;
    u32 PresentModeCount;
    VkPresentModeKHR* PresentModes;
};

class VulkanDevice
{
public:
    VkPhysicalDevice PhysicalDevice;             // Физическое устройство. Это представление самого графического процессора.
    VkDevice LogicalDevice;                      // Логическое устройство. Это представление устройства приложением, используемое для большинства операций Vulkan.
    VulkanSwapchainSupportInfo SwapchainSupport; // Информация о поддержке swapchain.
    i32 GraphicsQueueIndex;                      // Индекс графической очереди.
    i32 PresentQueueIndex;                       // Индекс текущей очереди.
    i32 TransferQueueIndex;                      // Индекс очереди передачи.
    bool SupportsDeviceLocalHostVisible;         // Указывает, поддерживает ли устройство тип памяти, который является как видимым для хоста, так и локальным для устройства.

    VkQueue GraphicsQueue;                       // Дескриптор очереди графики.
    VkQueue PresentQueue;                        // Дескриптор текущей очереди.
    VkQueue TransferQueue;                       // Дескриптор очереди передачи.
    
    VkCommandPool GraphicsCommandPool;           // Дескриптор пула команд для графических операций.

    VkPhysicalDeviceProperties properties;       // Свойства физического устройства.
    VkPhysicalDeviceFeatures features;           // Функции физического устройства.
    VkPhysicalDeviceMemoryProperties memory;     // Свойства памяти физического устройства.

    VkFormat DepthFormat;                        // Выбранный поддерживаемый формат глубины.
    u8 DepthChannelCount;                        // Количество каналов выбранного формата глубины.
public:
    VulkanDevice() : PhysicalDevice(), LogicalDevice(), SwapchainSupport(), GraphicsQueueIndex(), PresentQueueIndex(), TransferQueueIndex(), SupportsDeviceLocalHostVisible(), 
    GraphicsQueue(), PresentQueue(), TransferQueue(), GraphicsCommandPool(), properties(), features(), memory(), DepthFormat() {}
    ~VulkanDevice() = default;

    bool Create(VulkanAPI* VkAPI);

    void Destroy(VulkanAPI* VkAPI);

    void QuerySwapchainSupport(VkSurfaceKHR Surface);

    bool DetectDepthFormat();

private:
    struct VulkanPhysicalDeviceRequirements;
    struct VulkanPhysicalDeviceQueueFamilyInfo;

    bool SelectPhysicalDevice(VulkanAPI* VkAPI);
    bool PhysicalDeviceMeetsRequirements(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features,
        const VulkanPhysicalDeviceRequirements* requirements,
        VulkanPhysicalDeviceQueueFamilyInfo* OutQueueFamilyInfo,
        VulkanSwapchainSupportInfo* OutSwapchainSupport
    );


};
