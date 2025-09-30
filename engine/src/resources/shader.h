/// @file shader.h
/// @author 
/// @brief Класс шейдера. Отвечает за правильную отрисовку геометрии.
/// @version 1.0
/// @date 2024-05-18 
/// @copyright 
#pragma once

#include "containers/darray.h"
#include "containers/hashtable.hpp"
#include "resources/texture.hpp"

struct ShaderConfig;

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
    
/// @brief Представляет шейдер во внешнем интерфейсе.
struct MAPI Shader {
    enum Flags {
        NoneFlag       = 0x0,
        DepthTestFlag  = 0x1,
        DepthWriteFlag = 0x2,
        WireframeFlag  = 0x4
    };

    using FlagBits = u32;

    /// @brief Стадии шейдера, доступные в системе.
    enum class Stage {
        Vertex   = 0x00000001,
        Geometry = 0x00000002,
        Fragment = 0x00000004,
        Compute  = 0x00000008
    };

    /// @brief Доступные типы атрибутов.
    enum AttributeType {
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
    enum class UniformType {
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
    enum class Scope {
        Global   = 0,    // Глобальная область шейдера, обычно обновляется один раз для каждого кадра.
        Instance = 1,    // Область действия шейдера экземпляра, обычно обновляемая «для каждого экземпляра» шейдера.
        Local    = 2     // Область действия локального шейдера, обычно обновляется для каждого объекта.
    };

    /// @brief Конфигурация атрибута.
    struct AttributeConfig {
        MString name;         // Имя атрибута.
        u8 size;              // Размер атрибута.
        AttributeType type;   // Тип атрибута.

        constexpr AttributeConfig() : name(), size(), type() {}
        constexpr AttributeConfig(AttributeConfig&& sac) : name(static_cast<MString&&>(sac.name)), size(sac.size), type(sac.type) {
            size = 0;
        }
        AttributeConfig& operator =(const AttributeConfig& sac) {
            name = sac.name;
            size = sac.size;
            type = sac.type;
            return *this;
        }
        AttributeConfig& operator =(AttributeConfig&& sac) {
            name = static_cast<MString&&>(sac.name);
            size = sac.size;
            type = sac.type;
            return *this;
        }
    };

    /// @brief Конфигурация униформы.
    struct UniformConfig {
        MString name;       // Название униформы.
        u16 size;            // Размер униформы.
        u32 location;       // Расположение униформы.
        UniformType type;   // Тип униформы.
        Scope scope;        // Область применения униформы.
        constexpr UniformConfig() : name(), size(), location(), type(), scope() {}
        constexpr UniformConfig(UniformConfig&& suc) : name(static_cast<MString&&>(suc.name)), size(suc.size), location(suc.location), type(suc.type), scope(suc.scope) {
            suc.size = 0;
            suc.location = 0;
        }
        UniformConfig& operator =(const UniformConfig& suc) {
            name = suc.name;
            size = suc.size;
            location = suc.location;
            type = suc.type;
            scope = suc.scope;
            return *this;
        }
        UniformConfig& operator =(UniformConfig&& suc) {
            name = static_cast<MString&&>(suc.name);
            size = suc.size;
            location = suc.location;
            type = suc.type;
            scope = suc.scope;
            return *this;
        }
    };

    /// @brief Представляет текущее состояние данного шейдера.
    enum class State {
        NotCreated,                            // Шейдер еще не прошел процесс создания и непригоден для использования.
        Uninitialized,                         // Шейдер прошел процесс создания, но не инициализации. Это непригодно для использования.
        Initialized                            // Шейдер создан, инициализирован и готов к использованию.
    };

    ///@brief Представляет одну запись во внутреннем универсальном массиве.
    struct Uniform {
        u64 offset;         // Смещение в байтах от начала универсального набора (глобальное/экземплярное/локальное).
        u16 index;          // Индекс во внутренний универсальный массив.
        u16 location;       // Местоположение, которое будет использоваться для поиска. Обычно совпадает с индексом, за исключением сэмплеров, которые используются для поиска индекса текстуры во внутреннем массиве в заданной области (глобальный/экземпляр).
        u16 size;           // Размер униформы или 0 для сэмплеров.
        u8  SetIndex;       // Индекс набора дескрипторов, которому принадлежит униформа (0 = глобальный, 1 = экземпляр, INVALID::ID = локальный).
        Scope scope;        // Область применения униформы.
        UniformType type;   // Тип униформы.
    };
    
    /// @brief Представляет один атрибут вершины шейдера.
    struct Attribute {
        MString name;                      // Имя атрибута.
        AttributeType type;                // Тип атрибута.
        u32 size;                          // Размер атрибута в байтах.
        constexpr Attribute(MString& name, AttributeType type, u32 size) : name(static_cast<MString&&>(name)), type(type), size(size) {}
    };

