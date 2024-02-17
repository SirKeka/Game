#pragma once

#include "renderer/renderer_types.hpp"
#include "core/asserts.hpp"

#include <vulkan/vulkan.h>

// Проверяет возвращаемое значение данного выражения на соответствие VK_SUCCESS.
#define VK_CHECK(expr)               \
    {                                \
        MASSERT(expr == VK_SUCCESS); \
    }

class VulkanAPI : public RendererType
{
public:
    struct VulkanDevice
    {
        VkPhysicalDevice PhysicalDevice;
        VkDevice LogicalDevice;
    };
    

    static VkInstance instance;
    static VkAllocationCallbacks* allocator;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif
    //bool IsInitialise;
public:
    VulkanAPI(const char* ApplicationName);
    ~VulkanAPI();
    
    // bool Initialize() override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    bool EndFrame(f32 DeltaTime) override;

    void* operator new(u64 size);
    void operator delete(void* ptr);
};
