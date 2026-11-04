#include "texture.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"

Texture::~Texture()
{
    if(data) {
        // delete data;
        data = nullptr;
    }
}

void Texture::Destroy()
{
    SystemsManager::GetRenderingSystem()->Unload(this);
    MString::Zero(name);
    id           = 0;
    width        = 0;
    height       = 0;
    MipLevels    = 0;
    ChannelCount = 0;
    flags        = 0;
    generation   = 0;
}

Texture::Texture(const Texture &t)
    : name(), id(t.id), generation(t.generation), width(t.width), height(t.height), type(t.type),  
    flags(t.flags), MipLevels(t.MipLevels), ChannelCount(t.ChannelCount), 
    data(t.data ? SystemsManager::GetRenderingSystem()->TextureCopyData(&t) : nullptr) { 
    SetName(t.name); 
}

// Texture &Texture::operator=(const Texture &t)
// {
//     id = t.id;
//     type = t.type;
//     width = t.width;
//     height = t.height;
//     ChannelCount = t.ChannelCount;
//     flags = t.flags;
//     generation = t.generation;
//     SetName(t.name);
//     data = t.data;
//     return *this;
// }

Texture &Texture::operator=(Texture &&t)
{
    id = t.id;
    type = t.type;
    width = t.width;
    height = t.height;
    MipLevels = t.MipLevels;
    ChannelCount = t.ChannelCount;
    flags = t.flags;
    generation = t.generation;
    SetName(t.name);
    data = t.data;

    t.id = t.width = t.height = t.generation = 0;
    //t.type = 0;
    t.MipLevels = t.ChannelCount = 0;
    t.flags = 0;
    MString::Zero(t.name);
    t.data = nullptr;

    return *this;
}

void Texture::Create(const char* name, i32 width, i32 height, i32 ChannelCount, TextureFlagBits flags)
{
    this->width = width;
    this->height = height;
    this->ChannelCount = ChannelCount;
    this->generation = INVALID::ID;
    SetName(name);
    this->flags = flags;
    this->generation++;
}

void Texture::Create(const TextureConfig &config)
{
    SetName(config.name);
    width = config.width;
    height = config.height;
    type = config.type;
    flags |= config.HasTransparency ? Texture::HasTransparency : 0;
    flags |= config.IsWriteable ? Texture::IsWriteable : 0;
    // flags |= config.IsWriteable ? Texture::IsWrapped;
    flags |= config.Depth ? Texture::Flag::Depth : 0;
    MipLevels = config.MipLevels;
    ChannelCount = config.ChannelCount;
    data = config.data;
}

void Texture::Clear()
{
    id = width = height = ChannelCount = flags = generation = 0;
    MString::Zero(name);
}

Texture& Texture::Copy(const Texture& texture)
{
    id = texture.id;
    type = texture.type;
    width = texture.width;
    height = texture.height;
    MipLevels = texture.MipLevels;
    ChannelCount = texture.ChannelCount;
    flags = texture.flags;
    generation = texture.generation;
    SetName(texture.name);
    data = SystemsManager::GetRenderingSystem()->TextureCopyData(&texture);
    return *this;
}

void Texture::SetName(const char *string)
{
    MString::Copy(name, string, TEXTURE_NAME_MAX_LENGTH);
}

Texture::operator bool() const
{
    if (id != 0 && width  != 0 && height != 0 && ChannelCount != 0 && flags != 0 && generation != 0 && data != nullptr) {
        return true;
    }
    return false;
}

void *Texture::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Texture);
}

void *Texture::operator new[](u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void Texture::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Texture);
}

void Texture::operator delete[](void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}
