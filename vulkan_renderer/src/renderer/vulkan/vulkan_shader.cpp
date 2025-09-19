#include "vulkan_shader.h"
#include "core/memory_system.h"

VulkanShader::VulkanShader()
:
    MappedUniformBufferBlock(nullptr),
    id(),
    config(),
    renderpass(nullptr),
    stages(),
    DescriptorPool(),
    DescriptorSetLayouts(),
    GlobalDescriptorSets(),
    UniformBuffer(),
    pipelines(nullptr),
    InstanceCount(),
    InstanceStates() 
{}

VulkanShader::~VulkanShader()
{
}

void *VulkanShader::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}
