#pragma once
#include "defines.hpp"
#include "core/mmemory.hpp"
#include "resources/texture.hpp"
#include "math/vector4d_fwd.hpp"

constexpr i32 MATERIAL_NAME_MAX_LENGTH = 256;       // Максимальная длина имени материала.

/// @brief Конфигурация материала обычно загружается из файла или создается в коде для загрузки материала.
struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH]{};          // Название материала.
    MString ShaderName;                             // Имя шейдера материала.
    bool AutoRelease{};                             // Указывает, должен ли материал автоматически выпускаться, если на него не осталось ссылок.
    Vector4D<f32> DiffuseColour{};                  // Рассеяный цвет материала.
    f32 specular;                                   // Зеркальность материала.
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH]{}; // Имя диффузной карты. 
    char SpecularMapName[TEXTURE_NAME_MAX_LENGTH];  // Имя зеркальной карты.
    char NormalMapName[TEXTURE_NAME_MAX_LENGTH];    // Название карты нормалей.

    constexpr MaterialConfig() : name(), ShaderName(), AutoRelease(), DiffuseColour(), specular(), DiffuseMapName(), SpecularMapName(), NormalMapName() {}
    constexpr MaterialConfig(const char* name, const char* ShaderName, bool AutoRelease, const Vector4D<f32>& DiffuseColour) 
    : name(), ShaderName(ShaderName), AutoRelease(AutoRelease), DiffuseColour(DiffuseColour), specular(), DiffuseMapName(), SpecularMapName(), NormalMapName()
    {
        if (name) {
            for (u64 i = 0; i < MATERIAL_NAME_MAX_LENGTH; i++) {
                this->name[i] = name[i];
                if (!this->name[i] || !name[i]) {
                    break;
                }
            }
        }
        //MString::nCopy(this->name, name, MATERIAL_NAME_MAX_LENGTH);
    }
    void Reset() {
        ShaderName.Clear();
        AutoRelease = false;
        DiffuseColour = FVec4();
        specular = 0;
        for (u32 i = 0; i < TEXTURE_NAME_MAX_LENGTH; i++) {
            if (name[i]) {
                name[i] = '\0';
            }
            if (DiffuseMapName[i]) {
                DiffuseMapName[i] = '\0';
            }
            if (SpecularMapName[i]) {
                SpecularMapName[i] = '\0';
            }
            if (NormalMapName[i]) {
                NormalMapName[i] = '\0';
            }
            if (name[i] == DiffuseMapName[i] == SpecularMapName[i] == NormalMapName[i] == '\0') {
                break;
            }
            
        }
        
    }
        
    void* operator new(u64 size) { return MMemory::Allocate(size, MemoryTag::MaterialInstance); }
    void operator delete(void* ptr, u64 size) { MMemory::Free(ptr, size, MemoryTag::MaterialInstance); }
};