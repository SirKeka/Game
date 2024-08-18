#pragma once

#include "defines.hpp"
#include "resources/geometry.hpp"
#include "resources/material/material.hpp"

struct GeometryConfig {
    u32 VertexSize;
    u32 VertexCount;
    void* vertices;
    u32 IndexSize;
    u32 IndexCount;
    void* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char MaterialName[MATERIAL_NAME_MAX_LENGTH];

    FVec3 center;
    FVec3 MinExtents;
    FVec3 MaxExtents;

    constexpr GeometryConfig() 
    : VertexSize(), VertexCount(), vertices(nullptr), IndexSize(), IndexCount(), indices(nullptr), name(), MaterialName(), center(), MinExtents(), MaxExtents() {}
    constexpr GeometryConfig(u32 VertexSize, u32 VertexCount, void* vertices, u32 IndexSize, u32 IndexCount, void* indices, const char* name, const char* MaterialName)
    : VertexSize(VertexSize), 
    VertexCount(VertexCount), 
    vertices(vertices), 
    IndexSize(IndexSize), 
    IndexCount(IndexCount), 
    indices(indices), 
    name(), 
    MaterialName() {
        for (u64 i = 0, j = 0; i < 256;) {
            if(name[i]) {
                this->name[i] = name[i];
                i++;
            }
            if(MaterialName[j]) {
                this->MaterialName[j] = MaterialName[j];
                j++;
            }
            if (!name[i] && !MaterialName[j]) {
                break;
            }
        }
    }
    constexpr GeometryConfig(u32 VertexSize, u32 VertexCount, void* vertices, u32 IndexSize, u32 IndexCount, void* indices, const char* name, const char* MaterialName, FVec3 center, FVec3 MinExtents, FVec3 MaxExtents)
    : 
    VertexSize(VertexSize), 
    VertexCount(VertexCount), 
    vertices(vertices), 
    IndexSize(IndexSize), 
    IndexCount(IndexCount), 
    indices(indices), 
    name(), 
    MaterialName(), 
    center(center), 
    MinExtents(MinExtents), 
    MaxExtents(MaxExtents)  {
        for (u64 i = 0, j = 0; i < 256;) {
            if(name[i]) {
                this->name[i] = name[i];
                i++;
            }
            if(MaterialName[j]) {
                this->MaterialName[j] = MaterialName[j];
                j++;
            }
            if (!name[i] && !MaterialName[j]) {
                break;
            }
        }
    }
    constexpr GeometryConfig(const GeometryConfig& conf)
    : VertexSize(conf.VertexSize), VertexCount(conf.VertexCount), vertices(MMemory::Allocate(VertexCount * VertexSize, Memory::Array)), IndexSize(conf.IndexSize), IndexCount(conf.IndexCount), indices(MMemory::Allocate(IndexCount * IndexSize, Memory::Array)), name(), MaterialName(), center(conf.center), MinExtents(conf.MinExtents), MaxExtents(conf.MaxExtents) {
        MMemory::CopyMem(vertices, conf.vertices, VertexCount * VertexSize);
        MMemory::CopyMem(indices, conf.indices, IndexCount * IndexSize);
        CopyNames(conf.name, conf.MaterialName);
    }
    constexpr GeometryConfig(GeometryConfig&& conf) : VertexSize(conf.VertexSize), VertexCount(conf.VertexCount), vertices(conf.vertices), IndexSize(conf.IndexSize), IndexCount(conf.IndexCount), indices(conf.indices), name(), MaterialName(), center(conf.center), MinExtents(conf.MinExtents), MaxExtents(conf.MaxExtents) {
        CopyNames(conf.name, conf.MaterialName);
        conf.VertexSize = 0;
        conf.VertexCount = 0;
        conf.vertices = nullptr;
        conf.IndexSize = 0;
        conf.IndexCount = 0;
        conf.indices = nullptr;
        conf.center = FVec3();
        conf.MinExtents = FVec3();
        conf.MaxExtents = FVec3();
    }
    GeometryConfig& operator =(const GeometryConfig& conf) {
        VertexSize = conf.VertexSize;
        VertexCount = conf.VertexCount;
        vertices = MMemory::Allocate(VertexCount * VertexSize, Memory::Array);
        MMemory::CopyMem(vertices, conf.vertices, VertexCount * VertexSize);
        IndexSize = conf.IndexSize;
        IndexCount = conf.IndexCount;
        indices = MMemory::Allocate(IndexCount * IndexSize, Memory::Array);
        MMemory::CopyMem(indices, conf.indices, IndexCount * IndexSize);
        CopyNames(conf.name, conf.MaterialName);
        center = conf.center;
        MinExtents = conf.MinExtents;
        MaxExtents = conf.MaxExtents;
        return *this;
    }
    GeometryConfig& operator =(GeometryConfig&& conf) {
        VertexSize = conf.VertexSize;
        VertexCount = conf.VertexCount;
        vertices = conf.vertices;
        IndexSize = conf.IndexSize;
        IndexCount = conf.IndexCount;
        indices = conf.indices;
        CopyNames(conf.name, conf.MaterialName);
        center = conf.center;
        MinExtents = conf.MinExtents;
        MaxExtents = conf.MaxExtents;

        conf.VertexSize = 0;
        conf.VertexCount = 0;
        conf.vertices = nullptr;
        conf.IndexSize = 0;
        conf.IndexCount = 0;
        conf.indices = nullptr;
        conf.center = FVec3();
        conf.MinExtents = FVec3();
        conf.MaxExtents = FVec3();

        return *this;
    }

