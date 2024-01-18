#pragma once

#include "renderer/renderer_types.hpp"

#include <vulkan/vulkan.h>

class VulcanAPI : public RendererType
{
private:
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    //bool IsInitialise;
public:
    VulcanAPI(const char* ApplicationName);
    ~VulcanAPI();
    
    // bool Initialize() override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    bool EndFrame(f32 DeltaTime) override;

    void* operator new(u64 size);
    void operator delete(void* ptr);
};
