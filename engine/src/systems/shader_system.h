/// @file shader_system.h
/// @author 
/// @brief Система управления шейдерами. Отвечает за работу с средством рендеринга для создания, 
/// уничтожения, связывания/отмены привязки и установки свойств шейдера, таких как униформы.
/// @version 1.0
/// @date 2024-05-18 
/// @copyright 

#pragma once
#include "containers/hashtable.hpp"

struct Shader;
struct Renderpass;
struct ShaderConfig;
struct Texture;
struct FrameData;

class ShaderSystem
{
public:
    struct Config {
        u16 MaxShaderCount;
        u8  MaxUniformCount;
        u8  MaxGlobalTextures;
        u8  MaxInstanceTextures;
    };

    NON_COPYABLE(ShaderSystem);

    /// @brief Инициализирует шейдерную систему, используя предоставленную конфигурацию. 
    /// ПРИМЕЧАНИЕ: Вызовите это дважды: один раз, чтобы получить требуемую память (память = 0), а второй раз, включая выделенную память.
    /// @param MaxShaderCount максимальное количество шейдеров, которое будет хранится в системе. ПРИМЕЧАНИЕ: Должно быть не менее 512.
    static ShaderSystem* Initialize(u64& MemoryRequirement, void* memory, Config& config);
    /// @brief Выключает шейдерную систему.
    void Shutdown();

    /// @brief Создает новый шейдер с заданной конфигурацией. 
    /// @param config конфигурация, которая будет использоваться при создании шейдера.
    /// @return true в случае успеха; в противном случае false.
    MAPI static Shader* CreateShader(Renderpass& pass, ShaderConfig& config);

    /// @brief Получает идентификатор шейдера по имени.
    /// @param ShaderName имя шейдера.
    /// @return Идентификатор шейдера, если он найден; в противном случае INVALID::ID.
    MAPI static u32 GetShaderID(const MString& ShaderName);

    /// @brief Возвращает указатель на шейдер с заданным идентификатором.
    /// @param ShaderID идентификатор шейдера.
    /// @return Указатель на шейдер, если он найден; иначе nullptr.
    MAPI static Shader* GetShader(u32 ShaderID);

    /// @brief Возвращает указатель на шейдер с заданным идентификатором.
    /// @param ShaderName имя шейдера.
    /// @return Идентификатор шейдера, если он найден; в противном случае INVALID::ID.
    MAPI static Shader* GetShader(const MString& ShaderName);

    static ShaderSystem* GetState() { return pShaderSystem; }

    /// @brief Использует шейдер с заданным именем.
    /// @param ShaderName имя шейдера.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool Use(const char* ShaderName);

    /// @brief Использует шейдер с заданным идентификатором.
    /// @param ShaderID идентификатор шейдера.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool Use(u32 ShaderID);

