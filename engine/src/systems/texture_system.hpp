#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

#define DEFAULT_TEXTURE_NAME "default"

class TextureSystem
{
    struct TextureReference {
        u64 ReferenceCount;
        u32 handle;
        bool AutoRelease;
    };

private:
    u32 MaxTextureCount;
    Texture DefaultTexture;

    // Массив зарегистрированных текстур.
    Texture* RegisteredTextures;

    // Хэш-таблица для поиска текстур.
    HashTable RegisteredTextureTable;

public:
    TextureSystem() : MaxTextureCount(0) {}
    ~TextureSystem() = default;

    bool Initialize(u64* memory_requirement, void* state);
    void Shutdown(void* state);

    Texture* Acquire(MString name, bool AutoRelease);
    void Release(MString name);

    Texture* GetDefaultTexture();

    void* operator new(u64 size);
    //void operator delete(void* ptr)

private:
    bool LoadTexture(MString TextureName, Texture* t);
};



