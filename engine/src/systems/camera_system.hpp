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

class CameraSystem
{
private:
    u16 MaxCameraCount;             // ПРИМЕЧАНИЕ: Максимальное количество камер, которыми может управлять система.
    HashTable lookup;
    void* HashtableBlock;
    struct CameraLookup* cameras;

    Camera DefaultCamera;           // Незарегистрированная камера по умолчанию, которая всегда существует в качестве запасного варианта.

    static CameraSystem* state;
public:
    constexpr CameraSystem();
    ~CameraSystem();
};
