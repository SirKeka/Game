#pragma once
#include "defines.hpp"
#include "resources/texture.hpp"
#include "core/mmemory.hpp"

constexpr int MATERIAL_NAME_MAX_LENGTH = 256;       // Максимальная длина имени материала.

/// @brief Конфигурация материала обычно загружается из файла или создается в коде для загрузки материала.
struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH]{};          // Название материала.
    MString ShaderName;                             // Имя шейдера материала.
    bool AutoRelease{};                             // Указывает, должен ли материал автоматически выпускаться, если на него не осталось ссылок.
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH]{}; // Имя диффузной карты. 
    Vector4D<f32> DiffuseColour{};                  // Рассеяный цвет материала.

    MaterialConfig(
        char (&name)[MATERIAL_NAME_MAX_LENGTH], 
        class MString& ShaderName, 
        bool AutoRelease, 
        char (&DiffuseMapName)[TEXTURE_NAME_MAX_LENGTH], 
        Vector4D<f32>& DiffuseColour);
        
    void* operator new(u64 size);
};