#include "shader_system.h"
#include "texture_system.h"
#include "core/event.h"
#include "core/systems_manager.h"
#include "memory/linear_allocator.h"
#include "renderer/renderer_utils.h"
#include "renderer/rendering_system.h"

#include <new>
  
ShaderSystem::ShaderSystem(Config& config, void* LookupMemory, Shader* shaders)
:
MaxShaderCount(config.MaxShaderCount),
MaxUniformCount(config.MaxUniformCount),
MaxGlobalTextures(config.MaxGlobalTextures),
MaxInstanceTextures(config.MaxInstanceTextures),
LookupMemory(LookupMemory),
lookup(MaxShaderCount, false, (u32*)LookupMemory, true, INVALID::ID),
CurrentShaderID(INVALID::ID),
shaders(shaders) 
{
    // Делаем недействительными все идентификаторы шейдеров.
    for (u32 i = 0; i < MaxShaderCount; ++i) {
        shaders[i].id = INVALID::ID;
        shaders[i].RenderFrameNumber = INVALID::U64ID;
    }
}

#ifdef _DEBUG
bool ShaderSystem::FileWatchEvent(u16 code, void *sender, void *ListenerInst, const EventContext& context)
{
    auto ShaderSys = (ShaderSystem*)ListenerInst;
    if (code == EventSystem::WatchedFileWritten) {
        u32 FileWatchID = context.data.u32[0];

        // Найдите среди шейдеров тот, у которого изменился ID отслеживания файла.
        for (u32 i = 0; i < ShaderSys->MaxShaderCount; ++i) {
            Shader& shader = ShaderSys->shaders[i];
            for (u32 w = 0; w < shader.ShaderStageCount; ++w) {
                if (shader.ModuleWatchIDs[w] == FileWatchID) {
                    if (!shader.Reload()) {
                        MWARN("Не удалось выполнить горячую перезагрузку шейдера «%s». Подробности см. в журналах..", shader.name.c_str());
                        // Позволить другим системам подхватить это.
                        return false;
                    }
                }
            }
        }
    }

    // Return as unhandled to allow other systems to pick it up.
    return false;
}
#endif

ShaderSystem* ShaderSystem::Initialize(u64& MemoryRequirement, void* memory, Config& config)
{
    // Проверьте конфигурацию.
    if (config.MaxShaderCount < 512) {
        if (config.MaxShaderCount == 0) {
            MERROR("ShaderSystem::Initialize — MaxShaderCount должен быть больше 0");
            return nullptr;
        } else {
            // Это сделано для того, чтобы избежать коллизий хеш-таблиц.
            MWARN("ShaderSystem::Initialize — рекомендуется иметь значение MaxShaderCount не ниже 512.");
        }
    }

    // Выясняем, какой размер хеш-таблицы необходим.
    // Блок памяти будет содержать структуру состояния, а затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(ShaderSystem);
    u64 HashtableRequirement = sizeof(u32) * config.MaxShaderCount;
    u64 ShaderArrayRequirement = sizeof(Shader) * config.MaxShaderCount;
    MemoryRequirement = StructRequirement + HashtableRequirement + ShaderArrayRequirement;

    if (!memory) {
        return nullptr;
    }

    if (!pShaderSystem) {
        // Настраиваем указатель состояния, блок памяти, массив шейдеров, затем создаем хеш-таблицу.
        u8* addres = (u8*)memory;
        pShaderSystem = new(memory) ShaderSystem(
            config,
            (addres + StructRequirement), 
            (Shader*)(addres + StructRequirement + HashtableRequirement));
    }

    if (!pShaderSystem){
        // MERROR("ShaderSystem::Initialize — Неудалось инициализировать систему шейдеров.");
        return nullptr;
    }

    // Следите за горячей перезагрузкой файлов в отладочных сборках.
#ifdef _DEBUG
    EventSystem::Register(EventSystem::WatchedFileWritten, pShaderSystem, FileWatchEvent);
#endif
    
    return pShaderSystem;
}

