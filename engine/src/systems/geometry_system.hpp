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

    GeometryConfig() : VertexSize(), VertexCount(), vertices(nullptr), IndexSize(), IndexCount(), indices(nullptr), name(), MaterialName() {}

    GeometryConfig(u32 VertexSize, u32 VertexCount, void* vertices, u32 IndexSize, u32 IndexCount, void* indices, const char* name, const char* MaterialName)
    : VertexSize(VertexSize), VertexCount(VertexCount), vertices(vertices), IndexSize(IndexSize), IndexCount(IndexCount), indices(indices) {
        MMemory::CopyMem(this->name, name, GEOMETRY_NAME_MAX_LENGTH);
        MMemory::CopyMem(this->MaterialName, MaterialName, GEOMETRY_NAME_MAX_LENGTH);
    }
};

#define DEFAULT_GEOMETRY_NAME "default"

class GeometrySystem
{
private:
    // Максимальное количество геометрий, которые можно загрузить одновременно.
    // ПРИМЕЧАНИЕ. Должно быть значительно больше, чем количество статических сеток, 
    // потому что их может и будет больше одного на сетку.
    // Принимаем во внимание и другие системы
    static u32 MaxGeometryCount;

    GeometryConfig config;

    GeometryID DefaultGeometry{0, 0};
    GeometryID Default2dGeometry{0, 0};

    // Массив зарегистрированных сеток.
    struct GeometryReference* RegisteredGeometries;

    static GeometrySystem* state;

    GeometrySystem();
    ~GeometrySystem();
public:
    GeometrySystem(const GeometrySystem&) = delete;
    GeometrySystem& operator= (const GeometrySystem&) = delete;

    static GeometrySystem* Instance() { return state; }
    static void SetMaxGeometryCount(u32 value);

    bool Initialize();
    void Shutdown();

    /// @brief Получает существующую геометрию по идентификатору.
    /// @param id Идентификатор геометрии, по которому необходимо получить данные.
    /// @return Указатель на полученную геометрию или nullptr в случае неудачи.
    GeometryID* Acquire(u32 id);
    /// @brief Регистрирует и получает новую геометрию, используя данную конфигурацию.
    /// @param config Конфигурация геометрии.
    /// @param AutoRelease Указывает, должна ли полученная геометрия быть выгружена, когда ее счетчик ссылок достигнет 0.
    /// @return Указатель на полученную геометрию или nullptr в случае неудачи. 
    GeometryID* Acquire(GeometryConfig config, bool AutoRelease);
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
private:
    bool CreateGeometry(GeometryConfig config, GeometryID* gid);
    void DestroyGeometry(GeometryID* gid);
    bool CreateDefaultGeometries();
public:
    void* operator new(u64 size);
};
