#pragma once
#include "texture.hpp"
#include "core/mmemory.hpp"

/// @brief Структура, которая отображает текстуру, использование и другие свойства.
struct TextureMap {
    Texture* texture;                               // Указатель на текстуру.
    TextureUse use;                                 // Использование текстуры.
    TextureMap() : texture(nullptr), use() {}
    TextureMap(Texture* texture, TextureUse use) : texture(texture), use(use) {}
};

constexpr int MATERIAL_NAME_MAX_LENGTH = 256;       // Максимальная длина имени материала.

/// @brief Конфигурация материала обычно загружается из файла или создается в коде для загрузки материала.
struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH];            // Название материала.
    MString ShaderName;                             // Имя шейдера материала.
    bool AutoRelease;                               // Указывает, должен ли материал автоматически выпускаться, если на него не осталось ссылок.
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH];   // Рассеяный цвет материала.
    Vector4D<f32> DiffuseColour;                    // Имя диффузной карты.
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
    Material();
    Material(const Material& m);
    Material(const char* name, Vector4D<f32> DiffuseColour, TextureUse use, Texture* texture);
    ~Material();

    MINLINE void Destroy() { this->~Material(); }
    void Set(const char* name, Vector4D<f32> DiffuseColour, TextureUse use, Texture* texture);
    void SetName(const char* name);
};
