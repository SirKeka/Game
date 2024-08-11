/// @file shader.hpp
/// @author 
/// @brief Класс шейдера. Отвечает за правильную отрисовку геометрии.
/// @version 1.0
/// @date 2024-05-18 
/// @copyright 
#pragma once

#include "containers/darray.hpp"
#include "containers/hashtable.hpp"

/// @brief Стадии шейдера, доступные в системе.
enum class ShaderStage {
    Vertex   = 0x00000001,
    Geometry = 0x00000002,
    Fragment = 0x00000004,
    Compute  = 0x00000008
};

/// @brief Доступные типы атрибутов.
enum class ShaderAttributeType {
    Float32   =  0U,
    Float32_2 =  1U,
    Float32_3 =  2U,
    Float32_4 =  3U,
    Matrix4   =  4U,
    Int8      =  5U,
    UInt8     =  6U,
    Int16     =  7U,
    UInt16    =  8U,
    Int32     =  9U,
    UInt32    = 10U
};

/// @brief Доступные виды униформы.
enum class ShaderUniformType {
    Float32   =   0U,
    Float32_2 =   1U,
    Float32_3 =   2U,
    Float32_4 =   3U,
    Int8      =   4U,
    UInt8     =   5U,
    Int16     =   6U,
    UInt16    =   7U,
    Int32     =   8U,
    UInt32    =   9U,
    Matrix4   =  10U,
    Sampler   =  11U,
    Custom    = 255U
};

/// @brief Определяет область шейдера, которая указывает, как часто он обновляется.
enum class ShaderScope {
    Global   = 0,                               // Глобальная область шейдера, обычно обновляется один раз для каждого кадра.
    Instance = 1,                               // Область действия шейдера экземпляра, обычно обновляемая «для каждого экземпляра» шейдера.
    Local    = 2                                // Область действия локального шейдера, обычно обновляется для каждого объекта.
};

/// @brief Конфигурация атрибута.
struct ShaderAttributeConfig {
    MString name;                               // Имя атрибута.
    u8 size;                                    // Размер атрибута.
    ShaderAttributeType type;                   // Тип атрибута.
    
    constexpr ShaderAttributeConfig() : name(), size(), type() {}
    constexpr ShaderAttributeConfig(ShaderAttributeConfig&& sac) : name(std::move(sac.name)), size(sac.size), type(sac.type) {
        size = 0;
        type = static_cast<ShaderAttributeType>(0);
    }
    ShaderAttributeConfig& operator =(const ShaderAttributeConfig& sac) {
        name = sac.name;
        size = sac.size;
        type = sac.type;
        return *this;
    }
    ShaderAttributeConfig& operator =(ShaderAttributeConfig&& sac) {
        name = std::move(sac.name);
        size = sac.size;
        type = sac.type;
        return *this;
    }
};

/// @brief Конфигурация униформы.
struct ShaderUniformConfig {
    // u8 NameLength;                           // Длина имени.
    MString name;                               // Название униформы.
    u8 size;                                    // Размер униформы.
    u32 location;                               // Расположение униформы.
    ShaderUniformType type;                     // Тип униформы.
    ShaderScope scope;                          // Область применения униформы.
    constexpr ShaderUniformConfig() : name(), size(), location(), type(), scope() {}
    constexpr ShaderUniformConfig(ShaderUniformConfig&& suc) : name(std::move(suc.name)), size(suc.size), location(suc.location), type(suc.type), scope(suc.scope) {
        suc.size = 0;
        suc.location = 0;
    }
    ShaderUniformConfig& operator =(const ShaderUniformConfig& suc) {
        name = suc.name;
        size = suc.size;
        location = suc.location;
        type = suc.type;
        scope = suc.scope;
        return *this;
    }
    ShaderUniformConfig& operator =(ShaderUniformConfig&& suc) {
        name = std::move(suc.name);
        size = suc.size;
        location = suc.location;
        type = suc.type;
        scope = suc.scope;
        return *this;
    }
};

