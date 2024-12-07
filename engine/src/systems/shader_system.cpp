#include "shader_system.hpp"
#include "memory/linear_allocator.hpp"
#include "systems/texture_system.hpp"
#include "renderer/rendering_system.hpp"
#include "resources/texture_map.hpp"
#include <new>

struct sShaderSystem
{
    // Конфигурация шейдерной системы.--------------------------------------------------------------------------------------------------
    u16 MaxShaderCount;                 // Максимальное количество шейдеров, хранящихся в системе. ПРИМЕЧАНИЕ: Должно быть не менее 512.
    u8  MaxUniformCount;                // Максимальное количество униформ, разрешенное в одном шейдере.
    u8  MaxGlobalTextures;              // Максимальное количество текстур глобальной области действия, разрешенное в одном шейдере.
    u8  MaxInstanceTextures;            // Максимальное количество текстур экземпляра, разрешенное в одном шейдере.
    // ---------------------------------------------------------------------------------------------------------------------------------
    void* LookupMemory;                 // Память, используемая для таблицы поиска.
    HashTable<u32> lookup;              // Таблица поиска имени шейдера->идентификатор.
    u32 CurrentShaderID;                // Идентификатор текущего привязанного шейдера.
    Shader* shaders;                    // Коллекция созданных шейдеров.
    // ---------------------------------------------------------------------------------------------------------------------------------
    sShaderSystem(ShaderSystem::Config* config, void* LookupMemory, Shader* shaders)
    :
    MaxShaderCount(config->MaxShaderCount),
    MaxUniformCount(config->MaxUniformCount),
    MaxGlobalTextures(config->MaxGlobalTextures),
    MaxInstanceTextures(config->MaxInstanceTextures),
    LookupMemory(LookupMemory),
    lookup(MaxShaderCount, false, reinterpret_cast<u32*>(LookupMemory), true, INVALID::ID),
    CurrentShaderID(INVALID::ID),
    shaders(shaders) 
    {
        // Делаем недействительными все идентификаторы шейдеров.
        for (u32 i = 0; i < MaxShaderCount; ++i) {
            shaders[i].id = INVALID::ID;
            shaders[i].RenderFrameNumber = INVALID::U64ID;
        }
    }
};

static sShaderSystem* pShaderSystem = nullptr;

/// @brief Добавляет образец текстуры в шейдер. Должно быть сделано после инициализации шейдера.
/// @param config конфигурация униформы.
/// @return True в случае успеха; в противном случае ложь.
bool AddSampler(Shader* shader, const Shader::UniformConfig& config);
u32 GetShaderID(const MString& ShaderName);
u32 NewShaderID();
bool UniformAdd(Shader* shader, const char* UniformName, u32 size, Shader::UniformType type, Shader::Scope scope, u32 SetLocation, bool IsSampler);

bool ShaderSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<ShaderSystem::Config*>(config);
    // Проверьте конфигурацию.
    if (pConfig->MaxShaderCount < 512) {
        if (pConfig->MaxShaderCount == 0) {
            MERROR("ShaderSystem::Initialize — MaxShaderCount должен быть больше 0");
            return false;
        } else {
            // Это сделано для того, чтобы избежать коллизий хеш-таблиц.
            MWARN("ShaderSystem::Initialize — рекомендуется иметь значение MaxShaderCount не ниже 512.");
        }
    }

    // Выясняем, какой размер хеш-таблицы необходим.
    // Блок памяти будет содержать структуру состояния, а затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(sShaderSystem);
    u64 HashtableRequirement = sizeof(u32) * pConfig->MaxShaderCount;
    u64 ShaderArrayRequirement = sizeof(Shader) * pConfig->MaxShaderCount;
    MemoryRequirement = StructRequirement + HashtableRequirement + ShaderArrayRequirement;

    if (!memory) {
        return true;
    }

    if (!pShaderSystem) {
        // Настраиваем указатель состояния, блок памяти, массив шейдеров, затем создаем хеш-таблицу.
        u8* addres = reinterpret_cast<u8*>(memory);
        pShaderSystem = new(memory) sShaderSystem(
            pConfig,
            (addres + StructRequirement), 
            reinterpret_cast<Shader*>(addres + StructRequirement + HashtableRequirement));
    }

    if (!pShaderSystem){
        // MERROR("ShaderSystem::Initialize — Неудалось инициализировать систему шейдеров.");
        return false;
    }
    
    return true;
}

void ShaderSystem::Shutdown()
{
    if (pShaderSystem) {
        // Уничтожьте все существующие шейдеры.
        for (u32 i = 0; i < pShaderSystem->MaxShaderCount; ++i) {
            auto& shader = pShaderSystem->shaders[i];
            if (shader.id != INVALID::ID) {
                shader.Destroy();
            }
        }
        pShaderSystem->lookup.Destroy();
        pShaderSystem = nullptr;
    }
}

