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

    constexpr MaterialConfig(
        const char* name, 
        const char* ShaderName, 
        bool AutoRelease, 
        const char* DiffuseMapName, 
        const Vector4D<f32>& DiffuseColour) 
        : ShaderName(ShaderName), AutoRelease(AutoRelease), DiffuseColour(DiffuseColour)
        {
            // СДЕЛАТЬ: переделать
            for (u64 i = 0; i < TEXTURE_NAME_MAX_LENGTH; i++) {
                if (*name) {
                    this->name[i] = name[i];
                }
                if (DiffuseMapName/* || *DiffuseMapName*/) {
                    this->DiffuseMapName[i] = DiffuseMapName[i];
                }
                if (name[i] == '\0' && (!DiffuseMapName || DiffuseMapName[i] == '\0')) {
                    break;
                }
            }
        }
        
    void* operator new(u64 size) { return MMemory::Allocate(size, MemoryTag::MaterialInstance); }
    void operator delete(void* ptr, u64 size) { MMemory::Free(ptr, size, MemoryTag::MaterialInstance); }
};