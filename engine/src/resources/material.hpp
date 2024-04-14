#pragma once
#include "texture.hpp"
#include "core/mmemory.hpp"

struct TextureMap {
    Texture* texture;
    TextureUse use;
};

#define MATERIAL_NAME_MAX_LENGTH 256

enum class MaterialType {
    World,
    UI
};

struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH];
    MaterialType type;
    bool AutoRelease;
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH];
    Vector4D<f32> DiffuseColour;
};

class Material
{
public:
    u32 id;
    u32 generation;
    u32 InternalId;
    MaterialType type;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vector4D<f32> DiffuseColour;
    TextureMap DiffuseMap;
public:
    Material();
    Material(const Material& m);
    Material(const char* name, Vector4D<f32> DiffuseColour, TextureUse use, Texture* texture);
    ~Material();

    MINLINE void Destroy() { this->~Material(); }
    void Set(const char* name, Vector4D<f32> DiffuseColour, TextureUse use, Texture* texture);
    void SetName(const char* name);
};
