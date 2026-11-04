#pragma once

#include "core/memory_system.h"
#include "containers/mstring.h"

constexpr u32 TEXTURE_NAME_MAX_LENGTH = 512;

struct ImageResourceData {
    u8 ChannelCount  {};
    u32 width        {};
    u32 height       {};
    u8* pixels{nullptr};
    u8 MipLevels     {};

    constexpr ImageResourceData() : ChannelCount(), width(), height(), pixels(nullptr), MipLevels() {}
    constexpr ImageResourceData(u8 ChannelCount, u32 width, u32 height, u8* pixels, u8 MipLevels)
    : ChannelCount(ChannelCount), width(width), height(height), pixels(pixels), MipLevels(MipLevels) {}
    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Texture); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Texture); }
};

/// @brief Параметры, используемые при загрузке изображения.
struct ImageResourceParams {
    bool FlipY; // Указывает, следует ли переворачивать изображение по оси Y при загрузке.
};

enum class TextureUse : u8 {
    Unknown     = 0x00,     // Неизвестное применение. Это значение по умолчанию, но его никогда не следует использовать.
    MapDiffuse  = 0x01,     // Текстура используется как диффузная карта.
    MapSpecular = 0x02,     // Текстура используется как карта отражений.
    MapNormal   = 0x03,     // Текстура используется как карта нормалей.
    Cubemap     = 0x04      // Текстура используется как кубическая карта.
};

/// @brief Представляет поддерживаемые режимы фильтрации текстур.
enum class TextureFilterMode : u8 {
    Nearest = 0x0,      // Фильтрация ближайших соседей.
    Linear  = 0x1       // Линейная (т.е. билинейная) фильтрация.
};

enum class TextureRepeat : u8 {
    Repeat         = 0x1,
    MirroredRepeat = 0x2,
    ClampToEdge    = 0x3,
    ClampToBorder  = 0x4
};

/// @brief Представляет различные типы текстур.
enum class TextureType : u8 {
    _2D,        // Стандартная двухмерная текстура.
    _2DArray,   // Двумерная текстура в виде массива.
    Cube,       // Текстура куба, используемая для кубических карт.
    CubeArray,  // Массив кубических текстур, используемый для массивов кубических карт.
    Count,
};

using TextureFlagBits = u8; // Содержит битовые флаги для текстур.

/// @brief Конфигурация текстуры
/// @param (const_char*)name имя текстуры.
/// @param (u32)width ширина текстуры.
/// @param (u32)height высота текстуры.
/// @param (u16)ArraySize Для массивов текстур, сколько есть «слоёв». В противном случае это 1.
/// @param (u8)MipLevels количество уровней детализации.
/// @param (u8)ChannelCount количество каналов в текстуре.
/// @param (TextureType)type тип текстуры.
/// @param (bool)HasTransparency указывает, имеет ли текстура прозрачность.
/// @param (bool)IsWriteable указывает, можно ли записать (отобразить) текстуру.
/// @param (bool)RegisterTexture указывает, должна ли текстура быть зарегистрирована в системе.
/// @param (bool)Depth указывает, что текстура является текстурой глубины.
/// @param (void*)data указатель на данные текстуры.
struct TextureConfig
{
    const char* name;                   // Имя текстуры.
    u32 width;                          // Ширина текстуры.
    u32 height;                         // Высота текстуры.
    u16 ArraySize{1};                   // Для массивов текстур, сколько есть «слоёв». В противном случае это 1.
    u8 MipLevels{1};                    // Количество уровней детализации.
    u8 ChannelCount;                    // Количество каналов в текстуре.
    TextureType type{TextureType::_2D}; // Тип текстуры.
    bool HasTransparency;               // Указывает, имеет ли текстура прозрачность.
    bool IsWriteable;                   // Указывает, можно ли записать (отобразить) текстуру.
    bool RegisterTexture;               // Указывает, должна ли текстура быть зарегистрирована в системе.
    // bool IsWrapped;                     // Указывает, была ли текстура создана с помощью обертки или традиционного создания.
    bool Depth;                         // Указывает, что текстура является текстурой глубины.
    void* data;                         // Указатель на данные текстуры.
};

struct MAPI Texture
{
    enum { Default, Diffuse, Specular, Normal, Combined, Cube, Terrain, Max };

    enum Flag {
        HasTransparency = 0x1,  // Указывает, имеет ли текстура прозрачность.
        IsWriteable     = 0x2,  // Указывает, можно ли записать (отобразить) текстуру.
        IsWrapped       = 0x4,  // Указывает, была ли текстура создана с помощью обертки или традиционного создания.
        Depth           = 0x8   // Указывает, что текстура является текстурой глубины.
    };

    char name[TEXTURE_NAME_MAX_LENGTH]; // Имя текстуры.
    u32 id;                             // Уникальный идентификатор текстуры.
    u32 generation;                     // Генерация текстур. Увеличивается каждый раз при перезагрузке данных.
    u32 width;                          // Ширина текстуры.
    u32 height;                         // Высота текстуры.
    u16 ArraySize;                      // Для массивов текстур, сколько есть «слоёв». В противном случае это 1.
    TextureType type;                   // Тип текстуры.
    TextureFlagBits flags;              // Содержит битовые флаги для текстур.
    u8 MipLevels;                       // Счетчик указывающий на количество уровней детализации текстуры
    u8 ChannelCount;                    // Количество каналов в текстуре.
    
