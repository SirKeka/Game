#include "material.hpp"

Material::Material() : id(INVALID::ID), generation(INVALID::ID), InternalId(INVALID::ID), name(), DiffuseColour(), DiffuseMap() {}

Material::Material(const Material &m) : id(m.id), generation(m.generation), InternalId(m.InternalId)
{
    id = m.id;
    generation = m.generation;
    InternalId = m.InternalId;
    MString::Copy(name, m.name);
    DiffuseColour = m.DiffuseColour;
    DiffuseMap = m.DiffuseMap;
}

Material::Material(const char *name, Vector4D<f32> DiffuseColour, TextureUse use, Texture *texture)
: id(INVALID::ID), generation(INVALID::ID), InternalId(), ShaderID(), DiffuseColour(DiffuseColour), DiffuseMap(texture, use)
{}

Material::~Material()
{
    MTRACE("Уничтожение материала '%s'...", name);
    u32 id = INVALID::ID;
    u32 generation = INVALID::ID;
    u32 InternalId = INVALID::ID;
    MMemory::ZeroMem(name, MATERIAL_NAME_MAX_LENGTH);
    Vector4D<f32> DiffuseColour = Vector4D<f32>::Zero();
    MMemory::ZeroMem(&DiffuseMap, sizeof(TextureMap));
}

/*const bool Material::operator ! (Material& m)
{
    if ((id != 0 || id != INVALID::U32ID) && 
    (generation != 0 || generation != INVALID::U32ID) && 
    (InternalId != 0 || InternalId != INVALID::U32ID) && 
    (name[0] == '0' || name[0] == '/') && 
    (bool)DiffuseColour && DiffuseMap.use != TextureUse::Unknown &&
    DiffuseMap.texture != nullptr) {
        return true;
    }
    return false;
}*/

void Material::Set(const char *name, Vector4D<f32> DiffuseColour, TextureUse use, Texture *texture)
{
    MMemory::CopyMem(this->name, name, MATERIAL_NAME_MAX_LENGTH);
    this->DiffuseColour = DiffuseColour;
    this->DiffuseMap.use = use;
    this->DiffuseMap.texture = texture;
}

void Material::SetName(const char *name)
{
    MMemory::CopyMem(this->name, name, MATERIAL_NAME_MAX_LENGTH);
}