    /// @brief Освобождает ресурсы, имеющиеся в указанной конфигурации.
    /// @param config ссылка на конфигурацию, которую нужно удалить.
    void Dispose();
private:
    /// @brief 
    /// @param name 
    /// @param MaterialName 
    void CopyNames(const char (&name)[256], const char (&MaterialName)[256]);
};

constexpr const char* DEFAULT_GEOMETRY_NAME = "default";

class GeometrySystem
{
private:
    // Максимальное количество геометрий, которые можно загрузить одновременно.
    // ПРИМЕЧАНИЕ. Должно быть значительно больше, чем количество статических сеток, 
    // потому что их может и будет больше одного на сетку.
    // Принимаем во внимание и другие системы
    u32 MaxGeometryCount{};
    GeometryID DefaultGeometry{0, 0};
    GeometryID Default2dGeometry{0, 0};

    // Массив зарегистрированных сеток.
    struct GeometryReference* RegisteredGeometries;

    static GeometrySystem* state;

    GeometrySystem(u32 MaxGeometryCount, GeometryReference* RegisteredGeometries);
    ~GeometrySystem() {}
public:
    GeometrySystem(const GeometrySystem&) = delete;
    GeometrySystem& operator= (const GeometrySystem&) = delete;

    static MINLINE GeometrySystem* Instance() { return state; }

    static bool Initialize(u32 MaxGeometryCount);
    static void Shutdown();

    /// @brief Получает существующую геометрию по идентификатору.
    /// @param id Идентификатор геометрии, по которому необходимо получить данные.
    /// @return Указатель на полученную геометрию или nullptr в случае неудачи.
    GeometryID* Acquire(u32 id);
    /// @brief Регистрирует и получает новую геометрию, используя данную конфигурацию.
    /// @param config Конфигурация геометрии.
    /// @param AutoRelease Указывает, должна ли полученная геометрия быть выгружена, когда ее счетчик ссылок достигнет 0.
    /// @return Указатель на полученную геометрию или nullptr в случае неудачи. 
    GeometryID* Acquire(const GeometryConfig& config, bool AutoRelease);

    /// @brief Освобождает ссылку на предоставленную геометрию.
    /// @param Geometry Геометрия, которую нужно освободить.
    void Release(GeometryID *gid);
    /// @brief Получает указатель на геометрию по умолчанию.
    /// @return Указатель на геометрию по умолчанию.
    GeometryID* GetDefault();
    /// @brief Получает указатель на геометрию по умолчанию.
    /// @return Указатель на геометрию по умолчанию.
    GeometryID* GetDefault2D();
    /// @brief Генерирует конфигурацию для геометрии плоскости с учетом предоставленных параметров.
    /// ПРИМЕЧАНИЕ: массивы вершин и индексов распределяются динамически и должны освобождаться при удалении объекта.
    /// Таким образом, это не следует считать производственным кодом.
    /// @param width Общая ширина плоскости. Должно быть ненулевое.
    /// @param height Общая высота плоскости. Должно быть ненулевое.
    /// @param xSegmentCount Количество сегментов по оси X в плоскости. Должно быть ненулевое.
    /// @param ySegmentCount Количество сегментов по оси Y в плоскости. Должно быть ненулевое.
    /// @param TileX Сколько раз текстура должна пересекать плоскость по оси X. Должно быть ненулевое.
    /// @param TileY Сколько раз текстура должна пересекать плоскость по оси Y. Должно быть ненулевое.
    /// @param name Имя сгенерированной геометрии.
    /// @param MaterialName Имя материала, который будет использоваться.
    /// @return Конфигурация геометрии, которую затем можно передать в Geometry_system_acquire_from_config().
    GeometryConfig GeneratePlaneConfig(
        f32 width, f32 height, 
        u32 xSegmentCount, 
        u32 ySegmentCount, 
        f32 TileX, f32 TileY, 
        const char* name, 
        const char* MaterialName);
    
    /// @brief 
    /// @param width 
    /// @param height 
    /// @param depth 
    /// @param TileX 
    /// @param TileY 
    /// @param name 
    /// @param MaterialName 
    /// @return 
    GeometryConfig GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 TileX, f32 TileY, const char* name, const char* MaterialName);
private:
    bool CreateGeometry(const GeometryConfig& config, GeometryID* gid);
    void DestroyGeometry(GeometryID* gid);
    bool CreateDefaultGeometries();
public:
    // void* operator new(u64 size);
};
