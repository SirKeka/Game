#include "shader_system.hpp"
#include "memory/linear_allocator.hpp"
#include "systems/texture_system.hpp"
#include "renderer/renderer.hpp"
#include "resources/texture_map.hpp"
#include <new>

ShaderSystem* ShaderSystem::state = nullptr;

ShaderSystem::ShaderSystem(const Config& config, void* LookupMemory, Shader* shaders)
:
MaxShaderCount(config.MaxShaderCount),
MaxUniformCount(config.MaxUniformCount),
MaxGlobalTextures(config.MaxGlobalTextures),
MaxInstanceTextures(config.MaxInstanceTextures),
LookupMemory(LookupMemory),
lookup(MaxShaderCount, false, reinterpret_cast<u32*>(LookupMemory), true, INVALID::ID),
CurrentShaderID(INVALID::ID),
shaders(shaders) 
{
    // Делаем недействительными все идентификаторы шейдеров.
    for (u32 i = 0; i < MaxShaderCount; ++i) {
        this->shaders[i].id = INVALID::ID;
        this->shaders[i].RenderFrameNumber = INVALID::U64ID;
    }
}

ShaderSystem::~ShaderSystem()
{
    // Уничтожьте все существующие шейдеры.
        for (u32 i = 0; i < this->MaxShaderCount; ++i) {
            auto& shader = this->shaders[i];
            if (shader.id != INVALID::ID) {
                shader.Destroy();
            }
        }
        lookup.Destroy();
        
}

bool ShaderSystem::Initialize(const Config& config, LinearAllocator& SystemAllocator)
{
    // Проверьте конфигурацию.
    if (config.MaxShaderCount < 512) {
        if (config.MaxShaderCount == 0) {
            MERROR("ShaderSystem::Initialize — MaxShaderCount должен быть больше 0");
            return false;
        } else {
            // Это сделано для того, чтобы избежать коллизий хеш-таблиц.
            MWARN("ShaderSystem::Initialize — рекомендуется иметь значение MaxShaderCount не ниже 512.");
        }
    }

    if (!state) {
        // Выясняем, какой размер хеш-таблицы необходим.
        // Блок памяти будет содержать структуру состояния, а затем блок хеш-таблицы.
        u64 ClassRequirement = sizeof(ShaderSystem);
        u64 HashtableRequirement = sizeof(u32) * config.MaxShaderCount;
        u64 ShaderArrayRequirement = sizeof(Shader) * config.MaxShaderCount;
        u64 MemoryRequirement = ClassRequirement + HashtableRequirement + ShaderArrayRequirement;
        void* ptrShaderSystem = SystemAllocator.Allocate(MemoryRequirement);
        // Настраиваем указатель состояния, блок памяти, массив шейдеров, затем создаем хеш-таблицу.
        u8* addres = reinterpret_cast<u8*>(ptrShaderSystem);
        state = new(ptrShaderSystem) ShaderSystem(
            config,
            (addres + ClassRequirement), 
            reinterpret_cast<Shader*>(addres + ClassRequirement + HashtableRequirement));
    }
    if (!state){
        MERROR("ShaderSystem::Initialize — Неудалось инициализировать систему шейдеров.");
        return false;
    }
    
    return true;
}

void ShaderSystem::Shutdown()
{
    if (state) {
        state->~ShaderSystem(); // delete state;
        state = nullptr;
    }
}

