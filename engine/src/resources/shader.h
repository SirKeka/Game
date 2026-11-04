/// @file shader.h
/// @author 
/// @brief Класс шейдера. Отвечает за правильную отрисовку геометрии.
/// @version 1.0
/// @date 2024-05-18 
/// @copyright 
#pragma once

#include "containers/darray.h"
#include "containers/hashtable.hpp"

struct ShaderConfig;

/// @brief Отображает uniform на текстурную карту/карты при получении ресурсов экземпляра.  
struct ShaderInstanceUniformTextureConfig {
    /// @brief Местоположение uniform для отображения.  
    u16 UniformLocation;
    /// @brief Количество текстурных карт, привязанных к uniform.  
    u32 TextureMapCount;
    /// @brief Массив указателей на текстурные карты, которые должны быть отображены на uniform.  
    struct TextureMap** TextureMaps;
};

/// @brief Представляет конфигурацию ресурсов текстурных карт и отображений на uniform, необходимых для данных шейдера на уровне экземпляра.  
struct ShaderInstanceResourceConfig {
    /// @brief Количество конфигураций uniform.  
    u32 UniformConfigCount;
    /// @brief Массив конфигураций uniform.
    ShaderInstanceUniformTextureConfig* UniformConfigs;
};

namespace PrimitiveTopology
{
    enum Type {
        // Тип топологии не определен. Недопустимо для создания шейдера.
        None = 0x00,
        // Список треугольников. По умолчанию, если ничего не определено.
        TriangleList = 0x01,
        TriangleStrip = 0x02,
        TriangleFan = 0x04,
        LineList = 0x08,
        LineStrip = 0x10,
        PointList = 0x20,
        MAX = PointList << 1
    };
} // namespace e

/// @brief Определяет режим отсечения граней во время рендеринга.
enum class FaceCullMode : u8 
{
    None = 0x0,         // Грани не отсеиваются.
    Front = 0x1,        // Отсеиваются только передние грани.
    Back = 0x2,         // Отсеиваются только задние грани.
    FrontAndBack = 0x3  // Отсеиваются как передние, так и задние грани.
};

using ShaderFlagBits = u32;

/// @brief Стадии шейдера, доступные в системе.
enum class ShaderStage : u8 
{
    Vertex   = 0x00000001,
    Geometry = 0x00000002,
    Fragment = 0x00000004,
    Compute  = 0x00000008
};

/// @brief Конфигурация этапа шейдера, например вершина или фрагмент.
struct ShaderStageConfig
{
    MString name;
    MString filename;
    MString source;
    ShaderStage stage;
};

/// @brief Доступные виды униформы.
enum class ShaderUniformType : u8 
{
    Float32, Float32_2, Float32_3, Float32_4,
    Int8, UInt8, Int16, UInt16, Int32, UInt32,
    Matrix4,
    Sampler1D, Sampler2D, Sampler3D, SamplerCube, Sampler1DArray, Sampler2DArray, SamplerCubeArray,
    Custom = 255
};

/// @brief Определяет область шейдера, которая указывает, как часто он обновляется.
enum class ShaderScope : u8 
{
    Global,   // Глобальная область шейдера, обычно обновляется один раз для каждого кадра.
    Instance, // Область действия шейдера экземпляра, обычно обновляемая «для каждого экземпляра» шейдера.
    Local     // Область действия локального шейдера, обычно обновляется для каждого объекта.
};

/// @brief Представляет один атрибут вершины шейдера.
struct ShaderAttribute {
    MString name;               // Имя атрибута.
    u32 size;                   // Размер атрибута в байтах.
    
    /// @brief Доступные типы атрибутов.
    enum Type : u8 {
        Float32, Float32_2, Float32_3, Float32_4, Matrix4, Int8, UInt8, Int16, UInt16, Int32, UInt32
    } type;   // Тип атрибута.
    constexpr ShaderAttribute(MString& name, u32 size, Type type) : name((MString&&)name), size(size), type(type) {}
};

/// @brief Конфигурация атрибута.
struct ShaderAttributeConfig {
    MString name;               // Имя атрибута.
    u8 size;                    // Размер атрибута.
    ShaderAttribute::Type type; // Тип атрибута.

    constexpr ShaderAttributeConfig() : name(), size(), type() {}
};