    u32 id                                      {}; // Идентификатор шейдера
    MString name                                {}; // Имя шейдера
    FlagBits flags                              {}; // 
    u32 TopologyTypes                           {}; // Типы топологий, используемые шейдером и его конвейером. См. PrimitiveTopologyType.
    u64 RequiredUboAlignment                    {}; // Количество байтов, необходимое для выравнивания UBO. Это используется вместе с размером UBO для определения конечного шага, то есть того, насколько UBO располагаются в буфере. Например, требуемое выравнивание 256 означает, что шаг должен быть кратен 256 (верно для некоторых карт nVidia).
    u64 GlobalUboSize                           {}; // Фактический размер объекта глобального универсального буфера.
    u64 GlobalUboStride                         {}; // Шаг объекта глобального универсального буфера.
    u64 GlobalUboOffset                         {}; // Смещение в байтах для глобального UBO от начала универсального буфера.
    u64 UboSize                                 {}; // Фактический размер объекта универсального буфера экземпляра.
    u64 UboStride                               {}; // Шаг объекта универсального буфера экземпляра.
    u64 PushConstantSize                        {}; // Общий размер всех диапазонов констант push вместе взятых.
    u64 PushConstantStride                      {}; // Шаг константы нажатия, выровненный по 4 байтам, как того требует Vulkan.
    DArray<struct TextureMap*> GlobalTextureMaps{}; // Массив глобальных указателей текстурных карт.
    u8 InstanceTextureCount                     {}; // Количество экземпляров текстур.
    Scope BoundScope                            {};       
    u32 BoundInstanceID                         {}; // Идентификатор привязанного в данный момент экземпляра.
    u32 BoundUboOffset                          {}; // Смещение ubo текущего привязанного экземпляра.
    void* HashtableBlock                        {}; // Блок памяти, используемый единой хеш-таблицей.
    HashTable<u16> UniformLookup                {}; // Хэш-таблица для хранения единого индекса/местоположений по имени.
    DArray<Uniform> uniforms                    {}; // Массив униформы в этом шейдере.
    DArray<Attribute> attributes                {}; // Массив атрибутов.
    State state                                 {}; // Внутреннее состояние шейдера.
    u8 PushConstantRangeCount                   {}; // Число диапазонов push-констант.
    Range PushConstantRanges[32]                {}; // Массив диапазонов push-констант.
    u16 AttributeStride                         {}; // Размер всех атрибутов вместе взятых, то есть размер вершины.
    u64 RenderFrameNumber                       {}; // Используется для обеспечения того, чтобы глобальные переменные шейдера обновлялись только один раз за кадр.
    u8 DrawIndex                                {}; // Используется для обеспечения обновления глобальных переменных шейдера только один раз за отрисовку.

    // ЗАДАЧА: Пока нет реализации DirectX храним указатель шейдера Vulkan.
    struct VulkanShader* ShaderData   {};        // Непрозрачный указатель для хранения конкретных данных API средства рендеринга. Рендерер несет ответственность за создание и уничтожение этого.
public:
    constexpr Shader();
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
    bool AddAttribute(AttributeConfig& config);
    /// @brief Добавляет в шейдер новую униформу.
    /// @param config конфигурация униформы.
    /// @return True в случае успеха; в противном случае ложь.
    bool AddUniform(const UniformConfig& config);
    /// @brief Связывает глобальные ресурсы для использования и обновления.
    /// @return true в случае успеха, иначе false.
    bool BindGlobals();
    
    bool UniformAdd(const MString& UniformName, u32 size, UniformType type, Scope scope, u32 SetLocation, bool IsSampler);
    bool UniformNameValid(const MString& UniformName);
    bool UniformAddStateValid();
    u16  UniformIndex(const char* UniformName);
    void Destroy();
};

/// @brief Конфигурация шейдера. Обычно создается и уничтожается загрузчиком
    /// ресурсов шейдера и задается свойствами, найденными в файле ресурсов .shadercfg.
    struct ShaderConfig {
        MString name;                       // Имя создаваемого шейдера.
        FaceCullMode CullMode;              // Режим отбраковки лица, который будет использоваться. По умолчанию BACK, если не указано иное.
        u32 TopologyTypes;                  // Типы топологии для конвейера шейдера. См. PrimitiveTopologyType. По умолчанию "triangle list", если не указано иное.
        // u8 AttributeCount;                  // Количество атрибутов.
        DArray<Shader::AttributeConfig> attributes; // Коллекция атрибутов.
        // u8 UniformCount;                    // Учёт униформы.
        DArray<Shader::UniformConfig> uniforms;     // Коллекция униформы.
        // u8 StageCount;                      // Количество этапов, присутствующих в шейдере.
        DArray<Shader::Stage> stages;               // Сборник этапов.
        DArray<MString> StageNames;         // Коллекция сценических имен. Должно соответствовать массиву этапов.
        DArray<MString> StageFilenames;     // Коллекция имен файлов этапов, которые необходимо загрузить (по одному на этап). Должно соответствовать массиву этапов.
        Shader::FlagBits flags;             // Флаги, установленные для этого шейдера.

        ShaderConfig() : name(), CullMode(FaceCullMode::Back), TopologyTypes(PrimitiveTopology::Type::TriangleList), /*AttributeCount(),*/ attributes(), /*UniformCount(),*/ uniforms(), /*StageCount(),*/ stages(), StageNames(), StageFilenames(), flags() {}
        void Clear();
        void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Resource); }
        void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Resource); }
    };