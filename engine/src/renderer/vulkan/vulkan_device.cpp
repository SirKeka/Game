#include "vulkan_device.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "containers/darray.hpp"
#include "containers/mstring.hpp"

struct VulkanPhysicalDeviceRequirements {
    bool graphics;
    bool present;
    bool compute;
    bool transfer;
    DArray<const char*> DeviceExtensionNames;
    bool SamplerAnisotropy;
    bool DiscreteGPU;
};

struct VulkanPhysicalDeviceQueueFamilyInfo {
    u32 GraphicsFamilyIndex;
    u32 PresentFamilyIndex;
    u32 ComputeFamilyIndex;
    u32 TransferFamilyIndex;
};

bool SelectPhysicalDevice(VulkanAPI* VkAPI);
bool PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties* properties,
    const VkPhysicalDeviceFeatures* features,
    const VulkanPhysicalDeviceRequirements* requirements,
    VulkanPhysicalDeviceQueueFamilyInfo* OutQueueFamilyInfo,
    VulkanSwapchainSupportInfo* OutSwapchainSupport);

bool VulkanDeviceCreate(VulkanAPI* VkAPI)
{
    if (!SelectPhysicalDevice(VkAPI)) {
        return false;
    }

    return true;
}

void VulkanDeviceDestroy()
{

}

bool SelectPhysicalDevice(VulkanAPI *VkAPI)
{
    u32 PhysicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, 0));
    if (PhysicalDeviceCount == 0) {
        MFATAL("Устройств, поддерживающих Vulkan, обнаружено не было.");
        return false;
    }

    VkPhysicalDevice PhysicalDevices[PhysicalDeviceCount];
    VK_CHECK(vkEnumeratePhysicalDevices(VkAPI->instance, &PhysicalDeviceCount, PhysicalDevices));
    for (u32 i : PhysicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(PhysicalDevices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(PhysicalDevices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevices[i], &memory);
    }
    
    // TODO: Эти требования, вероятно, должны определяться движком
        // конфигурация.
        VulkanPhysicalDeviceRequirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        // ПРИМЕЧАНИЕ: Включите это, если потребуются вычисления.
        // requirements.compute = true;
        requirements.SamplerAnisotropy = true;
        requirements.DiscreteGPU = true;
        requirements.DeviceExtensionNames.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VulkanPhysicalDeviceQueueFamilyInfo QueueInfo = {};
}

bool PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device, 
    VkSurfaceKHR surface, 
    const VkPhysicalDeviceProperties *properties, 
    const VkPhysicalDeviceFeatures *features, 
    const VulkanPhysicalDeviceRequirements *requirements, 
    VulkanPhysicalDeviceQueueFamilyInfo *OutQueueFamilyInfo, 
    VulkanSwapchainSupportInfo *OutSwapchainSupport)
{
    // Оцените свойства устройства, чтобы определить, соответствует ли оно потребностям нашего приложения.
    OutQueueFamilyInfo->ComputeFamilyIndex = -1;
    OutQueueFamilyInfo->GraphicsFamilyIndex = -1;
    OutQueueFamilyInfo->PresentFamilyIndex = -1;
    OutQueueFamilyInfo->TransferFamilyIndex = -1;

    // Discrete GPU?
    if (requirements->DiscreteGPU) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            MINFO("Устройство не является дискретным графическим процессором, а он необходим. Пропускаем.");
            return false;
        }
    }

    u32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, 0);
    VkQueueFamilyProperties QueueFamilies[QueueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &QueueFamilyCount, QueueFamilies);

    // Look at each queue and see what queues it supports
    MINFO("Graphics | Present | Compute | Transfer | Name");
    bool MinTransferScore = 255;
    for (u32 i : QueueFamilies) {
        bool CurrentTransferScore = 0;

    return false;
    }
}