void ShaderSystem::Shutdown()
{
    // Уничтожьте все существующие шейдеры.
    for (u32 i = 0; i < MaxShaderCount; ++i) {
        auto& shader = shaders[i];
        if (shader.id != INVALID::ID) {
            shader.Destroy();
        }
    }
    lookup.Destroy();
    pShaderSystem = nullptr;
}

Shader* ShaderSystem::CreateShader(Renderpass& pass, ShaderConfig &config)
{
    u32 id = INVALID::ID;
    for (u32 i = 0; i < pShaderSystem->MaxShaderCount; ++i) {
        if (pShaderSystem->shaders[i].id == INVALID::ID) {
            id = i;
            break;
        }
    }
    
    if (id == INVALID::ID) {
        MERROR("Не удалось найти свободный слот для создания нового шейдера. Отмена.");
        return nullptr;
    }
    
    RenderingSystem* renderer = SystemsManager::GetRenderingSystem();
    Shader* NewShader = &pShaderSystem->shaders[id];
    new(NewShader) Shader(id, config);
    
    if (!renderer->ShaderCreate(NewShader, config, &pass)) {
        MERROR("Ошибка создания шейдера.");
        return nullptr;
    }

    // Готов к инициализации.
    NewShader->state = ShaderState::Uninitialized;

    // Атрибуты процесса ЗАДАЧА: переделать
    const u32 caSize = config.attributes.Length();
    for (u32 i = 0; i < caSize; ++i) {
        ShaderAttributeConfig& ac = config.attributes[i];
        if (!NewShader->AddAttribute(ac)) {
            MERROR("Не удалось добавить атрибут '%s' к шейдеру '%s'.", ac.name.c_str(), config.name.c_str());
            return nullptr;
        }
    }

    // Технологическая униформа i = 1
    const u32 cuSize = config.uniforms.Length();
    for (u32 i = 0; i < cuSize; ++i) {
        ShaderUniformConfig& uc = config.uniforms[i];
        if (UniformTypeIsSampler(uc.type)) {
            if (!NewShader->AddSampler(uc)) {
                MERROR("Не удалось добавить сэмплер '%s' в шейдер '%s'.", uc.name.c_str(), config.name.c_str());
                return nullptr;
            }
        } else {
            if (!NewShader->AddUniform(config.uniforms[i])) {
                MERROR("Не удалось добавить uniform '%s' к шейдеру '%s'.", uc.name.c_str(), config.name.c_str());
                return nullptr;
            }
        }
    }

    // Инициализируйте шейдер.
    if (!renderer->ShaderInitialize(NewShader)) {
        MERROR("ShaderSystem::Create: не удалось инициализировать шейдер '%s'.", NewShader->name.c_str());
        // ПРИМЕЧАНИЕ: Initialize автоматически уничтожает шейдер в случае сбоя.
        return nullptr;
    }

    // На этом этапе создание прошло успешно, поэтому сохраните идентификатор шейдера в хеш-таблице, чтобы позже можно было найти его по имени.
    if (!pShaderSystem->lookup.Set(NewShader->name.c_str(), NewShader->id)) {
        // Черт возьми, мы зашли так далеко... Что ж, удалите шейдер и перезапустите.
        renderer->ShaderDestroy(NewShader);
        return nullptr;
    }

    return NewShader;
}

u32 ShaderSystem::GetShaderID(const MString& ShaderName)
{
    u32 ShaderID = INVALID::ID;
    if (!pShaderSystem->lookup.Get(ShaderName.c_str(), &ShaderID)) {
        MERROR("Не зарегистрирован ни один шейдер с именем '%s'.", ShaderName.c_str());
        return INVALID::ID;
    }
    // MTRACE("Получен идентификатор %u для шейдера с именем '%s'.", ShaderID, ShaderName.c_str());
    return ShaderID;
}