    /// @brief Устанавливает значение униформы с заданным именем в указанное значение. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param UniformName имя униформы, которую нужно установить.
    /// @param value значение, которое необходимо установить.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool UniformSet(const char* UniformName, const void* value) {
        return UniformSetArrayed(UniformName, 0, value);
    }

    /// @brief Устанавливает единое значение по индексу. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param index индекс униформы.
    /// @param value стоимость униформы.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool UniformSet(u16 location, const void* value) {
        return UniformSetArrayed(location, 0, value);
    }

    /// @brief Устанавливает значение массивного uniform-параметра с заданным именем в указанное значение. ПРИМЕЧАНИЕ: Работает с текущим используемым шейдером.
    /// @param UniformName имя uniform-параметра, который нужно установить.
    /// @param ArrayIndex индекс в массиве uniform-параметров, если uniform действительно является массивом. В противном случае игнорируется.
    /// @param value значение, которое нужно установить.
    /// @return Возвращает true при успехе; в противном случае false.
    MAPI static bool UniformSetArrayed(const char* UniformName, u32 ArrayIndex, const void* value);

    /// @brief Устанавливает значение uniform по местоположению. ПРИМЕЧАНИЕ: Работает с текущим используемым шейдером. 
    /// @param location местоположение uniform. 
    /// @param ArrayIndex индекс в массиве uniform, если uniform действительно является массивом. В противном случае игнорируется. 
    /// @param value значение uniform.
    /// @return Возвращает true при успешном выполнении; в противном случае false.
    MAPI static bool UniformSetArrayed(u16 location, u32 ArrayIndex, const void* value);

    /// @brief Устанавливает текстуру семплера с заданным именем в предоставленную текстуру. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param SamplerName имя униформы, которую нужно установить.
    /// @param t указатель на текстуру, которую нужно установить.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool SamplerSet(const char* SamplerName, const Texture* texture) {
        return UniformSetArrayed(SamplerName, 0, texture);
    }

    /// @brief Устанавливает значение сэмплера по индексу. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param index индекс униформы.
    /// @param texture указатель на текстуру, которую нужно установить.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool SamplerSet(u16 index, const Texture* texture) {
        return UniformSetArrayed(index, 0, texture);
    }

    /// @brief Устанавливает текстуру для массивного сэмплера с заданным именем на предоставленную текстуру. ПРИМЕЧАНИЕ: Осуществляется в текущем используемом шейдере.  
    /// @param UniformName имя униформа, который нужно установить.  
    /// @param ArrayIndex индекс в массиве униформов, если униформ действительно является массивом. В противном случае игнорируется.  
    /// @param texture указатель на текстуру, которую нужно установить.  
    /// @return Возвращает true в случае успеха; иначе возвращает false.
    MAPI bool SamplerSetArrayed(const char* SamplerName, u32 ArrayIndex, const Texture* texture) {
        return UniformSetArrayed(SamplerName, ArrayIndex, texture);
    }

    /// @brief Устанавливает текстуру для массивного сэмплера с заданным именем на предоставленную текстуру. ПРИМЕЧАНИЕ: Осуществляется в текущем используемом шейдере.  
    /// @param location имя униформа, который нужно установить.  
    /// @param ArrayIndex индекс в массиве униформов, если униформ действительно является массивом. В противном случае игнорируется.  
    /// @param texture указатель на текстуру, которую нужно установить.  
    /// @return Возвращает true в случае успеха; иначе возвращает false.
    MAPI bool SamplerSetArrayed(u16 location, u32 ArrayIndex, const Texture* texture) {
        return UniformSetArrayed(location, ArrayIndex, texture);
    }

    /// @brief Применяет глобальную униформу. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool ApplyGlobal(bool NeedsUpdate, const FrameData& rFrameData);

    /// @brief Применяет униформы на уровне экземпляра. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool ApplyInstance(bool NeedsUpdate, const FrameData& rFrameData);

    /// @brief Применяет униформы с локальной областью действия. ПРИМЕЧАНИЕ: Работает с используемым в данный момент шейдером.
    /// @return true при успешном выполнении в противном случае false.
    MAPI static bool ApplyLocal(const FrameData& rFrameData);
    
    /// @brief Связывает экземпляр с заданным идентификатором для использования. 
    /// Это необходимо сделать до установки униформ на уровне экземпляра. 
    /// ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param InstanceID Идентификатор экземпляра для привязки.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool BindInstance(u32 InstanceID);

    /// @brief Привязывает экземпляр с указанным идентификатором для использования. Должно быть выполнено перед установкой униформ с локальной областью видимости. 
    /// ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @return true при успешном выполнении в противном случае false.
    MAPI static bool BindLocal();
private:
    // Конфигурация шейдерной системы.--------------------------------------------------------------------------------------------------
    u16 MaxShaderCount;                 // Максимальное количество шейдеров, хранящихся в системе. ПРИМЕЧАНИЕ: Должно быть не менее 512.
    u8  MaxUniformCount;                // Максимальное количество униформ, разрешенное в одном шейдере.
    u8  MaxGlobalTextures;              // Максимальное количество текстур глобальной области действия, разрешенное в одном шейдере.
    u8  MaxInstanceTextures;            // Максимальное количество текстур экземпляра, разрешенное в одном шейдере.
    // ---------------------------------------------------------------------------------------------------------------------------------
    [[maybe_unused]]void* LookupMemory; // Память, используемая для таблицы поиска.
    HashTable<u32> lookup;              // Таблица поиска имени шейдера->идентификатор.
    u32 CurrentShaderID;                // Идентификатор текущего привязанного шейдера.
    Shader* shaders;                    // Коллекция созданных шейдеров.
    // ---------------------------------------------------------------------------------------------------------------------------------

    static inline ShaderSystem* pShaderSystem;

    friend struct Shader;

    ShaderSystem(Config& config, void* LookupMemory, Shader* shaders);
#ifdef _DEBUG
    static bool FileWatchEvent(u16 code, void* sender, void* ListenerInst, const struct EventContext& context);
#endif
    // static bool UniformAdd(Shader* shader, const char* UniformName, u32 size, ShaderUniformType type, ShaderScope scope, u32 SetLocation, bool IsSampler);
};
