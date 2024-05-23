#pragma once
#include "defines.hpp"

/// @brief Максимальное количество экземпляров элемента управления пользовательского интерфейса
/// @todo СДЕЛАТЬ: сделать настраиваемым
constexpr int VULKAN_MAX_UI_COUNT = 1024;

/// @brief Установите некоторые жесткие ограничения на количество поддерживаемых текстур, атрибутов, 
// униформ и т.д. Это необходимо для сохранения локальности памяти и предотвращения динамического распределения.
namespace VulkanShaderConstants
{
    constexpr int MaxStages = 8;            // Максимально допустимое количество этапов (таких как вершина, фрагмент, вычисление и т. д.).
    constexpr int MaxGlobalTextures = 31;   // Максимальное количество текстур, разрешенное на глобальном уровне.
    constexpr int MaxInstanceTextures = 31; // Максимальное количество текстур, разрешенное на уровне экземпляра.
    constexpr int MaxAttributes = 16;       // Максимально допустимое количество входных атрибутов вершин.
    constexpr int MaxUniforms = 128;        // Максимальное количество униформ и сэмплеров, разрешенное на глобальном, экземплярном и локальном уровнях вместе взятых. Вероятно, это больше, чем когда-либо понадобится.
    constexpr int MaxBindings = 32;         // Максимальное количество привязок на набор дескрипторов.
    constexpr int MaxPushConstRanges = 32;  // Максимальное количество диапазонов push-констант для шейдера.
} // namespace VulkanShader

/// @brief Конфигурация этапа шейдера, например вершина или фрагмент.
struct VulkanShaderStageConfig {
    VkShaderStageFlagBits stage;            // Битовый флаг этапа шейдера.
    char FileName[255];                     // Имя файла шейдера.
};
 
/// @brief Конфигурация набора дескрипторов.
struct VulkanDescriptorSetConfig {
    u8 BindingCount;                                                                    // Количество привязок в этом наборе.
    VkDescriptorSetLayoutBinding bindings[VulkanShaderConstants::MaxBindings];          // Массив макетов привязки для этого набора.
};

/// @brief Конфигурация внутреннего шейдера, созданная с помощью VulkanShader:Create().
struct VulkanShaderConfig {
    u8 StageCount;                                                                      // Количество этапов в этом шейдере.
    VulkanShaderStageConfig stages[VulkanShaderConstants::MaxStages];                   // Конфигурация для каждого этапа этого шейдера.
    VkDescriptorPoolSize PoolSizes[2];                                                  // Массив размеров пула дескрипторов.
    u16 MaxDescriptorSetCount;                                                          // Максимальное количество наборов дескрипторов, которые можно выделить из этого шейдера. Обычно должно быть достаточно большое число.
    u8 DescriptorSetCount;                                                              // Общее количество наборов дескрипторов, настроенных для этого шейдера. Имеет значение 1, если используются только глобальные униформы/сэмплеры; иначе 2.
    VulkanDescriptorSetConfig DescriptorSets[2];                                        // Наборы дескрипторов, максимум 2. Индекс 0 = глобальный, 1 = экземпляр.
    VkVertexInputAttributeDescription attributes[VulkanShaderConstants::MaxAttributes]; // Массив описаний атрибутов для этого шейдера.

};
 
/// @brief Представляет состояние данного дескриптора. 
/// Это используется для определения того, когда дескриптор нуждается в обновлении. 
/// Для каждого кадра существует состояние (максимум 3).
struct VulkanDescriptorState {
    u8 generations[3];  // Генерация дескриптора для каждого кадра.
    u32 ids[3];         // Идентификатор для каждого кадра. Обычно используется для идентификаторов текстур.
};

/// @brief Представляет состояние набора дескрипторов. 
/// Это используется для отслеживания поколений и обновлений, возможно, 
/// для оптимизации путем пропуска наборов, которые не требуют обновления.
struct VulkanShaderDescriptorSetState {
    VkDescriptorSet DescriptorSets[3];                                          // Дескриптор устанавливает для этого экземпляра по одному на кадр.
    VulkanDescriptorState DescriptorStates[VulkanShaderConstants::MaxBindings]; // Состояние дескриптора для каждого дескриптора, который, в свою очередь, обрабатывает кадры. Подсчет управляется в конфигурации шейдера.
};

/// @brief Состояние экземпляра шейдера.
struct VulkanShaderInstanceState {
    u32 id;                                             // Идентификатор экземпляра. INVALID::ID, если не используется.
    u64 offset;                                         // Смещение в байтах в универсальном буфере экземпляра. 
    VulkanShaderDescriptorSetState DescriptorSetState;  //  Состояние набора дескрипторов. 
    class Texture** InstanceTextures;                   // Указатели экземпляров текстур, которые используются во время рендеринга. Они устанавливаются вызовами SetSampler.
};
 
/// @brief Представляет универсальный шейдер Vulkan. 
/// При этом используется набор входных данных и параметров, а также программы шейдеров, 
/// содержащиеся в файлах SPIR-V, для создания шейдера для использования при рендеринге.
struct VulkanShader {
    
    void* MappedUniformBufferBlock;                                     // Блок памяти, сопоставленный с универсальным буфером.
    u32 id;                                                             // Идентификатор шейдера.
    VulkanShaderConfig config;                                          // Конфигурация шейдера, созданная функцией VulkanCreateShader().
    VulkanRenderpass* renderpass;                                       // Указатель на проход рендеринга, который будет использоваться с этим шейдером.
    VulkanShaderStage stages[VulkanShaderConstants::MaxStages];         // Массив этапов (таких как вершина и фрагмент) для этого шейдера. Количество находится в config.
    VkDescriptorPool DescriptorPool;                                    // Пул дескрипторов, используемый для этого шейдера.
    VkDescriptorSetLayout DescriptorSetLayouts[2];                      // Макеты набора дескрипторов, максимум 2. Индекс 0 = глобальный, 1 = экземпляр.
    VkDescriptorSet GlobalDescriptorSets[3];                            // Наборы глобальных дескрипторов, по одному на кадр.
    VulkanBuffer UniformBuffer;                                         // Универсальный буфер, используемый этим шейдером.
    VulkanPipeline pipeline;                                            // Конвейер, связанный с этим шейдером.
    u32 InstanceCount;                                                  // Экземпляр состояния для всех экземпляров. СДЕЛАТЬ: динамичным */
    VulkanShaderInstanceState InstanceStates[VULKAN_MAX_MATERIAL_COUNT];

};