/// @brief Конфигурация униформы.
struct ShaderUniformConfig 
{
    MString name;           // Название униформы.
    u32 location;           // Расположение униформы.
    u32 ArrayLength;        // Длина массива, если параметр uniform является массивом.
    u16 size;               // Размер униформы. Если используется массив, это размер для каждого элемента.
    ShaderUniformType type; // Тип униформы.
    ShaderScope scope;      // Область применения униформы.
    constexpr ShaderUniformConfig() : name(), location(), ArrayLength(), size(), type(), scope() {}
};

/// @brief Представляет текущее состояние данного шейдера.
enum class ShaderState {
    NotCreated,     // Шейдер еще не прошел процесс создания и непригоден для использования.
    Uninitialized,  // Шейдер прошел процесс создания, но не инициализации. Это непригодно для использования.
    Initialized     // Шейдер создан, инициализирован и готов к использованию.
};

///@brief Представляет одну запись во внутреннем универсальном массиве.
struct ShaderUniform {
    u64 offset;             // Смещение в байтах от начала универсального набора (глобальное/экземплярное/локальное).
    u32 ArrayLength;        // Длина массива, если он равен единице; в противном случае — 0.
    u16 index;              // Индекс во внутренний универсальный массив.
    u16 location;           // Местоположение, которое будет использоваться для поиска. Обычно совпадает с индексом, за исключением сэмплеров, которые используются для поиска индекса текстуры во внутреннем массиве в заданной области (глобальный/экземпляр).
    u16 size;               // Размер униформы или 0 для сэмплеров.
    u8  SetIndex;           // Индекс набора дескрипторов, которому принадлежит униформа (0 = глобальный, 1 = экземпляр, INVALID::ID = локальный).
    ShaderScope scope;      // Область применения униформы.
    ShaderUniformType type; // Тип униформы.
};
     
/// @brief Представляет шейдер во внешнем интерфейсе.
struct MAPI Shader {
    enum Flags {
        None         = 0x0,
        DepthTest    = 0x1,
        DepthWrite   = 0x2,
        Wireframe    = 0x4,
        StencilTest  = 0x8,
        StencilWrite = 0x10,
        
    };

    u32 id;                                       // Идентификатор шейдера
    ShaderFlagBits flags;                         // 
    MString name;                                 // Имя шейдера
    DArray<struct TextureMap*> GlobalTextureMaps; // Массив глобальных указателей текстурных карт.
    DArray<ShaderUniform> uniforms;               // Массив униформы в этом шейдере.
    DArray<ShaderAttribute> attributes;           // Массив атрибутов.
    struct VulkanShader* ShaderData;              // Указатель для хранения конкретных данных API средства рендеринга. Рендерер несет ответственность за создание и уничтожение этого.
    void* HashtableBlock;                         // Блок памяти, используемый единой хеш-таблицей.
    HashTable<u16> UniformLookup;                 // Хэш-таблица для хранения единого индекса/местоположений по имени.

    u64 RequiredUboAlignment;                     // Количество байтов, необходимое для выравнивания UBO. Это используется вместе с размером UBO для определения конечного шага, то есть того, насколько UBO располагаются в буфере. Например, требуемое выравнивание 256 означает, что шаг должен быть кратен 256 (верно для некоторых карт nVidia).
    u64 GlobalUboSize;                            // Фактический размер объекта глобального универсального буфера.
    u64 GlobalUboStride;                          // Шаг объекта глобального универсального буфера.
    u64 GlobalUboOffset;                          // Смещение в байтах для глобального UBO от начала универсального буфера.
    u64 UboSize;                                  // Фактический размер объекта универсального буфера экземпляра.
    u64 UboStride;                                // Шаг объекта универсального буфера экземпляра.
    u64 LocalUboOffset;;                          //
    u64 LocalUboSize;                             // 
    u64 LocalUboStride;                           // 
    u64 RenderFrameNumber;                        // Используется для обеспечения того, чтобы глобальные переменные шейдера обновлялись только один раз за кадр.
    u32 TopologyTypes;                            // Типы топологий, используемые шейдером и его конвейером. См. PrimitiveTopologyType.
    u32 BoundInstanceID;                          // Идентификатор привязанного в данный момент экземпляра.
    u32 BoundUboOffset;                           // Смещение ubo текущего привязанного экземпляра.
    u8 InstanceTextureCount;                      // Количество экземпляров текстур.
    ShaderScope BoundScope;                       //       
    ShaderState state;                            // Внутреннее состояние шейдера.
    u16 AttributeStride;                          // Размер всех атрибутов вместе взятых, то есть размер вершины.

    u8 GlobalUniformCount;                        // Количество униформ, не относящихся к выборке, в глобальном масштабе.
    u8 GlobalUniformSamplerCount;                 // Количество глобальных униформ с сэмплерами.
    DArray<u32> GlobalSamplerIndices;             // Сохраняет единообразные индексы глобальных сэмплеров для быстрого поиска.
    
