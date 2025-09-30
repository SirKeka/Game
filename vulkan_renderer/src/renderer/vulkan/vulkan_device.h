#pragma once

#include <vulkan/vulkan.h>
#include "containers/darray.h"

class VulkanAPI;
struct VulkanPhysicalDeviceRequirements;

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
    enum SupportFlagBits {
        NoneBit = 0x00,

        /// @brief Указывает, поддерживает ли устройство собственную динамическую топологию (т. е. * при использовании Vulkan API >= 1.3).
        NativeDynamicTopologyBit = 0x01,

        /// @brief Указывает, поддерживает ли устройство динамическую топологию. В противном случае рендереру потребуется сгенерировать отдельный конвейер для каждого типа топологии.
        DynamicTopologyBit = 0x02,
        LineSmoothRasterisationBit = 0x04,
        /// @brief Указывает, поддерживает ли устройство динамическую замену лицевой плоскости(face) (т.е. с использованием API Vulkan версии ≥ 1.3).
        NativeDynamicFrontFaceBit = 0x08,
        /// @brief Указывает, поддерживает ли устройство динамическую замену лицевой плоскости(face) на основе расширений.
        DynamicFrontFaceBit = 0x10,
    };

    /// @brief Побитовые флаги поддержки устройств. @see VulkanDevice::SupportFlagBits.
    using SupportFlags = u32;

    u32 ApiMaijor;                               // Поддерживаемая основная версия API на уровне устройства.
    u32 ApiMinor;                                // Поддерживаемая второстепенная версия API на уровне устройства.
    u32 ApiPatch;                                // Поддерживаемая версия исправления API на уровне устройства.

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

    SupportFlags supportFlags;                   // Указывает на поддержку различных функций.
public:
    VulkanDevice() : PhysicalDevice(), LogicalDevice(), SwapchainSupport(), GraphicsQueueIndex(), PresentQueueIndex(), TransferQueueIndex(), SupportsDeviceLocalHostVisible(), 
    GraphicsQueue(), PresentQueue(), TransferQueue(), GraphicsCommandPool(), properties(), features(), memory(), DepthFormat() {}
    ~VulkanDevice() = default;

    bool Create(VulkanAPI* VkAPI);

    void Destroy(VulkanAPI* VkAPI);

    void QuerySwapchainSupport(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, VulkanSwapchainSupportInfo* OutSupportInfo);

    bool DetectDepthFormat();

private:
    struct VulkanPhysicalDeviceQueueFamilyInfo {
        i32 GraphicsFamilyIndex;
        i32 PresentFamilyIndex;
        i32 ComputeFamilyIndex;
        i32 TransferFamilyIndex;
    };

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
