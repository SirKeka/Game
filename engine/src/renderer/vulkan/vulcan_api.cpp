#include "vulcan_api.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"
//#include "core/mstring.h"

VulcanAPI::VulcanAPI(const char* ApplicationName)
{
    // TODO: пользовательский allocator.
    allocator = 0;

    // Настройка экземпляра Vulkan.
    VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    AppInfo.apiVersion = VK_API_VERSION_1_2;
    AppInfo.pApplicationName = ApplicationName;
    AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
    AppInfo.pEngineName = "Moon Engine";
    AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0); 

    VkInstanceCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    CreateInfo.pApplicationInfo = &AppInfo;
    CreateInfo.enabledExtensionCount = 0;
    CreateInfo.ppEnabledExtensionNames = 0;
    CreateInfo.enabledLayerCount = 0;
    CreateInfo.ppEnabledLayerNames = 0;

    VkResult result = vkCreateInstance(&CreateInfo, allocator, &instance);
    if(result != VK_SUCCESS) MERROR("vkCreateInstance не дал результата: %u", result);

    MINFO("Средство визуализации Vulkan успешно инициализировано.");
}

VulcanAPI::~VulcanAPI()
{
}

void VulcanAPI::ShutDown()
{
    this->~VulcanAPI();
}

void VulcanAPI::Resized(u16 width, u16 height)
{

}

bool VulcanAPI::BeginFrame(f32 Deltatime)
{
    return true;
}

bool VulcanAPI::EndFrame(f32 DeltaTime)
{
    return true;
}

void *VulcanAPI::operator new(u64 size)
{
    return MMemory::Allocate(size, MEMORY_TAG_RENDERER);
}

void VulcanAPI::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(VulcanAPI), MEMORY_TAG_RENDERER);
}
