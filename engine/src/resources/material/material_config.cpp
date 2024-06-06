#include "material_config.hpp"

MaterialConfig::MaterialConfig(char (&name)[MATERIAL_NAME_MAX_LENGTH], MString &ShaderName, bool AutoRelease, char (&DiffuseMapName)[TEXTURE_NAME_MAX_LENGTH], Vector4D<f32> &DiffuseColour)
 : 
 /*name(name),*/ 
 ShaderName(ShaderName), 
 AutoRelease(AutoRelease), 
 /*DiffuseMapName(DiffuseMapName),*/ 
 DiffuseColour(DiffuseColour)
{
    for (u64 i = 0; i < TEXTURE_NAME_MAX_LENGTH; i++) {
        if (name[i]) {
            this->name[i] = name[i];
        }
        this->DiffuseMapName[i] = DiffuseMapName[i];
        if (name[i] == '\0' && DiffuseMapName[i] == '\0') {
            break;
        }
    }
}

void *MaterialConfig::operator new(u64 size)
{
    return MMemory::Allocate(size, MemoryTag::MaterialInstance);
}
