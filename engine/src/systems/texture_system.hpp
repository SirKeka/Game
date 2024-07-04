#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

constexpr const char* DEFAULT_TEXTURE_NAME = "default";                     // Имя текстуры по умолчанию.
constexpr const char* DEFAULT_DIFFUSE_TEXTURE_NAME = "default_diffuse";     // Имя диффизной текстуры по умолчанию.
constexpr const char* DEFAULT_SPECULAR_TEXTURE_NAME = "default_specular";   // Имя зеркальной текстуры по умолчанию.
constexpr const char* DEFAULT_NORMAL_TEXTURE_NAME = "default_normal";       // Имя текстуры нормалей по умолчанию.
struct TextureReference;

class TextureSystem
{
private:
    u32 MaxTextureCount{};
    Texture DefaultTexture{};
    Texture DefaultDiffuseTexture{};
    Texture DefaultSpecularTexture{};
    Texture DefaultNormalTexture{};

    // Массив зарегистрированных текстур.
    Texture* RegisteredTextures{};

    // Хэш-таблица для поиска текстур.
    HashTable<TextureReference> RegisteredTextureTable{};

    static TextureSystem* state;

    TextureSystem(u32 MaxTextureCount, Texture* RegisteredTextures, TextureReference* HashtableBlock);
public:
    ~TextureSystem();
    TextureSystem(const TextureSystem&) = delete;
    TextureSystem& operator= (const TextureSystem&) = delete;

    static bool Initialize(u32 MaxTextureCount);
    static void Shutdown();

    Texture* Acquire(const char* name, bool AutoRelease);
    void Release(const char* name);
    /// @brief Функция для получения стандартной текстуры.
    /// @return указатель на стандартную текстуру.
    Texture* GetDefaultTexture();
    /// @brief Функция для получения стандартной диффузной текстуры.
    /// @return указатель на стандартную текстуру.
    Texture* GetDefaultDiffuseTexture();
    /// @brief Функция для получения стандартной зеркальной текстуры.
    /// @return указатель на стандартную зеркальную текстуру.
    Texture* GetDefaultSpecularTexture();
    /// @brief Функция для получения текстуры нормалей.
    /// @return указатель на текстуру нормалей.
    Texture* GetDefaultNormalTexture();
    static MINLINE TextureSystem* Instance() { return state; };
    // void* operator new(u64 size);
    // void operator delete(void* ptr)

private:
    bool CreateDefaultTexture();
    void DestroyDefaultTexture();
    static bool LoadTexture(const char* TextureName, Texture* t);
};



