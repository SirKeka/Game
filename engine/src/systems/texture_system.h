#pragma once

#include "resources/texture.h"
#include "containers/hashtable.hpp"
#include <new>

#define DEFAULT_TEXTURE_NAME          "default"             // Имя текстуры по умолчанию.
#define DEFAULT_DIFFUSE_TEXTURE_NAME  "default_diffuse"     // Имя диффизной текстуры по умолчанию.
#define DEFAULT_SPECULAR_TEXTURE_NAME "default_specular"    // Имя зеркальной текстуры по умолчанию.
#define DEFAULT_NORMAL_TEXTURE_NAME   "default_normal"      // Имя текстуры нормалей по умолчанию.
#define DEFAULT_COMBINED_TEXTURE_NAME "default_combined"    // Имя текстуры по умолчанию для комбинированной (металлической, шероховатости, AO) текстуры.
#define DEFAULT_CUBE_TEXTURE_NAME     "default_cube"        // Имя текстуры куба по умолчанию.
#define DEFAULT_TERRAIN_TEXTURE_NAME  "default_terrain"     // Имя текстуры ландшафта по умолчанию.
struct TextureReference;

using ETextureFlag = u8;

/// @brief Конфигурация системы текстур.
struct TextureSystemConfig
{
    /// @brief Максимальное количество текстур, которые можно загрузить одновременно.
    u32 MaxTextureCount;
};

class TextureSystem
{
public:
    TextureSystem(const TextureSystem&) = delete;
    TextureSystem& operator= (const TextureSystem&) = delete;
    static TextureSystem* Initialize(u64& MemoryRequirement, void* memory, TextureSystemConfig& config);
    void Shutdown();

    /// @brief Пытается получить текстуру с заданным именем. Если она еще не загружена, это запускает её загрузку. 
    /// Если текстура не найдена, возвращается указатель на текстуру по умолчанию. Если текстура найдена и загружена, ее счетчик ссылок увеличивается.
    /// @param name Имя текстуры, которую нужно найти.
    /// @param AutoRelease Указывает, должна ли текстура автоматически освобождаться, когда ее счетчик ссылок равен 0. Вступает в силу только при первом получении текстуры.
    /// @return Указатель на загруженную текстуру. Может быть указателем на текстуру по умолчанию, если она не найдена.
    MAPI static Texture* Acquire(const char* name, bool AutoRelease);

    /// @brief Пытается получить текстуру кубической карты с указанным именем. 
    /// Если она еще не загружена, это запускает ее загрузку. 
    /// Если текстура не найдена, возвращается указатель на текстуру по умолчанию. 
    /// Если текстура _найдена_ и загружена, ее счетчик ссылок увеличивается. 
    /// Требуются текстуры с именем в качестве базы, по одной для каждой стороны куба, в следующем порядке:
    /// - name_f Front
    /// - name_b Back
    /// - name_u Up
    /// - name_d Down
    /// - name_r Right
    /// - name_l Left
    ///
    /// Например, «skybox_f.png», «skybox_b.png» и т. д., где имя — «skybox».
    ///
    /// @param name имя текстуры для поиска. Используется как базовая строка для фактических имен текстур.
    /// @param AutoRelease указывает, должна ли текстура автоматически освобождаться, когда ее счетчик ссылок равен 0.
    /// Вступает в силу только при первом получении текстуры.
    /// @return Указатель на загруженную текстуру. Может быть указателем на текстуру по умолчанию, если она не найдена.
    MAPI static Texture* AcquireCube(const char* name, bool AutoRelease);

    /// @brief Попытки получить текстуру-массив с указанным именем. 
    /// Для этого используется предоставленный массив имен текстур, 
    /// чтобы загрузить данные из каждой в свой собственный слой. 
    /// Все текстуры должны иметь одинаковый размер. 
    /// Размер определяется первым файлом в списке.
    /// @param name имя текстуры для получения. Не привязано к имени файла, используется только для поиска.
    /// @param LayerCount количество слоев в массиве текстур (должно быть не менее 1).
    /// @param LayerTextureNames имена текстур для загрузки, по одной на слой. Все текстуры должны иметь одинаковые размеры. Размер массива должен соответствовать layer_count.
    /// @param AutoRelease указывает, будут ли ресурсы текстуры автоматически освобождаться при освобождении последней ссылки.
    /// @return Указатель на сгенерированную текстуру.
    static Texture* AcquireTexturesAsArrayed(const char* name, u32 LayerCount, MString* LayerTextureNames, bool AutoRelease);
    
