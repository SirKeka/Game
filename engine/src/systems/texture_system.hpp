#pragma once

#include "resources/texture.hpp"
#include "containers/hashtable.hpp"

constexpr const char* DEFAULT_TEXTURE_NAME = "default";                     // Имя текстуры по умолчанию.
constexpr const char* DEFAULT_DIFFUSE_TEXTURE_NAME = "default_diffuse";     // Имя диффизной текстуры по умолчанию.
constexpr const char* DEFAULT_SPECULAR_TEXTURE_NAME = "default_specular";   // Имя зеркальной текстуры по умолчанию.
constexpr const char* DEFAULT_NORMAL_TEXTURE_NAME = "default_normal";       // Имя текстуры нормалей по умолчанию.
struct TextureReference;

namespace ETexture {
    enum ETexture : u8 {
        Default, Diffuse, Specular, Normal
    };
}

using ETextureFlag = u8;

class TextureSystem
{
private:
    u32 MaxTextureCount{};
    Texture DefaultTexture[4]{}; // DefaultTexture, DefaultDiffuseTexture, DefaultSpecularTexture, DefaultNormalTexture

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

    /// @brief Пытается получить текстуру с заданным именем. Если она еще не загружена, это запускает её загрузку. 
    /// Если текстура не найдена, возвращается указатель на текстуру по умолчанию. Если текстура найдена и загружена, ее счетчик ссылок увеличивается.
    /// @param name Имя текстуры, которую нужно найти.
    /// @param AutoRelease Указывает, должна ли текстура автоматически освобождаться, когда ее счетчик ссылок равен 0. Вступает в силу только при первом получении текстуры.
    /// @return Указатель на загруженную текстуру. Может быть указателем на текстуру по умолчанию, если она не найдена.
    Texture* Acquire(const char* name, bool AutoRelease);
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
    Texture* AcquireCube(const char* name, bool AutoRelease);
    /// @brief Пытается получить записываемую текстуру с заданным именем. 
    /// Это не указывает на попытку загрузки файла текстуры и не пытается его загрузить. Также увеличивает счетчик ссылок. 
    /// ПРИМЕЧАНИЕ: Текстуры, доступные для записи, не выгружвются автоматически.
    /// @param name имя текстуры, которую нужно получить.
    /// @param width ширина текстуры в пикселях.
    /// @param height высота текстуры в пикселях.
    /// @param ChannelCount количество каналов в текстуре (обычно 4 для RGBA)
    /// @param HasTransparency указывает, будет ли текстура иметь прозрачность.
    /// @return Указатель на сгенерированную текстуру.
    Texture* AquireWriteable(const char* name, u32 width, u32 height, u8 ChannelCount, bool HasTransparency);
    /// @brief Выгружает текстуру.
    /// @param name название текстуры.
    void Release(const char* name);
    /// @brief Обертывает предоставленные внутренние данные в структуру текстуры, используя предоставленные параметры. 
    /// Это лучше всего использовать, когда система рендеринга создает внутренние ресурсы, и их необходимо передать в систему текстур. 
    /// Можно искать по имени с помощью методов получения. 
    /// ПРИМЕЧАНИЕ: Обернутые текстуры не выпускаются автоматически.
    /// @param name имя текстуры.
    /// @param width ширина текстуры в пикселях.
    /// @param height высота текстуры в пикселях.
    /// @param ChannelCount количество каналов в текстуре (обычно 4 для RGBA)
    /// @param HasTransparency указывает, будет ли текстура иметь прозрачность.
    /// @param IsWriteable указатель на внутренние данные, которые будут установлены в текстуре.
    /// @param RegisterTexture указывает, должна ли текстура быть зарегистрирована в системе.
    /// @param InternalData указатель на обернутую текстуру.
    /// @return Указатель на текстуру.
    template<typename T>
    Texture* WrapInternal(const char* name, u32 width, u32 height, u8 ChannelCount, bool HasTransparency, bool IsWriteable, bool RegisterTexture, T* InternalData) {
        u32 id = INVALID::ID;
        Texture* texture = nullptr;
        if (RegisterTexture) {
            // ПРИМЕЧАНИЕ: Обернутые текстуры никогда не выпускаются автоматически, поскольку это означает, 
            // что их ресурсы создаются и управляются где-то во внутренних компонентах средства рендеринга.
            if (!ProcessTextureReference(name, TextureType::_2D, 1, false, true, id)) {
                MERROR("ТекстурнойСистеме::WrapInternal не удалось получить новый идентификатор текстуры.");
                return nullptr;
            }
            texture = &state->RegisteredTextures[id];
        } else {
            texture = new Texture();
            // MTRACE("TextureSystem::WrapInternal создала текстуру «%s», но не зарегистрировалась, что привело к выделению. Освобождение этой памяти зависит от вызывающего абонента.", name);
        } 
        TextureFlagBits flag = 0;
        flag |= HasTransparency ? TextureFlag::HasTransparency : 0;
        flag |= IsWriteable ? TextureFlag::IsWriteable : 0;
        flag |= TextureFlag::IsWrapped;
        *texture = Texture(id, TextureType::_2D, width, height, ChannelCount, flag, name, InternalData);
        return texture;
    }
    /// @brief Устанавливает внутренние данные текстуры. Полезно, например, для замены внутренних данных внутри средства рендеринга для обернутых текстур.
    /// @param texture указатель на текстуру, которую необходимо обновить.
    /// @param InternalData указатель на внутренние данные, которые необходимо установить.
    /// @return true в случае успеха, иначе false.
    template<typename T>
    bool SetInternal(Texture* texture, T* InternalData) {
        if (texture) {
            texture->Data = InternalData;
            texture->generation++;
            return true;
        }
        return false;
    }
    /// @brief Изменяет размер данной текстуры. Может быть сделано только на записываемых текстурах. Потенциально восстанавливает внутренние данные, если это настроено.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param width новая ширина текстуры в пикселях.
    /// @param height новая высота текстуры в пикселях.
    /// @param RegenerateInternalData указывает, следует ли восстановить внутренние данные.
    /// @return true в случае успеха, иначе false.
    bool Resize(Texture* texture, u32 width, u32 height, bool RegenerateInternalData);
    /// @brief Записывает данные в предоставленную текстуру. Может использоваться только с записываемыми текстурами.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param data указатель на данные, которые необходимо записать.
    /// @return true в случае успеха, иначе false.
    bool WriteData(Texture* texture, u32 offset, u32 size, void* data);
    /// @brief Функция для получения стандартной текстуры.
    /// @return указатель на стандартную текстуру.
    Texture* GetDefaultTexture(ETextureFlag texture);
    static MINLINE TextureSystem* Instance() { return state; };

private:
    bool CreateDefaultTexture();
    void DestroyDefaultTexture();
    static bool LoadTexture(const char* TextureName, Texture* t);
    bool ProcessTextureReference(const char *name, TextureType type, i8 ReferenceDiff, bool AutoRelease, bool SkipLoad, u32 &OutTextureId);
};



