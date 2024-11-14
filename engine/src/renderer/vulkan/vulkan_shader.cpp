#include "vulkan_shader.hpp"
#include "core/mmemory.hpp"

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
    pipeline(),
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