    /// @brief Пытается получить записываемую текстуру с заданным именем. 
    /// Это не указывает на попытку загрузки файла текстуры и не пытается его загрузить. Также увеличивает счетчик ссылок. 
    /// ПРИМЕЧАНИЕ: Текстуры, доступные для записи, не выгружвются автоматически.
    /// @param name имя текстуры, которую нужно получить.
    /// @param width ширина текстуры в пикселях.
    /// @param height высота текстуры в пикселях.
    /// @param ChannelCount количество каналов в текстуре (обычно 4 для RGBA)
    /// @param HasTransparency указывает, будет ли текстура иметь прозрачность.
    /// @param type Тип текстуры.
    /// @param ArraySize Количество «слоев» в текстуре.
    /// @return Указатель на сгенерированную текстуру.
    MAPI static Texture* AquireWriteable(TextureConfig &config);
    
    /// @brief Выгружает текстуру.
    /// @param name название текстуры.
    MAPI static void Release(const char* name);

    /// @brief Обертывает предоставленные внутренние данные в структуру текстуры, используя предоставленные параметры. 
    /// Это лучше всего использовать, когда система рендеринга создает внутренние ресурсы, и их необходимо передать в систему текстур. 
    /// Можно искать по имени с помощью методов получения. 
    /// ПРИМЕЧАНИЕ: Если OutTexture = nullptr, то в функцию обязательно должна передаваться конфигурация. Обернутые текстуры не выпускаются автоматически.
    /// @param config указатель на конфигурацию текстуры
    /// @param OutTexture необязательный указатель для хранения обернутой текстуры. Если null, вместо этого выделяется и возвращается новый указатель.
    MAPI static void WrapInternal(const TextureConfig* config, Texture* OutTexture);

    /// @brief Устанавливает внутренние данные текстуры. Полезно, например, для замены внутренних данных внутри средства рендеринга для обернутых текстур.
    /// @param texture указатель на текстуру, которую необходимо обновить.
    /// @param InternalData указатель на внутренние данные, которые необходимо установить.
    /// @return true в случае успеха, иначе false.
    MAPI bool SetInternal(Texture* texture, void* InternalData);

    /// @brief Изменяет размер данной текстуры. Может быть сделано только на записываемых текстурах. Потенциально восстанавливает внутренние данные, если это настроено.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param width новая ширина текстуры в пикселях.
    /// @param height новая высота текстуры в пикселях.
    /// @param RegenerateInternalData указывает, следует ли восстановить внутренние данные.
    /// @return true в случае успеха, иначе false.
    MAPI static bool Resize(Texture* texture, u32 width, u32 height, bool RegenerateInternalData);

    /// @brief Записывает данные в предоставленную текстуру. Может использоваться только с записываемыми текстурами.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param data указатель на данные, которые необходимо записать.
    /// @return true в случае успеха, иначе false.
    MAPI static bool WriteData(Texture* texture, u32 offset, u32 size, u8* pixels);

    /// @brief Проверяет, является ли переданная текстура текстурой по умолчанию. Вернет false, если система текстур еще не инициализирована.
    /// @param texture Указатель на проверяемую текстуру.
    /// @returns true, если это текстура по умолчанию, иначе false.
    MAPI static bool IsDefaultTexture(Texture* texture);

    /// @brief Функция для получения стандартной текстуры.
    /// @return указатель на стандартную текстуру.
    MAPI static Texture* GetDefaultTexture(u8 texture = Texture::Default);

private:
    struct Reference {
        u64 ReferenceCount{};
        u32 handle{};
        bool AutoRelease{false};
        constexpr Reference() : ReferenceCount(), handle(), AutoRelease() {}
        constexpr Reference(u64 ReferenceCount, u32 handle, bool AutoRelease) : ReferenceCount(ReferenceCount), handle(handle), AutoRelease(AutoRelease) {}
    };

    u32 MaxTextureCount{};
    Texture DefaultTexture[Texture::Max]; // Default, Diffuse, Specular, Normal, Combined, Cube, Terrain

    /// @brief Массив зарегистрированных текстур.
    Texture* RegisteredTextures;

    /// @brief Хэш-таблица для поиска текстур.
    HashTable<Reference> RegisteredTextureTable;

    static inline TextureSystem* state;

    TextureSystem(u32 MaxTextureCount, Texture* RegisteredTextures, Reference* HashtableBlock)
    : MaxTextureCount(MaxTextureCount), DefaultTexture(), RegisteredTextures(new(RegisteredTextures) Texture[MaxTextureCount]()), 
    RegisteredTextureTable(MaxTextureCount, false, HashtableBlock, true, Reference(0, INVALID::ID, false)) {}

    ~TextureSystem() { Shutdown(); }

    bool CreateDefaultTextures();
    bool ProcessTextureReference(const char *name, i8 ReferenceDiff, bool AutoRelease, u32 &OutTextureId, bool& NeedsCreation);
};