    void* data;                         // Необработанные данные текстуры (пиксели).

    constexpr Texture() : name(), id(INVALID::ID), generation(INVALID::ID), width(), height(), ArraySize(1), type(), flags(), MipLevels(1), ChannelCount(), data(nullptr) {}
    constexpr Texture(const TextureConfig& config, u32 id = 0)
    : name(), id(id), generation(INVALID::ID), width(config.width), height(config.height), ArraySize(config.ArraySize), 
      type(config.type), flags(), MipLevels(config.MipLevels), ChannelCount(config.ChannelCount), data(config.data) {
        flags |= config.HasTransparency ? HasTransparency : 0;
        flags |= config.IsWriteable ? IsWriteable : 0;
        // flags |= config.IsWrapped ? IsWrapped : 0;
        flags |= config.Depth ? Depth : 0;
        MString::Copy(name, config.name, TEXTURE_NAME_MAX_LENGTH);
    }
    ~Texture();

    /// @brief 
    void Destroy();
    
    Texture(const Texture& t);
    constexpr Texture(Texture&& t) : name(), id(t.id), generation(t.generation), width(t.width), height(t.height), type(t.type),  
    flags(t.flags), MipLevels(t.MipLevels), ChannelCount(t.ChannelCount), data(t.data) { 
        MString::Copy(this->name, t.name, TEXTURE_NAME_MAX_LENGTH); 
        t.id = t.width = t.height = t.generation = 0;
        t.ArraySize = 0;
        t.ChannelCount = t.flags = 0;
        MString::Zero(t.name);
        t.data = nullptr;
    }

    /// @brief Присваивает(не копирует) данный одной текстуры другой. 
    /// @param t текстура данные которой нужно присвоить
    /// @return 
    Texture& operator= (const Texture& t) = default;
    Texture& operator= (Texture&& t);
    void Create(const char* name, i32 width, i32 height, i32 ChannelCount, TextureFlagBits flags);

    void Create(const TextureConfig& config);
    void Create(const TextureConfig* config) { if (config) Create(*config); }

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

/// @brief Структура, которая отображает текстуру, использование и другие свойства.
struct TextureMap {
    Texture* texture{};                     // Указатель на текстуру.
    u32 InternalID;                         // Идентификатор, используемый для поиска и управления внутренними ресурсами.
    u32 generation;                         // Кэшированное поколение назначенной текстуры. Используется для определения того, когда следует повторно генерировать ресурсы этой текстурной карты при изменении поколения текстуры (например, это может потребоваться при изменении MIP-уровней текстуры).
    u8 MipLevels{1};                        // Кэшированные уровни MIP-карт. Должны соответствовать назначенной текстуре. Всегда должны быть не меньше 1.
    TextureFilterMode FilterMinify;         // Режим фильтрации текстур для минимизации.
    TextureFilterMode FilterMagnify;        // Режим фильтрации текстур для увеличения.
    TextureRepeat RepeatU;                  // Режим повтора по оси U (или X, или S)
    TextureRepeat RepeatV;                  // Режим повтора по оси V (или Y, или T)
    TextureRepeat RepeatW;                  // Режим повтора по оси W (или Z, или U)
    
    // void* sampler{};                        // Указатель на внутренние данные, специфичные для API. Обычно внутренний сэмплер.

    constexpr TextureMap(Texture* texture = nullptr) : texture(texture), InternalID(), generation(), MipLevels(1), FilterMinify(), FilterMagnify(), RepeatU(), RepeatV(), RepeatW() {} // sampler(nullptr) {}

    TextureMap(const TextureMap& tm) : texture(new Texture(*tm.texture)), InternalID(tm.InternalID), generation(tm.generation), MipLevels(tm.MipLevels),
    FilterMinify(tm.FilterMinify), FilterMagnify(tm.FilterMagnify), RepeatU(tm.RepeatU), RepeatV(tm.RepeatV), RepeatW(tm.RepeatW) {}// sampler(tm.sampler) {}
    constexpr TextureMap(TextureMap&& tm) : texture(tm.texture), InternalID(tm.InternalID), generation(tm.generation), MipLevels(tm.MipLevels), 
    FilterMinify(tm.FilterMinify), FilterMagnify(tm.FilterMagnify), RepeatU(tm.RepeatU), RepeatV(tm.RepeatV), RepeatW(tm.RepeatW) { // sampler(tm.sampler) {
        tm.texture = nullptr; // tm.sampler = nullptr;
    }

    TextureMap& operator= (TextureMap&& tm) {
        texture = tm.texture; InternalID = tm.InternalID; generation = tm.generation;
        MipLevels = tm.MipLevels; FilterMinify = tm.FilterMinify; FilterMagnify = tm.FilterMagnify;
        RepeatU = tm.RepeatU; RepeatV = tm.RepeatV; RepeatW = tm.RepeatW; // sampler = tm.sampler;

        tm.texture = nullptr;
        // tm.sampler = nullptr;
        return *this;
    }

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
};
