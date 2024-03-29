#pragma once
#include "texture.hpp"

struct TextureMap {
    Texture* texture;
    TextureUse use;
};

#define MATERIAL_NAME_MAX_LENGTH 256

class Material
{
public:
    u32 id;
    u32 generation;
    u32 InternalId;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vector4D<f32> DiffuseColour;
    TextureMap DiffuseMap;
public:
    Material() : id(), generation(), InternalId(), name(), DiffuseColour(), DiffuseMap() {}
    Material(const Material& m) : id(m.id), generation(m.generation), InternalId(m.InternalId),/* name(m.name),*/ DiffuseColour(m.DiffuseColour), DiffuseMap(m.DiffuseMap) {MMemory::CopyMem(name, m.name, sizeof(m.name));}
    //~Material() = delete;
    //void Destroy();
};
