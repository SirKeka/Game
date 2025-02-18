#pragma once

#include "defines.hpp"
#include "resources/geometry.hpp"
#include "resources/material.h"

#define DEFAULT_GEOMETRY_NAME "default"

/// @brief Конфигурация геометрической системы.
struct GeometrySystemConfig {
    /// @brief ПРИМЕЧАНИЕ: Должна быть значительно больше, чем количество статических сеток, 
    /// поскольку их может быть и будет больше одной на сетку. Примите во внимание и другие системы.
    u32 MaxGeometryCount;
};

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
    struct GeometryReference* RegisteredGeometries{nullptr};

    MAPI static GeometrySystem* state;

    GeometrySystem(u32 MaxGeometryCount, GeometryReference* RegisteredGeometries);
    ~GeometrySystem() {}
public:
    GeometrySystem(const GeometrySystem&) = delete;
    GeometrySystem& operator= (const GeometrySystem&) = delete;

    static MINLINE GeometrySystem* Instance() { return state; }

    static bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    static void Shutdown();

    /// @brief Получает существующую геометрию по идентификатору.
    /// @param id Идентификатор геометрии, по которому необходимо получить данные.
    /// @return Указатель на полученную геометрию или nullptr в случае неудачи.
    MAPI static GeometryID* Acquire(u32 id);
    /// @brief Регистрирует и получает новую геометрию, используя данную конфигурацию.
    /// @param config Конфигурация геометрии.
    /// @param AutoRelease Указывает, должна ли полученная геометрия быть выгружена, когда ее счетчик ссылок достигнет 0.
    /// @return Указатель на полученную геометрию или nullptr в случае неудачи. 
    MAPI static GeometryID* Acquire(GeometryConfig& config, bool AutoRelease);

    /// @brief Освобождает ссылку на предоставленную геометрию.
    /// @param Geometry Геометрия, которую нужно освободить.
    static void Release(GeometryID *gid);
    /// @brief Получает указатель на геометрию по умолчанию.
    /// @return Указатель на геометрию по умолчанию.
    static GeometryID* GetDefault();
    /// @brief Получает указатель на геометрию по умолчанию.
    /// @return Указатель на геометрию по умолчанию.
    static GeometryID* GetDefault2D();
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
    static GeometryConfig GeneratePlaneConfig(
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
    MAPI static GeometryConfig GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 TileX, f32 TileY, const char* name, const char* MaterialName);
private:
    static bool CreateGeometry(GeometryConfig& config, GeometryID* gid);
    static void DestroyGeometry(GeometryID* gid);
    static bool CreateDefaultGeometries();
public:
    // void* operator new(u64 size);
};
