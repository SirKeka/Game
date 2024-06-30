#pragma once
#include "material_config.hpp"

/// @brief Структура, которая отображает текстуру, использование и другие свойства.
struct TextureMap {
    Texture* texture;                       // Указатель на текстуру.
    TextureUse use;                         // Использование текстуры.
    constexpr TextureMap() : texture(nullptr), use() {}
    constexpr TextureMap(Texture* texture, TextureUse use) : texture(texture), use(use) {}
    constexpr TextureMap(const TextureMap& tm) : texture(tm.texture), use(tm.use) {}
};

/// @brief Материал, который отражает различные свойства поверхности в мире, такие как текстура, цвет, неровности, блеск и многое другое.
class Material
{
    //friend class MaterialSystem;
    //friend class GeometrySystem;
public:
    u32 id{};                               // Идентификатор материала.
    u32 generation{};                       // Поколение материала. Увеличивается при каждом изменении материала.
    u32 InternalId{};                       // Внутренний идентификатор материала. Используется серверной частью рендеринга для сопоставления с внутренними ресурсами.
    u32 ShaderID{};                         // Индентификатор шейдера.
    char name[MATERIAL_NAME_MAX_LENGTH]{};  // Название материала.
    Vector4D<f32> DiffuseColour{};          // Рассеянный(Diffuse) цвет.
    TextureMap DiffuseMap{};                // Карта диффузной текстуры.
    TextureMap SpecularMap{};               // Карта блеска материала.
    TextureMap NormalMap{};                 // Текстурная карта нормалей.
    f32 specular{};                         // Блеск материала определяет, насколько сконцентрировано зеркальное освещение.

    u32 RenderFrameNumber;                  // Синхронизируется с текущим номером кадра средства рендеринга, когда к этому кадру был применен материал.
    
public:
    constexpr Material() : id(INVALID::ID), generation(INVALID::ID), InternalId(INVALID::ID), ShaderID(), name(), DiffuseColour(), DiffuseMap(), SpecularMap(), NormalMap(), specular() {}
    constexpr Material(const Material& m);
    constexpr Material(const char* name, const Vector4D<f32>& DiffuseColour, const TextureMap& DiffuseMap, const TextureMap& SpecularMap, const TextureMap& NormalMap)
    : 
        id(INVALID::ID), 
        generation(INVALID::ID), 
        InternalId(), 
        ShaderID(), 
        name(), 
        DiffuseColour(DiffuseColour), 
        DiffuseMap(DiffuseMap), 
        SpecularMap(SpecularMap), 
        NormalMap(NormalMap),
        specular() 
    { MString::nCopy(this->name, name, MATERIAL_NAME_MAX_LENGTH); }
    ~Material();

    MINLINE void Destroy() { this->~Material(); }
    void Reset();
    void SetName(const char* name);
};
