#pragma once
#include "texture.hpp"
#include "core/mmemory.hpp"

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
    Material() : id(INVALID_ID), generation(INVALID_ID), InternalId(INVALID_ID), name(), DiffuseColour(), DiffuseMap() {}
    Material(const Material& m) : id(m.id), generation(m.generation), InternalId(m.InternalId), /*name(m.name),*/ DiffuseColour(m.DiffuseColour), DiffuseMap(m.DiffuseMap) {MString::Copy(name, m.name);}
    //~Material() = delete;
    void Destroy() {
        u32 id = INVALID_ID;
        u32 generation = INVALID_ID;
        u32 InternalId = INVALID_ID;
        MMemory::ZeroMem(name, MATERIAL_NAME_MAX_LENGTH);
        Vector4D<f32> DiffuseColour = Vector4D<f32>::Zero();
        MMemory::ZeroMem(&DiffuseMap, sizeof(TextureMap));
    }
};