bool ShaderSystem::Create(Renderpass& pass, const Shader::Config &config)
{
    u32 id = NewShaderID();
    Shader* NewShader = &state->shaders[id];
    new(NewShader) Shader(id, config);
    
    // Флаги процесса.
    if (config.DepthTest) {
        NewShader->flags |= Shader::DepthTestFlag;
    }
    if (config.DepthWrite) {
        NewShader->flags |= Shader::DepthWriteFlag;
    }
    
    if (!Renderer::Load(NewShader, config, &pass, config.StageCount, config.StageFilenames, config.stages.Data())) {
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
            if (NewShader->uniforms.Length() + 1 > state->MaxUniformCount) {
                MERROR("Шейдер может принимать только общее максимальное количество форм и сэмплеров %d в глобальной, экземплярной и локальной областях.", state->MaxUniformCount);
            } else {
                NewShader->AddUniform(config.uniforms[i]);
            }
        }
    }

    // Инициализируйте шейдер.
    if (!Renderer::ShaderInitialize(NewShader)) {
        MERROR("ShaderSystem::Create: не удалось инициализировать шейдер '%s'.", config.name.c_str());
        // ПРИМЕЧАНИЕ: Initialize автоматически уничтожает шейдер в случае сбоя.
        return false;
    }

    // На этом этапе создание прошло успешно, поэтому сохраните идентификатор шейдера в хеш-таблице, чтобы позже можно было найти его по имени.
    if (!state->lookup.Set(config.name.c_str(), NewShader->id)) {
        // Черт возьми, мы зашли так далеко... Что ж, удалите шейдер и перезапустите.
        Renderer::Unload(NewShader);
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
    if (ShaderID >= state->MaxShaderCount || state->shaders[ShaderID].id == INVALID::ID) {
        return nullptr;
    }
    return &state->shaders[ShaderID];
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
    if (state->CurrentShaderID != ShaderID) {
        Shader* NextShader = GetShader(ShaderID);
        state->CurrentShaderID = ShaderID;
        if (!Renderer::ShaderUse(NextShader)) {
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
    if (state->CurrentShaderID == INVALID::ID) {
        MERROR("ShaderSystem::UniformSet вызывается без использования шейдера.");
        return false;
    }
    Shader* shader = &state->shaders[state->CurrentShaderID];
    u16 index = UniformIndex(shader, UniformName);
    return UniformSet(index, value);
}

bool ShaderSystem::UniformSet(u16 index, const void *value)
{
    auto& shader = state->shaders[state->CurrentShaderID];
    auto& uniform = shader.uniforms[index];
    if (shader.BoundScope != uniform.scope) {
        if (uniform.scope == Shader::Scope::Global) {
            shader.BindGlobals();
        } else if (uniform.scope == Shader::Scope::Instance) {
            shader.BindInstance(shader.BoundInstanceID);
        } else {
            // ПРИМЕЧАНИЕ: Больше здесь делать нечего, просто установите униформу.
        }
        shader.BoundScope = uniform.scope;
    }
    return Renderer::SetUniform(&shader, &uniform, value);
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
    return Renderer::ShaderApplyGlobals(&state->shaders[state->CurrentShaderID]);
}

bool ShaderSystem::ApplyInstance(bool NeedsUpdate)
{
    return Renderer::ShaderApplyInstance(&state->shaders[state->CurrentShaderID], NeedsUpdate);
}

bool ShaderSystem::BindInstance(u32 InstanceID)
{
    auto& s = state->shaders[state->CurrentShaderID];
    s.BoundInstanceID = InstanceID;
    return s.BindInstance(InstanceID);
}

bool ShaderSystem::AddSampler(Shader *shader, const Shader::UniformConfig &config)
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
        if (GlobalTextureCount + 1 > state->MaxGlobalTextures) {
            MERROR("Глобальное количество текстур шейдера %i превышает максимальное значение %i", GlobalTextureCount, state->MaxGlobalTextures);
            return false;
        }
        location = GlobalTextureCount;
        // ПРИМЕЧАНИЕ: создание карты текстур по умолчанию, которая будет использоваться здесь. Всегда можно обновить позже.
        // Выделите указатель, назначьте текстуру и вставьте ее в глобальные карты текстур.
        // ПРИМЕЧАНИЕ: Это распределение выполняется только для глобальных карт текстур.
        auto DefaultMap = new TextureMap();
        if (!Renderer::TextureMapAcquireResources(DefaultMap)) {
            MERROR("Не удалось получить ресурсы для глобальной карты текстур во время создания шейдера.");
            return false;
        }

        DefaultMap->texture = TextureSystem::Instance()->GetDefaultTexture(ETexture::Default);
        shader->GlobalTextureMaps.PushBack(DefaultMap);
    } else {
        // В противном случае это происходит на уровне экземпляра, поэтому подсчитывайте, сколько ресурсов необходимо добавить во время получения ресурса.
        if (shader->InstanceTextureCount + 1 > state->MaxInstanceTextures) {
            MERROR("Количество текстур экземпляра шейдера %i превышает максимальное значение %i", shader->InstanceTextureCount, state->MaxInstanceTextures);
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

u32 ShaderSystem::GetShaderID(const MString &ShaderName)
{
    u32 ShaderID = INVALID::ID;
    if (!state->lookup.Get(ShaderName.c_str(), &ShaderID)) {
        MERROR("Не зарегистрирован ни один шейдер с именем '%s'.", ShaderName.c_str());
        return INVALID::ID;
    }
    return ShaderID;
}

u32 ShaderSystem::NewShaderID()
{
    for (u32 i = 0; i < state->MaxShaderCount; ++i) {
        if (state->shaders[i].id == INVALID::ID) {
            return i;
        }
    }
    return INVALID::ID;
}

bool ShaderSystem::UniformAdd(Shader* shader, const char *UniformName, u32 size, Shader::UniformType type, Shader::Scope scope, u32 SetLocation, bool IsSampler)
{
    if (shader->uniforms.Length() + 1 > state->MaxUniformCount) {
        MERROR("Шейдер может принимать только общее максимальное количество форм и сэмплеров %d в глобальной, экземплярной и локальной областях.", state->MaxUniformCount);
        return false;
    }
    return shader->UniformAdd(UniformName, size, type, scope, SetLocation, IsSampler);
}
