/// @file shader_system.hpp
/// @author 
/// @brief Система управления шейдерами. Отвечает за работу с средством рендеринга для создания, 
/// уничтожения, связывания/отмены привязки и установки свойств шейдера, таких как униформы.
/// @version 1.0
/// @date 2024-05-18 
/// @copyright 

#pragma once

#include "defines.hpp"
#include "containers/hashtable.hpp"
#include "resources/shader.hpp"
#include "resources/texture.hpp"

struct Renderpass;

namespace ShaderSystem
{
    struct Config {
        u16 MaxShaderCount;
        u8  MaxUniformCount;
        u8  MaxGlobalTextures;
        u8  MaxInstanceTextures;
    };

    /// @brief Инициализирует шейдерную систему, используя предоставленную конфигурацию. 
    /// ПРИМЕЧАНИЕ: Вызовите это дважды: один раз, чтобы получить требуемую память (память = 0), а второй раз, включая выделенную память.
    /// @param MaxShaderCount максимальное количество шейдеров, которое будет хранится в системе. ПРИМЕЧАНИЕ: Должно быть не менее 512.
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    /// @brief Выключает шейдерную систему.
    void Shutdown();

    /// @brief Создает новый шейдер с заданной конфигурацией. 
    /// @param config конфигурация, которая будет использоваться при создании шейдера.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Create(Renderpass& pass, Shader::Config& config);
    /// @brief Получает идентификатор шейдера по имени.
    /// @param ShaderName имя шейдера.
    /// @return Идентификатор шейдера, если он найден; в противном случае INVALID::ID.
    MAPI u32 GetID(const MString& ShaderName);
    /// @brief Возвращает указатель на шейдер с заданным идентификатором.
    /// @param ShaderID идентификатор шейдера.
    /// @return Указатель на шейдер, если он найден; иначе nullptr.
    MAPI Shader* GetShader(u32 ShaderID);
    /// @brief Возвращает указатель на шейдер с заданным идентификатором.
    /// @param ShaderName имя шейдера.
    /// @return Идентификатор шейдера, если он найден; в противном случае INVALID::ID.
    MAPI Shader* GetShader(const MString& ShaderName);
    /// @brief Использует шейдер с заданным именем.
    /// @param ShaderName имя шейдера.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Use(const char* ShaderName);
    /// @brief Использует шейдер с заданным идентификатором.
    /// @param ShaderID идентификатор шейдера.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Use(u32 ShaderID);
    /// @brief Возвращает индекс униформы для униформы с заданным именем, если он найден.
    /// @param s указатель на шейдер, из которого нужно получить индекс.
    /// @param UniformName название униформы, которую нужно найти.
    /// @return Единый индекс, если он найден; в противном случае INVALID::U16ID.
    MAPI u16 UniformIndex(Shader* shader, const char* UniformName);
    /// @brief Устанавливает значение униформы с заданным именем в указанное значение. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param UniformName имя униформы, которую нужно установить.
    /// @param value значение, которое необходимо установить.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool UniformSet(const char* UniformName, const void* value);
    /// @brief Устанавливает единое значение по индексу. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param index индекс униформы.
    /// @param value стоимость униформы.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool UniformSet(u16 index, const void* value);
    /// @brief Устанавливает текстуру семплера с заданным именем в предоставленную текстуру. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param SamplerName имя униформы, которую нужно установить.
    /// @param t указатель на текстуру, которую нужно установить.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool SamplerSet(const char* SamplerName, const Texture* texture);
    /// @brief Устанавливает значение сэмплера по индексу. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param index индекс униформы.
    /// @param texture указатель на текстуру, которую нужно установить.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool SamplerSet(u16 index, const Texture* texture);
    /// @brief Применяет глобальную униформу. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool ApplyGlobal();
    /// @brief Применяет униформы на уровне экземпляра. ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool ApplyInstance(bool NeedsUpdate);
    /// @brief Связывает экземпляр с заданным идентификатором для использования. 
    /// Это необходимо сделать до установки униформ на уровне экземпляра. 
    /// ПРИМЕЧАНИЕ: Работает с текущим шейдером.
    /// @param InstanceID Идентификатор экземпляра для привязки.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool BindInstance(u32 InstanceID);
};