    u8 InstanceUniformCount;                      // Количество экземпляров, не являющихся образцами, в униформе.
    u8 InstanceUniformSamplerCount;               // Количество униформ сэмплера экземпляров.
    DArray<u32> InstanceSamplerIndices;           // Сохраняет единообразные индексы экземпляров сэмплеров для быстрого поиска.

    u8 LocalUniformCount;                         // Количество местных лиц, не участвовавших в отборе проб, в униформе.
    u8 ShaderStageCount;
    ShaderStageConfig* StageConfigs;
    u8 DrawIndex;                                 // Используется для обеспечения обновления глобальных переменных шейдера только один раз за отрисовку.
    bool IsWireframe;                             // Указывает, помечен ли шейдер в данный момент для использования каркасного режима.

#ifdef _DEBUG
    u32* ModuleWatchIDs;
#endif

    constexpr Shader() { MemorySystem::ZeroMem(this, sizeof(Shader)); }
    Shader(u32 id, ShaderConfig& config);
    ~Shader();

    /// @brief Создает новый шейдер.
    /// @param id идентификатор шейдера.
    /// @param config конфигурация на основе которой будет создан шейдер.
    /// @return true в случае успеха, иначе false.
    bool Create(u32 id, ShaderConfig& config);
    
    /// @brief Добавляет новый атрибут вершины. Должно быть сделано после инициализации шейдера.
    /// @param config конфигурация атрибута.
    /// @return True в случае успеха; в противном случае ложь.
    bool AddAttribute(ShaderAttributeConfig& config);
    
    /// @brief Добавляет в шейдер новую униформу.
    /// @param config конфигурация униформы.
    /// @return True в случае успеха; в противном случае ложь.
    bool AddUniform(const ShaderUniformConfig& config);
    
    /// @brief Связывает глобальные ресурсы для использования и обновления.
    /// @return true в случае успеха, иначе false.
    bool BindGlobals();

    ShaderUniform& GetUniform(u16 location) { return uniforms[location]; }
    ShaderUniform* GetUniform(const char* name);

    /// @brief Перезагружает указанный шейдер.
    /// @return Возвращает true при успешном выполнении, иначе — false.
    bool Reload();

    /// @brief 
    /// @param WireframeEnabled 
    /// @return 
    bool SetWireframe(bool WireframeEnabled);
    
    /// @brief Возвращает местоположение униформы с заданным именем, если она найдена.
    /// @param UniformName название формы, которую нужно найти.
    /// @return Если найдено, укажите местоположение в формате Uniform; в противном случае — INVALID::U16ID.
    u16  UniformLocation(const char* UniformName);
    void Destroy();
private:
    /// @brief Добавляет образец текстуры в шейдер. Должно быть сделано после инициализации шейдера.
    /// @param config конфигурация униформы.
    /// @return True в случае успеха; в противном случае ложь.
    bool AddSampler(const ShaderUniformConfig& config);
    bool UniformAdd(const ShaderUniformConfig& config, u32 location);
    bool UniformNameValid(const MString& UniformName);

    friend class ShaderSystem;
};

constexpr int sh = sizeof(Shader); // 352

/// @brief Конфигурация шейдера. Обычно создается и уничтожается загрузчиком
/// ресурсов шейдера и задается свойствами, найденными в файле ресурсов .shadercfg.
struct ShaderConfig 
{
    MString name;                              // Имя создаваемого шейдера.
    DArray<ShaderAttributeConfig> attributes;  // Коллекция атрибутов.
    DArray<ShaderUniformConfig> uniforms;      // Коллекция униформы.
    DArray<ShaderStageConfig> StageConfigs;    // Набор конфигураций этапов. ПРИМЕЧАНИЕ: инициализируется, как только известен счет.
    u32 TopologyTypes{1};                      // Типы топологии для конвейера шейдера. См. PrimitiveTopologyType. По умолчанию "triangle list", если не указано иное.
    ShaderFlagBits flags{};                    // Флаги, установленные для этого шейдера.
    u32 MaxInstances{1};                       // Максимально допустимое количество экземпляров. ПРИМЕЧАНИЕ: Это напрямую влияет на количество доступных ресурсов.
    FaceCullMode CullMode = FaceCullMode::Back;// Режим отбраковки лица, который будет использоваться. По умолчанию BACK, если не указано иное.

    void Clear();
    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Resource); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Resource); }
};