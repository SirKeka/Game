#include "vulcan_api.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"

#include "containers/mstring.hpp"
#include "containers/darray.hpp"

#include "platform/platform.hpp"

VulcanAPI::VulcanAPI(const char* ApplicationName)
{
    // TODO: пользовательский allocator.
    allocator = 0;

    // Настройка экземпляра Vulkan.
    VkApplicationInfo AppInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    AppInfo.apiVersion = VK_API_VERSION_1_3;
    AppInfo.pApplicationName = ApplicationName;
    AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
    AppInfo.pEngineName = "Moon Engine";
    AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0); 

    VkInstanceCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    CreateInfo.pApplicationInfo = &AppInfo;

    // Получите список необходимых расширений
    
    DArray<const char*>RequiredExtensions;                              // Создаем массив указателей
    RequiredExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);         // Общее расширение поверхности
    //platform_get_required_extension_names(&required_extensions);      // Расширения для конкретной платформы
#if defined(_DEBUG)
    RequiredExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);     // утилиты отладки

    MDEBUG("Необходимые расширения:");
    u32 length = RequiredExtensions.Lenght();
    for (u32 i = 0; i < length; ++i) {
        MDEBUG(RequiredExtensions[i]);
    }
#endif

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
