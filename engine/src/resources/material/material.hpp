#pragma once
#include "material_config.hpp"

/// @brief Структура, которая отображает текстуру, использование и другие свойства.
struct TextureMap {
    Texture* texture;                               // Указатель на текстуру.
    TextureUse use;                                 // Использование текстуры.
    constexpr TextureMap() : texture(nullptr), use() {}
    constexpr TextureMap(Texture* texture, TextureUse use) : texture(texture), use(use) {}
};

/// @brief Материал, который отражает различные свойства поверхности в мире, такие как текстура, цвет, неровности, блеск и многое другое.
class Material
{
    //friend class MaterialSystem;
    //friend class GeometrySystem;
public:
    u32 id;                                         // Идентификатор материала.
    u32 generation;                                 // Поколение материала. Увеличивается при каждом изменении материала.
    u32 InternalId;                                 // Внутренний идентификатор материала. Используется серверной частью рендеринга для сопоставления с внутренними ресурсами.
    u32 ShaderID;                                   // Индентификатор шейдера.
    char name[MATERIAL_NAME_MAX_LENGTH];            // Название материала.
    Vector4D<f32> DiffuseColour;                    // Рассеянный(Diffuse) цвет.
    TextureMap DiffuseMap;                          // Карта диффузной текстуры.
public:
    constexpr Material() : id(INVALID::ID), generation(INVALID::ID), InternalId(INVALID::ID), ShaderID(), name(), DiffuseColour(), DiffuseMap() {}
    constexpr Material(const Material& m);
    constexpr Material(const char* name, Vector4D<f32> DiffuseColour, TextureUse use, Texture* texture) : id(INVALID::ID), generation(INVALID::ID), InternalId(), ShaderID(), name(), DiffuseColour(DiffuseColour), DiffuseMap(texture, use) 
    { 
        MString::nCopy(this->name, name, MATERIAL_NAME_MAX_LENGTH);
    }
    ~Material();

    MINLINE void Destroy() { this->~Material(); }
    void Set(const char* name, Vector4D<f32> DiffuseColour, TextureUse use, Texture* texture);
    void SetName(const char* name);
};