bool ShaderSystem::Create(Renderpass& pass, const Shader::Config &config)
{
    u32 id = NewShaderID();
    auto NewShader = &pShaderSystem->shaders[id];
    new(NewShader) Shader(id, config);
    
    // Флаги процесса.
    if (config.DepthTest) {
        NewShader->flags |= Shader::DepthTestFlag;
    }
    if (config.DepthWrite) {
        NewShader->flags |= Shader::DepthWriteFlag;
    }
    
    if (!RenderingSystem::Load(NewShader, config, &pass, config.StageCount, config.StageFilenames, config.stages.Data())) {
        MERROR("Ошибка создания шейдера.");
        return false;
    }

    // Готов к инициализации.
    NewShader->state = Shader::State::Uninitialized;

    // Атрибуты процесса
    for (u32 i = 0; i < config.AttributeCount; ++i) {
        NewShader->AddAttribute(config.attributes[i]);
    }

    // Технологическая униформа i = 1
    for (u32 i = 0; i < config.UniformCount; ++i) {
        if (config.uniforms[i].type == Shader::UniformType::Sampler) {
            AddSampler(NewShader, config.uniforms[i]);
        } else {
            if (NewShader->uniforms.Length() + 1 > pShaderSystem->MaxUniformCount) {
                MERROR("Шейдер может принимать только общее максимальное количество форм и сэмплеров %d в глобальной, экземплярной и локальной областях.", pShaderSystem->MaxUniformCount);
            } else {
                NewShader->AddUniform(config.uniforms[i]);
            }
        }
    }

    // Инициализируйте шейдер.
    if (!RenderingSystem::ShaderInitialize(NewShader)) {
        MERROR("ShaderSystem::Create: не удалось инициализировать шейдер '%s'.", config.name.c_str());
        // ПРИМЕЧАНИЕ: Initialize автоматически уничтожает шейдер в случае сбоя.
        return false;
    }

    // На этом этапе создание прошло успешно, поэтому сохраните идентификатор шейдера в хеш-таблице, чтобы позже можно было найти его по имени.
    if (!pShaderSystem->lookup.Set(config.name.c_str(), NewShader->id)) {
        // Черт возьми, мы зашли так далеко... Что ж, удалите шейдер и перезапустите.
        RenderingSystem::Unload(NewShader);
        return false;
    }

    return true;
}

u32 ShaderSystem::GetID(const MString& ShaderName)
{
    return GetShaderID(ShaderName);
}

Shader *ShaderSystem::GetShader(u32 ShaderID)
{
    if (ShaderID >= pShaderSystem->MaxShaderCount || pShaderSystem->shaders[ShaderID].id == INVALID::ID) {
        return nullptr;
    }
    return &pShaderSystem->shaders[ShaderID];
}

Shader *ShaderSystem::GetShader(const MString &ShaderName)
{
    u32 ShaderID = GetShaderID(ShaderName);
    if (ShaderID != INVALID::ID) {
        return GetShader(ShaderID);
    }
    return nullptr;
}

bool ShaderSystem::Use(const char *ShaderName)
{
    u32 NextShaderID = GetShaderID(ShaderName);
    if (NextShaderID == INVALID::ID) {
        return false;
    }

    return Use(NextShaderID);
}

bool ShaderSystem::Use(u32 ShaderID)
{
    // Выполняйте использование только в том случае, если идентификатор шейдера отличается.
    if (pShaderSystem->CurrentShaderID != ShaderID) {
        auto NextShader = GetShader(ShaderID);
        pShaderSystem->CurrentShaderID = ShaderID;
        if (!RenderingSystem::ShaderUse(NextShader)) {
            MERROR("Не удалось использовать шейдер '%s'.", NextShader->name.c_str());
            return false;
        }
        if (!NextShader->BindGlobals()) {
            MERROR("Не удалось привязать глобальные переменные для шейдера. '%s'.", NextShader->name.c_str());
            return false;
        }
    }
    return true;
}

u16 ShaderSystem::UniformIndex(Shader *shader, const char *UniformName)
{
    return shader->UniformIndex(UniformName);
}

bool ShaderSystem::UniformSet(const char *UniformName, const void *value)
{
    if (pShaderSystem->CurrentShaderID == INVALID::ID) {
        MERROR("ShaderSystem::UniformSet вызывается без использования шейдера.");
        return false;
    }
    Shader* shader = &pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    u16 index = UniformIndex(shader, UniformName);
    return UniformSet(index, value);
}

bool ShaderSystem::UniformSet(u16 index, const void *value)
{
    auto& shader = pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    auto& uniform = shader.uniforms[index];
    if (shader.BoundScope != uniform.scope) {
        if (uniform.scope == Shader::Scope::Global) {
            shader.BindGlobals();
        } else if (uniform.scope == Shader::Scope::Instance) {
            RenderingSystem::ShaderBindInstance(&shader, shader.BoundInstanceID);
        } else {
            // ПРИМЕЧАНИЕ: Больше здесь делать нечего, просто установите униформу.
        }
        shader.BoundScope = uniform.scope;
    }
    return RenderingSystem::SetUniform(&shader, &uniform, value);
}

