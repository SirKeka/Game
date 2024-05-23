#pragma once
#include "defines.hpp"

/// @brief Максимальное количество экземпляров элемента управления пользовательского интерфейса
/// @todo СДЕЛАТЬ: сделать настраиваемым
constexpr int VULKAN_MAX_UI_COUNT = 1024;

/// @brief Установите некоторые жесткие ограничения на количество поддерживаемых текстур, атрибутов, 
// униформ и т.д. Это необходимо для сохранения локальности памяти и предотвращения динамического распределения.
namespace VulkanShader
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
    u8 BindingCount;                                                    // Количество привязок в этом наборе.
    VkDescriptorSetLayoutBinding bindings[VulkanShader::MaxBindings];   // Массив макетов привязки для этого набора.
};

/// @brief Конфигурация внутреннего шейдера, созданная с помощью VulkanShader:Create().
struct VulkanShaderConfig {
    u8 StageCount;                                                              // Количество этапов в этом шейдере.
    VulkanShaderStageConfig stages[VulkanShader::MaxStages];                    // Конфигурация для каждого этапа этого шейдера.
    VkDescriptorPoolSize PoolSizes[2];                                          // Массив размеров пула дескрипторов.
    u16 MaxDescriptorSetCount;                                                  // Максимальное количество наборов дескрипторов, которые можно выделить из этого шейдера. Обычно должно быть достаточно большое число.
    u8 DescriptorSetCount;                                                      // Общее количество наборов дескрипторов, настроенных для этого шейдера. Имеет значение 1, если используются только глобальные униформы/сэмплеры; иначе 2.
    VulkanDescriptorSetConfig DescriptorSets[2];                                // Наборы дескрипторов, максимум 2. Индекс 0 = глобальный, 1 = экземпляр.
    VkVertexInputAttributeDescription attributes[VulkanShader::MaxAttributes];  // Массив описаний атрибутов для этого шейдера.

};

/**
 * @brief Represents a state for a given descriptor. This is used
 * to determine when a descriptor needs updating. There is a state
 * per frame (with a max of 3).
 */
typedef struct vulkan_descriptor_state {
    /** @brief The descriptor generation, per frame. */
    u8 generations[3];
    /** @brief The identifier, per frame. Typically used for texture ids. */
    u32 ids[3];
} vulkan_descriptor_state;

/**
 * @brief Represents the state for a descriptor set. This is used to track
 * generations and updates, potentially for optimization via skipping
 * sets which do not need updating.
 */
typedef struct vulkan_shader_descriptor_set_state {
    /** @brief The descriptor sets for this instance, one per frame. */
    VkDescriptorSet descriptor_sets[3];

    /** @brief A descriptor state per descriptor, which in turn handles frames. Count is managed in shader config. */
    vulkan_descriptor_state descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
} vulkan_shader_descriptor_set_state;

/**
 * @brief The instance-level state for a shader.
 */
typedef struct vulkan_shader_instance_state {
    /** @brief The instance id. INVALID_ID if not used. */
    u32 id;
    /** @brief The offset in bytes in the instance uniform buffer. */
    u64 offset;

    /** @brief  A state for the descriptor set. */
    vulkan_shader_descriptor_set_state descriptor_set_state;

    /**
     * @brief Instance texture pointers, which are used during rendering. These
     * are set by calls to set_sampler.
     */
    struct texture** instance_textures;
} vulkan_shader_instance_state;

/**
 * @brief Represents a generic Vulkan shader. This uses a set of inputs
 * and parameters, as well as the shader programs contained in SPIR-V
 * files to construct a shader for use in rendering.
 */
typedef struct vulkan_shader {
    /** @brief The block of memory mapped to the uniform buffer. */
    void* mapped_uniform_buffer_block;

    /** @brief The shader identifier. */
    u32 id;

    /** @brief The configuration of the shader generated by vulkan_create_shader(). */
    vulkan_shader_config config;

    /** @brief A pointer to the renderpass to be used with this shader. */
    vulkan_renderpass* renderpass;

    /** @brief An array of stages (such as vertex and fragment) for this shader. Count is located in config.*/
    vulkan_shader_stage stages[VULKAN_SHADER_MAX_STAGES];

    /** @brief The descriptor pool used for this shader. */
    VkDescriptorPool descriptor_pool;

    /** @brief Descriptor set layouts, max of 2. Index 0=global, 1=instance. */
    VkDescriptorSetLayout descriptor_set_layouts[2];
    /** @brief Global descriptor sets, one per frame. */
    VkDescriptorSet global_descriptor_sets[3];
    /** @brief The uniform buffer used by this shader. */
    vulkan_buffer uniform_buffer;

    /** @brief The pipeline associated with this shader. */
    vulkan_pipeline pipeline;

    /** @brief The instance states for all instances. @todo TODO: make dynamic */
    u32 instance_count;
    vulkan_shader_instance_state instance_states[VULKAN_MAX_MATERIAL_COUNT];

} vulkan_shader;