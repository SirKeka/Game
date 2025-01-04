/// @file material_system.hpp
/// @author
/// @brief Система материалов отвечает за управление материалами в движке, включая подсчет ссылок и автоматическую выгрузку.
/// @version 1.0
/// @date
/// 
/// @copyright
#pragma once
#include "resources/material/material.hpp"
#include "containers/hashtable.hpp"

class Matrix4D;

/// @brief Имя материала по умолчанию.
constexpr const char* DEFAULT_MATERIAL_NAME = "default";    // Имя материала по умолчанию.

/// @brief Конфигурация для системы материалов.
struct MaterialSystemConfig
{
    /// @brief Максимальное количество загруженных материалов.
    u32 MaxMaterialCount;
};


namespace MaterialSystem
{
    MAPI Material* GetDefaultMaterial();

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
    MAPI Material* Acquire(const MaterialConfig& config);

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
