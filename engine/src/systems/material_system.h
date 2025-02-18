/// @file material_system.hpp
/// @author
/// @brief Система материалов отвечает за управление материалами в движке, включая подсчет ссылок и автоматическую выгрузку.
/// @version 1.0
/// @date
/// 
/// @copyright
#pragma once
#include "resources/material.h"
#include "containers/hashtable.hpp"

class Matrix4D;

/// @brief Имя материала по умолчанию.
#define DEFAULT_MATERIAL_NAME "default"    // Имя материала по умолчанию.

/// @brief Имя материала пользовательского интерфейса по умолчанию.
#define DEFAULT_UI_MATERIAL_NAME "default_ui"

/// @brief Имя материала ландшафта по умолчанию.
#define DEFAULT_TERRAIN_MATERIAL_NAME "default_terrain"

/// @brief Конфигурация для системы материалов.
struct MaterialSystemConfig
{
    /// @brief Максимальное количество загруженных материалов.
    u32 MaxMaterialCount;
};

namespace MaterialSystem
{
    MAPI Material* GetDefaultMaterial();

    /// @brief Получает указатель на материал пользовательского интерфейса по умолчанию. Не ссылается на счетчик.
    MAPI Material* GetDefaultUiMAterial();

    /// @brief Получает указатель на материал ландшафта по умолчанию. Не ссылается на счетчик.
    MAPI Material* GetDefaultTerrainMaterial();

    /// @brief Функция создает объект класса и инициализирует его
    /// @param MaxMaterialCount максимальное количество загружаемых материалов.
    /// @return true если инициализаця прошла успешно или false если нет
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);

    void Shutdown();

    /// @brief Пытается получить материал с заданным именем. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал найден и загружен, его счетчик ссылок увеличивается.
    /// @param name имя искомого материала.
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    MAPI Material* Acquire(const char* name);

    /// @brief Пытается получить материал из заданной конфигурации. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал _найден и загружен, его счетчик ссылок увеличивается.
    /// @param config конфигурация загружаемого материала
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    MAPI Material* Acquire(const Material::Config& config);

    /// @brief Пытается получить материал ландшафта с указанным именем. 
    /// Если он еще не загружен, это запускает его загрузку с использованием предоставленных стандартных имен материалов. 
    /// Если материал не может быть загружен, возвращается указатель на материал ландшафта по умолчанию. 
    /// Если материал _найден_ и загружен, его счетчик ссылок увеличивается.
    /// @param MaterialName имя материала ландшафта, который нужно найти.
    /// @param MaterialCount количество стандартных имен исходных материалов.
    /// @param MaterialNames имена исходных материалов, которые нужно использовать.
    /// @param AutoRelease
    /// @return указатель на загруженный материал ландшафта. Может быть указателем на материал ландшафта по умолчанию, если он не найден.
    MAPI Material* Acquire(const char* MaterialName, u32 MaterialCount, const char** MaterialNames, bool AutoRelease);

    MAPI void Release(const char* name);

    /// @brief Применяет данные глобального уровня для идентификатора шейдера материала.
    /// @param ShaderID идентификатор шейдера, к которому применяются глобальные переменные.
    /// @param RenderFrameNumber текущий номер кадра рендерера.
    /// @param projection константная ссылка на матрицу проекции.
    /// @param view константная ссылка на матрицу представления.
    /// @param AmbientColour окружающий цвет сцены.
    /// @param ViewPosition позиция камеры.
    /// @return true в случае успеха иначе false.
    MAPI bool ApplyGlobal(u32 ShaderID, u64 RenderFrameNumber, const Matrix4D& projection, const Matrix4D& view, const FVec4& AmbientColour = FVec4(), const FVec4& ViewPosition = FVec4(), u32 RenderMode = 0);
    
    /// @brief Применяет данные материала на уровне экземпляра для данного материала.
    /// @param material указатель на материал, который будет применен.
    /// @return true в случае успеха иначе false.
    MAPI bool ApplyInstance(Material* material, bool NeedsUpdate);

    /// @brief Применяет данные о материале локального уровня (обычно только матрицу модели).
    /// @param material указатель на материал, который будет применен.
    /// @param model константная ссылка на применяемую матрицу модели.
    /// @return true в случае успеха иначе false.
    MAPI bool ApplyLocal(Material* material, const Matrix4D& model);

    /// @brief Выводит все зарегистрированные материалы и их количества/маркеры.
    MAPI void Dump();
};
