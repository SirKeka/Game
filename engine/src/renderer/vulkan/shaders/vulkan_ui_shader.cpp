#include "vulkan_ui_shader.hpp"

VulkanUI_Shader::VulkanUI_Shader()
{
}

bool VulkanUI_Shader::Create(VulkanAPI *VkAPI)
{
    return false;
}

void VulkanUI_Shader::Destroy(VulkanAPI *VkAPI)
{
}

void VulkanUI_Shader::Use(VulkanAPI *VkAPI)
{
}

void VulkanUI_Shader::UpdateGlobalState(VulkanAPI *VkAPI, f32 DeltaTime)
{
}

void VulkanUI_Shader::SetModel(VulkanAPI *VkAPI, Matrix4D model)
{
}

void VulkanUI_Shader::ApplyMaterial(VulkanAPI *VkAPI, Material *material)
{
}

bool VulkanUI_Shader::AcquireResources(VulkanAPI *VkAPI, Material *material)
{
    return false;
}
