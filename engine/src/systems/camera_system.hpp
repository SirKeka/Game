/// @file camera_system.hpp
/// @author 
/// @brief Система камер отвечает за управление камерами по всему движку.
/// @version 1.0
/// @date 2024-08-06
/// 
/// @copyright
#pragma once

#include "containers/hashtable.hpp"
#include "renderer/camera.hpp"

constexpr const char* DEFAULT_CAMERA_NAME = "default";
struct CameraLookup;

/// @brief Конфигурация системы камер.
struct CameraSystemConfig
{
    /// @brief ПРИМЕЧАНИЕ: Максимальное количество камер, которыми может управлять система.
    u16 MaxCameraCount;
};

class CameraSystem
{
private:
    u16 MaxCameraCount;             // ПРИМЕЧАНИЕ: Максимальное количество камер, которыми может управлять система.
    HashTable<u16> lookup;
    [[maybe_unused]]void* HashtableBlock;
    CameraLookup* cameras;

    Camera DefaultCamera;           // Незарегистрированная камера по умолчанию, которая всегда существует в качестве запасного варианта.

    static CameraSystem* state;
    CameraSystem(u16 MaxCameraCount, void* HashtableBlock, CameraLookup* cameras);
public:
    CameraSystem(const CameraSystem&) = delete;
    CameraSystem& operator= (const CameraSystem&) = delete;
    ~CameraSystem() {}

    MAPI static CameraSystem* Instance();

    /// @brief Инициализирует систему камеры.
    /// 
    /// @return True в случае успеха, иначе false.
    static bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    /// @brief Выключает систему камер.
    static void Shutdown();

    /// @brief Получает указатель на камеру по имени. Если камера не найдена, создается новая и перенастраивается. Внутренний счетчик ссылок увеличивается.
    /// @param name имя камеры для получения.
    /// @return Указатель на камеру в случае успеха; nullptr в случае ошибки.
    Camera* Acquire(const char* name);
    /// @brief Освобождает камеру с указанным именем. Внутренний счетчик ссылок уменьшается. Если он достигает 0, камера сбрасывается, и ссылка может использоваться новой камерой.
    /// @param name Имя камеры для освобождения.
    void Release(const char* name);
    /// @brief Получает указатель на камеру по умолчанию.
    /// @return Указатель на камеру по умолчанию.
    static MAPI Camera* GetDefault();
};
