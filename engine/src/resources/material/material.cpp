#include "material.hpp"

constexpr Material::Material(const Material &m) 
: 
id(m.id), 
generation(m.generation), 
InternalId(m.InternalId), 
ShaderID(m.ShaderID), 
name(), 
DiffuseColour(m.DiffuseColour), 
DiffuseMap(m.DiffuseMap), 
SpecularMap(m.SpecularMap), 
specular(m.specular),
RenderFrameNumber(m.RenderFrameNumber)
{
    MString::Copy(name, m.name);
}

Material::~Material()
{
    MTRACE("Уничтожение материала '%s'...", name);
    MMemory::ZeroMem(this, sizeof(Material));
    id = generation = InternalId = RenderFrameNumber = INVALID::ID;
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

void Material::Reset()
{
    id = generation = InternalId = RenderFrameNumber = 0;
}

void Material::SetName(const char *name)
{
    MMemory::CopyMem(this->name, name, MATERIAL_NAME_MAX_LENGTH);
}
