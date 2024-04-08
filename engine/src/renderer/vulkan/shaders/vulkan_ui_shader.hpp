#pragma once

class VulkanUI_Shader
{
private:
    /* data */
public:
    VulkanUI_Shader(/* args */);
    ~VulkanUI_Shader();
    
    
    bool Create(VulkanAPI* VkAPI);

    void Destroy(VulkanAPI* VkAPI);

    void Use(VulkanAPI* VkAPI);

    void UpdateGlobalState(VulkanAPI* VkAPI, f32 DeltaTime);

    void SetModel(VulkanAPI* VkAPI, Matrix4D model);
    void ApplyMaterial(VulkanAPI* VkAPI, Material* material);

    bool AcquireResources(VulkanAPI* VkAPI, Material* material);
    void ReleaseResources(VulkanAPI* VkAPI, Material* material);
};
