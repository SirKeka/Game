#pragma once
#include "vulkan_api.h"

/**
 * Возвращает строковое представление результата.
 * @param result результат, для которого нужно получить строку.
 * @param get_extended указывает, следует ли также возвращать расширенный результат.
 * @returns Код ошибки и/или расширенное сообщение об ошибке в строковой форме. По умолчанию успех для неизвестных типов результатов.
 */
const char* VulkanResultString(VkResult result, bool GetExtended);

/**
 * Указывает, является ли переданный результат успешным или ошибочным, как определено спецификацией Vulkan.
 * @returns True, если успех; в противном случае false. По умолчанию значение true для неизвестных типов результатов.
 */
bool VulkanResultIsSuccess(VkResult result);

#if defined(_DEBUG)
void VulkanSetDebugObjectName(VulkanAPI* VkAPI, VkObjectType ObjectType, void* ObjectHandle, const char* ObjectName);
void VulkanSetDebugObjectTag(VulkanAPI* VkAPI, VkObjectType ObjectType, void* ObjectHandle, u64 TagSize, const void* TagData);
void VulkanBeginLabel(VulkanAPI* VkAPI, VkCommandBuffer buffer, const char* LabelName, const FVec4& colour);
void VulkanEndLabel(VulkanAPI* VkAPI, VkCommandBuffer buffer);

#define VK_SET_DEBUG_OBJECT_NAME(VkAPI, ObjectType, ObjectHandle, ObjectName) VulkanSetDebugObjectName(VkAPI, ObjectType, ObjectHandle, ObjectName)
#define VK_SET_DEBUG_OBJECT_TAG(VkAPI, ObjectType, ObjectHandle, TagSize, TagData) VulkanSetDebugObjectTag(VkAPI, ObjectType, ObjectHandle, TagSize, TagData)
#define VK_BEGIN_DEBUG_LABEL(VkAPI, CommandBuffer, LabelName, colour) VulkanBeginLabel(VkAPI, CommandBuffer, LabelName, colour)
#define VK_END_DEBUG_LABEL(VkAPI, CommandBuffer) VulkanEndLabel(VkAPI, CommandBuffer)
#else
// Ничего не делает в неотладочных сборках.
#define VK_SET_DEBUG_OBJECT_NAME(VkAPI, ObjectType, ObjectHandle, ObjectName)
// Ничего не делает в неотладочных сборках.
#define VK_SET_DEBUG_OBJECT_TAG(VkAPI, ObjectType, ObjectHandle, TagSize, TagData)
// Ничего не делает в неотладочных сборках.
#define VK_BEGIN_DEBUG_LABEL(VkAPI, CommandBuffer, LabelName, colour)
// Ничего не делает в неотладочных сборках.
#define VK_END_DEBUG_LABEL(VkAPI, CommandBuffer)
#endif