bool ShaderSystem::SamplerSet(const char *SamplerName, const Texture *texture)
{
    return UniformSet(SamplerName, texture);
}

bool ShaderSystem::SamplerSet(u16 index, const Texture *texture)
{
    return UniformSet(index, texture);
}

bool ShaderSystem::ApplyGlobal()
{
    return RenderingSystem::ShaderApplyGlobals(&pShaderSystem->shaders[pShaderSystem->CurrentShaderID]);
}

bool ShaderSystem::ApplyInstance(bool NeedsUpdate)
{
    return RenderingSystem::ShaderApplyInstance(&pShaderSystem->shaders[pShaderSystem->CurrentShaderID], NeedsUpdate);
}

bool ShaderSystem::BindInstance(u32 InstanceID)
{
    auto& s = pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    s.BoundInstanceID = InstanceID;
    return RenderingSystem::ShaderBindInstance(&s, InstanceID);
}

bool AddSampler(Shader *shader, const Shader::UniformConfig &config)
{
    // Образцы нельзя использовать для push-констант.
    if (config.scope == Shader::Scope::Local) {
        MERROR("Shader::AddSampler невозможно добавить сэмплер в локальной области.");
        return false;
    }

    // Убедитесь, что имя действительно и уникально.
    if (!shader->UniformNameValid(config.name) || !shader->UniformAddStateValid()) {
        return false;
    }

    // Если глобальный, вставьте в глобальный список.
    u32 location = 0;
    if (config.scope == Shader::Scope::Global) {
        u32 GlobalTextureCount = shader->GlobalTextureMaps.Length();
        if (GlobalTextureCount + 1 > pShaderSystem->MaxGlobalTextures) {
            MERROR("Глобальное количество текстур шейдера %i превышает максимальное значение %i", GlobalTextureCount, pShaderSystem->MaxGlobalTextures);
            return false;
        }
        location = GlobalTextureCount;
        // ПРИМЕЧАНИЕ: создание карты текстур по умолчанию, которая будет использоваться здесь. Всегда можно обновить позже.
        // Выделите указатель, назначьте текстуру и вставьте ее в глобальные карты текстур.
        // ПРИМЕЧАНИЕ: Это распределение выполняется только для глобальных карт текстур.
        auto DefaultMap = new TextureMap();
        if (!RenderingSystem::TextureMapAcquireResources(DefaultMap)) {
            MERROR("Не удалось получить ресурсы для глобальной карты текстур во время создания шейдера.");
            return false;
        }

        DefaultMap->texture = TextureSystem::GetDefaultTexture(Texture::Default);
        shader->GlobalTextureMaps.PushBack(DefaultMap);
    } else {
        // В противном случае это происходит на уровне экземпляра, поэтому подсчитывайте, сколько ресурсов необходимо добавить во время получения ресурса.
        if (shader->InstanceTextureCount + 1 > pShaderSystem->MaxInstanceTextures) {
            MERROR("Количество текстур экземпляра шейдера %i превышает максимальное значение %i", shader->InstanceTextureCount, pShaderSystem->MaxInstanceTextures);
            return false;
        }
        location = shader->InstanceTextureCount;
        shader->InstanceTextureCount++;
    }

    // Относитесь к этому как к униформе. ПРИМЕЧАНИЕ: В случае сэмплеров OutLocation используется для непосредственного определения 
    // значения поля «location» записи хэш-таблицы, а затем ему присваивается индекс универсального массива. Это позволяет осуществлять 
    // поиск местонахождения сэмплеров, как если бы они были униформой (поскольку технически это так и есть). 
    // ЗАДАЧА: возможно, придется сохранить это в другом месте
    if (!shader->UniformAdd(config.name, 0, config.type, config.scope, location, true)) {
        MERROR("Невозможно добавить форму сэмплера.");
        return false;
    }

    return true;
}

u32 GetShaderID(const MString &ShaderName)
{
    u32 ShaderID = INVALID::ID;
    if (!pShaderSystem->lookup.Get(ShaderName.c_str(), &ShaderID)) {
        MERROR("Не зарегистрирован ни один шейдер с именем '%s'.", ShaderName.c_str());
        return INVALID::ID;
    }
    return ShaderID;
}

u32 NewShaderID()
{
    for (u32 i = 0; i < pShaderSystem->MaxShaderCount; ++i) {
        if (pShaderSystem->shaders[i].id == INVALID::ID) {
            return i;
        }
    }
    return INVALID::ID;
}

bool UniformAdd(Shader* shader, const char *UniformName, u32 size, Shader::UniformType type, Shader::Scope scope, u32 SetLocation, bool IsSampler)
{
    if (shader->uniforms.Length() + 1 > pShaderSystem->MaxUniformCount) {
        MERROR("Шейдер может принимать только общее максимальное количество форм и сэмплеров %d в глобальной, экземплярной и локальной областях.", pShaderSystem->MaxUniformCount);
        return false;
    }
    return shader->UniformAdd(UniformName, size, type, scope, SetLocation, IsSampler);
}