Shader *ShaderSystem::GetShader(u32 ShaderID)
{
    if (ShaderID == INVALID::ID) {
        MERROR("ShaderSystem::GetShader был передан INVALID::ID. Будет возвращён nullptr.");
        return nullptr;
    }
    if (pShaderSystem->shaders[ShaderID].id == INVALID::ID) {
        MERROR("ShaderSystem::GetShader был передан недопустимый идентификатор (%u). Будет возвращён nullptr.", ShaderID);
        return nullptr;
    }
    if (ShaderID >= pShaderSystem->MaxShaderCount) {
        MERROR("ShaderSystem::GetShader был передан идентификатор (%u) вне диапазона (0-%u). Будет возвращён nullptr.", ShaderID, pShaderSystem->MaxShaderCount);
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
    // if (pShaderSystem->CurrentShaderID != ShaderID) {
        Shader* NextShader = GetShader(ShaderID);
        pShaderSystem->CurrentShaderID = ShaderID;

        if (!SystemsManager::GetRenderingSystem()->ShaderUse(NextShader)) {
            MERROR("Не удалось использовать шейдер '%s'.", NextShader->name.c_str());
            return false;
        }
        
        if (!NextShader->BindGlobals()) {
            MERROR("Не удалось привязать глобальные переменные для шейдера. '%s'.", NextShader->name.c_str());
            return false;
        }
    // }
    return true;
}

bool ShaderSystem::UniformSetArrayed(const char* UniformName, u32 ArrayIndex, const void* value)
{
    if (pShaderSystem->CurrentShaderID == INVALID::ID) {
        MERROR("ShaderSystem::UniformSet вызывается без использования шейдера.");
        return false;
    }
    Shader* shader = &pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    u16 index = shader->UniformLocation(UniformName);
    return UniformSetArrayed(index, ArrayIndex, value);
}

bool ShaderSystem::UniformSetArrayed(u16 location, u32 ArrayIndex, const void *value)
{
    Shader& shader = pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    ShaderUniform& uniform = shader.uniforms[location];
    auto renderer = SystemsManager::GetRenderingSystem();
    if (shader.BoundScope != uniform.scope) {
        if (uniform.scope == ShaderScope::Global) {
            shader.BindGlobals();
        } else if (uniform.scope == ShaderScope::Instance) {
            renderer->ShaderBindInstance(&shader);
        } else {
            // ПРИМЕЧАНИЕ: Больше здесь делать нечего, просто установите униформу.
        }
        shader.BoundScope = uniform.scope;
    }
    return renderer->ShaderSetUniform(&shader, &uniform, ArrayIndex, value);
}

bool ShaderSystem::ApplyGlobal(bool NeedsUpdate, const FrameData& rFrameData)
{
    return SystemsManager::GetRenderingSystem()->ShaderApplyGlobals(&pShaderSystem->shaders[pShaderSystem->CurrentShaderID], NeedsUpdate, rFrameData);
}

bool ShaderSystem::ApplyInstance(bool NeedsUpdate, const FrameData& rFrameData)
{
    return SystemsManager::GetRenderingSystem()->ShaderApplyInstance(&pShaderSystem->shaders[pShaderSystem->CurrentShaderID], NeedsUpdate, rFrameData);
}

bool ShaderSystem::ApplyLocal(const FrameData &rFrameData)
{
    auto* shader = &pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    return SystemsManager::GetRenderingSystem()->ShaderApplyLocal(shader, rFrameData);
}

bool ShaderSystem::BindInstance(u32 InstanceID)
{
    auto* shader = &pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    shader->BoundInstanceID = InstanceID;
    return SystemsManager::GetRenderingSystem()->ShaderBindInstance(shader, InstanceID);
}

bool ShaderSystem::BindLocal()
{
    auto* shader = &pShaderSystem->shaders[pShaderSystem->CurrentShaderID];
    return SystemsManager::GetRenderingSystem()->ShaderBindLocal(shader);
}
