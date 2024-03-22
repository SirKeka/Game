#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

#define DEFAULT_TEXTURE_NAME "default"
struct TextureReference;

class TextureSystem
{
private:
    static u32 MaxTextureCount;
    Texture DefaultTexture;

    // Массив зарегистрированных текстур.
    Texture* RegisteredTextures;

    // Хэш-таблица для поиска текстур.
    HashTable<TextureReference> RegisteredTextureTable;

    static TextureSystem* state;

    TextureSystem();
public:
    ~TextureSystem();
    TextureSystem(const TextureSystem&) = delete;
    TextureSystem& operator= (const TextureSystem&) = delete;

    bool Initialize();
    void Shutdown();

    Texture* Acquire(MString name, bool AutoRelease);
    void Release(MString name);
    /// @brief Функция для получения стандартной текстуры.
    /// @return указатель на стандартную текстуру.
    Texture* GetDefaultTexture();
    /// @brief Задает максимальное возможное количество текстур от которого зависит размер выделяеймой памяти под систему.
    /// @param value максимальное количество текстур.
    static void SetMaxTextureCount(u32 value);
    static TextureSystem* Instance();
    void* operator new(u64 size);
    //void operator delete(void* ptr)

private:
    bool CreateDefaultTexture();
    void DestroyDefaultTexture();
    static bool LoadTexture(MString TextureName, Texture* t);
};



