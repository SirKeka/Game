#pragma once

#include "vulkan_pipeline.h"
#include "renderer/renderbuffer.h"

/// @todo ЗАДАЧА: сделать настраиваемым
constexpr int VULKAN_MAX_UI_COUNT = 1024;       // Максимальное количество экземпляров элемента управления пользовательского интерфейса
constexpr int VULKAN_MAX_MATERIAL_COUNT = 1024; // Максимальное число экземпляров материала.

/// @brief Установите некоторые жесткие ограничения на количество поддерживаемых текстур, атрибутов, 
// униформ и т.д. Это необходимо для сохранения локальности памяти и предотвращения динамического распределения.
namespace VulkanShaderConstants
{
    constexpr int MaxStages = 8;            // Максимально допустимое количество этапов (таких как вершина, фрагмент, вычисление и т. д.).
    constexpr int MaxGlobalTextures = 31;   // Максимальное количество текстур, разрешенное на глобальном уровне.
    constexpr int MaxInstanceTextures = 31; // Максимальное количество текстур, разрешенное на уровне экземпляра.
    constexpr int MaxAttributes = 16;       // Максимально допустимое количество входных атрибутов вершин.
    constexpr int MaxUniforms = 128;        // Максимальное количество униформ и сэмплеров, разрешенное на глобальном, экземплярном и локальном уровнях вместе взятых. Вероятно, это больше, чем когда-либо понадобится.
    constexpr int MaxBindings = 2;          // Максимальное количество привязок на набор дескрипторов.
    constexpr int MaxPushConstRanges = 32;  // Максимальное количество диапазонов push-констант для шейдера.
} // namespace VulkanShader

/// @brief Представляет один этап шейдера.
struct VulkanShaderStage {
    VkShaderModuleCreateInfo CreateInfo;                    // Информация о создании шейдерного модуля.
    VkShaderModule handle;                                  // Дескриптор внутреннего шейдерного модуля.
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;  // Информация о создании этапа шейдера конвейера.
};
 
/// @brief Конфигурация набора дескрипторов.
struct VulkanDescriptorSetConfig {
    VkDescriptorSetLayoutBinding* bindings{}; // Массив макетов привязки для этого набора.
    u8 BindingCount;                          // Количество привязок в этом наборе.
    u8 SamplerBindingIndexStart;              // Начальный индекс привязок сэмплера.
};
 
/// @brief Представляет состояние данного дескриптора. 
/// Это используется для определения того, когда дескриптор нуждается в обновлении. 
/// Для каждого кадра существует состояние (максимум 3).
struct VulkanDescriptorState {
    u32 ids[3];         // Идентификатор для каждого кадра. Обычно используется для идентификаторов текстур.
    u8 generations[3];  // Генерация дескриптора для каждого кадра.
};

struct VulkanUniformSamplerState {
    struct ShaderUniform* uniform{};
    struct TextureMap** UniformTextureMaps{};  // Указатели экземпляров текстурных карт, которые используются во время рендеринга. Они устанавливаются вызовами SetSampler.
    VulkanDescriptorState* DescriptorStates{}; // Состояние дескриптора для каждого дескриптора, которое, в свою очередь, обрабатывает кадры. Количество кадров управляется в конфигурации шейдера.
};

/// @brief Состояние экземпляра шейдера.
struct VulkanShaderInstanceState {
    u64 offset;                                   // Смещение в байтах в универсальном буфере экземпляра. 
    VulkanUniformSamplerState* SamplerUniforms{}; // Сопоставление униформ сэмплера с дескрипторами и картами текстур.
    // ЗАДАЧА: обрабатывать количество кадров, отличное от 3.
    VkDescriptorSet DescriptorSets[3];            // Дескриптор устанавливается для этого экземпляра, по одному на кадр.
    u32 id{INVALID::ID};                          // Идентификатор экземпляра. INVALID::ID, если не используется.
    VulkanDescriptorState UboDescriptorState;     // дескриптор UBO
};
 
/// @brief Представляет универсальный шейдер Vulkan. 
/// При этом используется набор входных данных и параметров, а также программы шейдеров, 
/// содержащиеся в файлах SPIR-V, для создания шейдера для использования при рендеринге.
struct VulkanShader {
    u32 id;                                                                             // Идентификатор шейдера.
    u16 MaxDescriptorSetCount;                                                          // Максимальное количество наборов дескрипторов, которые можно выделить из этого шейдера. Обычно должно быть достаточно большим.
    u8 DescriptorSetCount;                                                              // Общее количество наборов дескрипторов, настроенных для этого шейдера. 1, если используются только глобальные униформы/сэмплеры; в противном случае 2.
    VulkanDescriptorSetConfig DescriptorSets[2];                                        // Наборы дескрипторов, максимум 2. Индекс 0=глобальный, 1=экземпляр
    void* MappedUniformBufferBlock;                                                     // Блок памяти, сопоставленный с универсальным буфером.
    void* LocalPushConstantBlock;                                                       // Блок памяти, используемый для констант push, 128Б.
    VulkanPipeline** pipelines;                                                         // Массив указателей на конвейеры, связанные с этим шейдером.
    VulkanPipeline** WireframePipelines;                                                // Массив указателей на конвейеры каркасного рендеринга, связанные с этим шейдером.
    VkPrimitiveTopology CurrentTopology;                                                // Текущая выбранная топология.
    FaceCullMode CullMode;                                                              // Режим отбраковки лиц, предоставляемый передней частью.
    u8 BoundPipelineIndex;                                                              // Текущий связанный индекс конвеера.
    u8 StageCount;                                                                      // Количество стадий шейдера в этом шейдере.
    VkVertexInputAttributeDescription attributes[VulkanShaderConstants::MaxAttributes]; // Массив описаний атрибутов для этого шейдера.
    VulkanShaderStage stages[VulkanShaderConstants::MaxStages];                         // Массив этапов (таких как вершина и фрагмент) для этого шейдера. Количество находится в config.
    u32 MaxInstances;                                                                   // Максимальное количество экземпляров.
    u32 PoolSizeCount;                                                                  // 
    VkDescriptorPoolSize PoolSizes[2];                                                  // Массив размеров пула дескрипторов.
    VkDescriptorSetLayout DescriptorSetLayouts[2];                                      // Макеты набора дескрипторов, максимум 2. Индекс 0 = глобальный, 1 = экземпляр.
    VkDescriptorSet GlobalDescriptorSets[3];                                            // Наборы глобальных дескрипторов, по одному на кадр.
    VkDescriptorPool DescriptorPool;                                                    // Пул дескрипторов, используемый для этого шейдера.
    RenderBuffer UniformBuffer;                                                         // Универсальный буфер, используемый этим шейдером.
    VulkanRenderpass* renderpass;                                                       // Указатель на проход рендеринга, который будет использоваться с этим шейдером.
    VulkanDescriptorState GlobalUboDescriptorState;                                     // Дескриптор UBO
    VulkanUniformSamplerState* GlobalSamplerUniforms;                                   // Сопоставление униформ сэмплера с дескрипторами и картами текстур.
    VulkanShaderInstanceState* InstanceStates;                                          // Состояния экземпляров для всех экземпляров. ЗАДАЧА: сделать динамическим

    VulkanShader(u32 MaxInstances) : LocalPushConstantBlock(MemorySystem::Allocate(128, Memory::Renderer)), MaxInstances(MaxInstances),
    InstanceStates(new(MemorySystem::Allocate(sizeof(VulkanShaderInstanceState) * MaxInstances, Memory::Array))VulkanShaderInstanceState[MaxInstances]) {}

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer, true); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
};
