#pragma once

#include "core/memory_system.h"
#include "containers/mstring.hpp"

constexpr u32 TEXTURE_NAME_MAX_LENGTH = 512;

struct ImageResourceData {
    u8 ChannelCount  {};
    u32 width        {};
    u32 height       {};
    u8* pixels{nullptr};

    constexpr ImageResourceData() : ChannelCount(), width(), height(), pixels(nullptr) {}
    constexpr ImageResourceData(u8 ChannelCount, u32 width, u32 height, u8* pixels)
    : ChannelCount(ChannelCount), width(width), height(height), pixels(pixels) {}
    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Texture); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Texture); }
};

/// @brief Параметры, используемые при загрузке изображения.
struct ImageResourceParams {
    bool FlipY; // Указывает, следует ли переворачивать изображение по оси Y при загрузке.
    constexpr ImageResourceParams(bool FlipY) : FlipY(FlipY) {}
};

/// @brief Определяет режим отсечения граней во время рендеринга.
enum class FaceCullMode {
    None = 0x0,         // Грани не отсеиваются.
    Front = 0x1,        // Отсеиваются только передние грани.
    Back = 0x2,         // Отсеиваются только задние грани.
    FrontAndBack = 0x3  // Отсеиваются как передние, так и задние грани.
};

enum class TextureUse {
    Unknown     = 0x00,     // Неизвестное применение. Это значение по умолчанию, но его никогда не следует использовать.
    MapDiffuse  = 0x01,     // Текстура используется как диффузная карта.
    MapSpecular = 0x02,     // Текстура используется как карта отражений.
    MapNormal   = 0x03,     // Текстура используется как карта нормалей.
    Cubemap     = 0x04      // Текстура используется как кубическая карта.
};

/// @brief Представляет поддерживаемые режимы фильтрации текстур.
enum class TextureFilter {
    ModeNearest = 0x0,      // Фильтрация ближайших соседей.
    ModeLinear  = 0x1       // Линейная (т.е. билинейная) фильтрация.
};

enum class TextureRepeat {
    Repeat         = 0x1,
    MirroredRepeat = 0x2,
    ClampToEdge    = 0x3,
    ClampToBorder  = 0x4
};

/// @brief Представляет различные типы текстур.
enum class TextureType {
    _2D,    // Стандартная двухмерная текстура.
    Cube    // Текстура куба, используемая для кубических карт.
};

using TextureFlagBits = u8; // Содержит битовые флаги для текстур.

/// @brief Конфигурация текстуры
struct TextureConfig
{
    /// @brief Имя текстуры.
    const char* name; 
    /// @brief Тип текстуры.
    TextureType type;
    /// @brief Ширина текстуры.
    u32 width; 
    /// @brief Высота текстуры.
    u32 height; 
    /// @brief Количество каналов в текстуре.
    u8 ChannelCount;
    /// @brief Указывает, имеет ли текстура прозрачность.
    bool HasTransparency;
    /// @brief Указывает, можно ли записать (отобразить) текстуру.
    bool IsWriteable;
    /// @brief Указывает, должна ли текстура быть зарегистрирована в системе.
    bool RegisterTexture;
    /// @brief Указывает, была ли текстура создана с помощью обертки или традиционного создания.
    bool IsWrapped;
    /// @brief Указывает, что текстура является текстурой глубины.
    bool Depth;
    /// @brief Указатель на данные текстуры.
    void* data;
};

struct MAPI Texture
{
    enum { Default, Diffuse, Specular, Normal };

    enum Flag {
        HasTransparency = 0x1,  // Указывает, имеет ли текстура прозрачность.
        IsWriteable     = 0x2,  // Указывает, можно ли записать (отобразить) текстуру.
        IsWrapped       = 0x4,  // Указывает, была ли текстура создана с помощью обертки или традиционного создания.
        Depth           = 0x8   // Указывает, что текстура является текстурой глубины.
    };

    u32 id;                             // Уникальный идентификатор текстуры.
    TextureType type;                   // Тип текстуры.
    u32 width;                          // Ширина текстуры.
    u32 height;                         // Высота текстуры.
    u8 ChannelCount;                    // Количество каналов в текстуре.
    TextureFlagBits flags;              // Содержит битовые флаги для текстур.
    u32 generation;                     // Генерация текстур. Увеличивается каждый раз при перезагрузке данных.
    char name[TEXTURE_NAME_MAX_LENGTH]; // Имя текстуры.

    void* data;                         // Необработанные данные текстуры (пиксели).

    constexpr Texture() 
    : id(INVALID::ID), type(), width(0), height(0), ChannelCount(0), flags(), generation(INVALID::ID), name(), data(nullptr) {}
    constexpr Texture(u32 id, const TextureConfig& config)
    : id(id), type(config.type), width(config.width), height(config.height), ChannelCount(config.ChannelCount), flags(), generation(INVALID::ID), name(), data(config.data) {
        flags |= config.HasTransparency ? HasTransparency : 0;
        flags |= config.IsWriteable ? IsWriteable : 0;
        flags |= config.IsWrapped ? IsWrapped : 0;
        flags |= config.Depth ? Depth : 0;
        MString::Copy(this->name, config.name, TEXTURE_NAME_MAX_LENGTH);
    }
    constexpr Texture(const TextureConfig& config)
    :
        id(), 
        type(config.type),
        width(config.width), 
        height(config.height), 
        ChannelCount(config.ChannelCount), 
        flags(),
        generation(INVALID::ID), 
        name(),
        data(config.data)
    {
        flags |= config.HasTransparency ? HasTransparency : 0;
        flags |= config.IsWriteable ? IsWriteable : 0;
        flags |= config.IsWrapped ? IsWrapped : 0;
        flags |= config.Depth ? Depth : 0;
        MString::Copy(this->name, config.name, TEXTURE_NAME_MAX_LENGTH);
    }
    ~Texture();

    /// @brief 
    void Destroy();
    
    Texture(const Texture& t);
    constexpr Texture(Texture&& t) : id(t.id), type(t.type), width(t.width), height(t.height), ChannelCount(t.ChannelCount), 
    flags(t.flags), generation(t.generation), name(), data(t.data) { 
        MString::Copy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
        t.id = 0;
        t.width = 0;
        t.height = 0;
        t.ChannelCount = 0;
        t.flags = 0;
        t.generation = 0;
        MString::Zero(t.name);
        t.data = nullptr;
    }

    /// @brief Присваивает(не копирует) данный одной текстуры другой. 
    /// @param t текстура данные которой нужно присвоить
    /// @return 
    // Texture& operator= (const Texture& t);
    Texture& operator= (Texture&& t);
    void Create(const char* name, i32 width, i32 height, i32 ChannelCount, TextureFlagBits flags);

    void Create(const TextureConfig& config);

    void Clear();

    /// @brief Копирует текстуру в новую структуру и возвращает ссылку на нее
    /// @param texture текстура, которую нужно скопировать
    /// @return ссылку на копию текстуры
    Texture& Copy(const Texture& texture);

    /// @brief Задает имя текстуры
    /// @param string строка с именем
    void SetName(const char* string);

    explicit operator bool() const;
    void* operator new(u64 size);
    void* operator new[](u64 size);
    void operator delete(void* ptr, u64 size);
    void operator delete[](void* ptr, u64 size);
};