/// @brief Конфигурация шейдера. Обычно создается и уничтожается загрузчиком
/// ресурсов шейдера и задается свойствами, найденными в файле ресурсов .shadercfg.
struct ShaderConfig {
    MString name{};                               // Имя создаваемого шейдера.
    bool UseInstances{false};                     // Указывает, использует ли шейдер униформы уровня экземпляра.
    bool UseLocal{false};                         // Указывает, использует ли шейдер униформы локального уровня.
    u8 AttributeCount{};                          // Количество атрибутов.
    DArray<ShaderAttributeConfig> attributes{};   // Коллекция атрибутов.
    u8 UniformCount{};                            // Учёт униформы.
    DArray<ShaderUniformConfig> uniforms{};       // Коллекция униформы.
    MString RenderpassName{};                     // Имя прохода рендеринга, используемого этим шейдером.
    u8 StageCount{};                              // Количество этапов, присутствующих в шейдере.
    DArray<ShaderStage> stages{};                 // Сборник этапов.
    DArray<MString> StageNames{};                 // Коллекция сценических имен. Должно соответствовать массиву этапов.
    DArray<MString> StageFilenames{};             // Коллекция имен файлов этапов, которые необходимо загрузить (по одному на этап). Должно соответствовать массиву этапов.

    ShaderConfig() : name(), UseInstances(false), UseLocal(false), AttributeCount(), attributes(), UniformCount(), uniforms(), RenderpassName(), StageCount(), stages(), StageNames(), StageFilenames() {}
    //~ShaderConfig() {}
    void* operator new(u64 size) {return MMemory::Allocate(size, MemoryTag::Resource);}
    void operator delete(void* ptr, u64 size) {MMemory::Free(ptr, size, MemoryTag::Resource);}
};

/// @brief Представляет текущее состояние данного шейдера.
enum class ShaderState {
    NotCreated,                                 // Шейдер еще не прошел процесс создания и непригоден для использования.
    Uninitialized,                              // Шейдер прошел процесс создания, но не инициализации. Это непригодно для использования.
    Initialized                                 // Шейдер создан, инициализирован и готов к использованию.
};

///@brief Представляет одну запись во внутреннем универсальном массиве.
struct ShaderUniform {
    u64 offset{};                                // Смещение в байтах от начала универсального набора (глобальное/экземплярное/локальное).
    u16 index{};                                 // Индекс во внутренний универсальный массив.
    u16 location{};                              // Местоположение, которое будет использоваться для поиска. Обычно совпадает с индексом, за исключением сэмплеров, которые используются для поиска индекса текстуры во внутреннем массиве в заданной области (глобальный/экземпляр).
    u16 size{};                                  // Размер униформы или 0 для сэмплеров.
    u8  SetIndex{};                              // Индекс набора дескрипторов, которому принадлежит униформа (0 = глобальный, 1 = экземпляр, INVALID::ID = локальный).
    ShaderScope scope{};                         // Область применения униформы.
    ShaderUniformType type{};                    // Тип униформы.
    constexpr ShaderUniform() : offset(), index(), location(), size(), SetIndex(), scope(), type() {}
    constexpr ShaderUniform(u16 index, u16 location, ShaderScope scope, ShaderUniformType type) : offset(), index(index), location(location), size(), SetIndex(), scope(scope), type(type) {}
};

/// @brief Представляет один атрибут вершины шейдера.
struct ShaderAttribute {
    MString name;                               // Имя атрибута.
    ShaderAttributeType type;                   // Тип атрибута.
    u32 size;                                   // Размер атрибута в байтах.
    constexpr ShaderAttribute() : name(), type(), size() {}
    constexpr ShaderAttribute(const MString& name, ShaderAttributeType type, u32 size) : name(name), type(type), size(size) {}
    constexpr ShaderAttribute(const ShaderAttribute& sa) : name(sa.name), type(sa.type), size(sa.size) {}
};  

/// @brief Представляет шейдер во внешнем интерфейсе.
class Shader {
    friend class ShaderSystem;                  // Система шейдеров является дружественным класом для шейдера
    friend class VulkanAPI;                     // Указание рендера в качестве друга для доступа к переменным класса.
    friend class MaterialSystem;
    u32 id{};                                   // Идентификатор шейдера
    MString name{};                             // Имя шейдера
    bool UseInstances{};                        // Указывает, использует ли шейдер экземпляры. В противном случае предполагается, что используются только глобальные униформы и сэмплеры.
    bool UseLocals{};                           // Указывает, используются ли локальные значения (обычно для матриц моделей и т. д.).
    u64 RequiredUboAlignment{};                 // Количество байтов, необходимое для выравнивания UBO. Это используется вместе с размером UBO для определения конечного шага, то есть того, насколько UBO располагаются в буфере. Например, требуемое выравнивание 256 означает, что шаг должен быть кратен 256 (верно для некоторых карт nVidia).
    u64 GlobalUboSize{};                        // Фактический размер объекта глобального универсального буфера.
    u64 GlobalUboStride{};                      // Шаг объекта глобального универсального буфера.
    u64 GlobalUboOffset{};                      // Смещение в байтах для глобального UBO от начала универсального буфера.
    u64 UboSize{};                              // Фактический размер объекта универсального буфера экземпляра.
    u64 UboStride{};                            // Шаг объекта универсального буфера экземпляра.
    u64 PushConstantSize{};                     // Общий размер всех диапазонов констант push вместе взятых.
    u64 PushConstantStride{};                   // Шаг константы нажатия, выровненный по 4 байтам, как того требует Vulkan.
    DArray<class TextureMap*> GlobalTextureMaps;// Массив глобальных указателей текстурных карт.
    u8 InstanceTextureCount{};                  // Количество экземпляров текстур.
    ShaderScope BoundScope{};                     
    u32 BoundInstanceID{};                      // Идентификатор привязанного в данный момент экземпляра.
    u32 BoundUboOffset{};                       // Смещение ubo текущего привязанного экземпляра.
    void* HashtableBlock{};                     // Блок памяти, используемый единой хеш-таблицей.
    HashTable<u16> UniformLookup;               // Хэш-таблица для хранения единого индекса/местоположений по имени.
    DArray<ShaderUniform> uniforms;             // Массив униформы в этом шейдере.
    DArray<ShaderAttribute> attributes;         // Массив атрибутов.
    ShaderState state{};                        // Внутреннее состояние шейдера.
    u8 PushConstantRangeCount{};                // Число диапазонов push-констант.
    Range PushConstantRanges[32]{};             // Массив диапазонов push-констант.
    u16 AttributeStride{};                      // Размер всех атрибутов вместе взятых, то есть размер вершины.
    u64 RenderFrameNumber{};                    // Используется для обеспечения того, чтобы глобальные переменные шейдера обновлялись только один раз за кадр.

    // ЗАДАЧА: Пока нет реализации DirectX храним указатель шейдера Vulkan.
    class VulkanShader* ShaderData;             // Непрозрачный указатель для хранения конкретных данных API средства рендеринга. Рендерер несет ответственность за создание и уничтожение этого.
public:
    constexpr Shader();
    Shader(u32 id, const ShaderConfig* config);
    ~Shader();

    bool Create(u32 id, const ShaderConfig* config);
    /// @brief Добавляет новый атрибут вершины. Должно быть сделано после инициализации шейдера.
    /// @param config конфигурация атрибута.
    /// @return True в случае успеха; в противном случае ложь.
    bool AddAttribute(const ShaderAttributeConfig& config);
    /// @brief Добавляет в шейдер новую униформу.
    /// @param config конфигурация униформы.
    /// @return True в случае успеха; в противном случае ложь.
    bool AddUniform(const ShaderUniformConfig& config);
    /// @brief Связывает глобальные ресурсы для использования и обновления.
    /// @return true в случае успеха, иначе false.
    bool BindGlobals();
    /// @brief Связывает ресурсы экземпляра для использования и обновления.
    /// @param InstanceID идентификатор экземпляра, который необходимо привязать.
    /// @return true в случае успеха, иначе false.
    bool BindInstance(u32 InstanceID);
    bool UniformAdd(const MString& UniformName, u32 size, ShaderUniformType type, ShaderScope scope, u32 SetLocation, bool IsSampler);
    bool UniformNameValid(const MString& UniformName);
    bool UniformAddStateValid();
    u16  UniformIndex(const char* UniformName);
    void Destroy();
    
    
};