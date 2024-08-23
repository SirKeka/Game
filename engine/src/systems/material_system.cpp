#include "material_system.hpp"
#include "systems/texture_system.hpp"
#include "renderer/renderer.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"

#include "memory/linear_allocator.hpp"
#include <new>

MaterialSystem* MaterialSystem::state = nullptr;

MaterialSystem::MaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, MaterialReference* HashTableBlock) 
: 
MaxMaterialCount(MaxMaterialCount),
DefaultMaterial(DEFAULT_MATERIAL_NAME, 
                Vector4D<f32>::One(), 
                TextureMap(TextureSystem::Instance()->GetDefaultTexture(ETexture::Diffuse), TextureUse::MapDiffuse), 
                TextureMap(TextureSystem::Instance()->GetDefaultTexture(ETexture::Specular), TextureUse::MapSpecular), 
                TextureMap(TextureSystem::Instance()->GetDefaultTexture(ETexture::Normal), TextureUse::MapNormal)), 
RegisteredMaterials(new(RegisteredMaterials) Material[MaxMaterialCount]()),
RegisteredMaterialTable(MaxMaterialCount, false, HashTableBlock, true, MaterialReference(0, INVALID::ID, false)), 
MaterialShaderID(INVALID::ID), 
UI_ShaderID(INVALID::ID)
{}

MaterialSystem::~MaterialSystem()
{
    // Сделать недействительными все материалы в массиве.
    for (u32 i = 0; i < MaxMaterialCount; ++i) { 
        if (state->RegisteredMaterials[i].id != INVALID::ID) {
            MTRACE("id%u, generation%u, InternalId%u, %u", RegisteredMaterials[i].id, RegisteredMaterials[i].generation, RegisteredMaterials[i].InternalId, i);
            DestroyMaterial(&state->RegisteredMaterials[i]);
        }
    }

    // Уничтожьте материал по умолчанию.
    DestroyMaterial(&state->DefaultMaterial);
}

bool MaterialSystem::Initialize(u32 MaxMaterialCount)
{
    if (MaxMaterialCount == 0) {
        MFATAL("MaterialSystem::Initialize — MaxMaterialCount должен быть > 0.");
        return false;
    }

    if (!state) {
        // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
        u64 StructRequirement = sizeof(MaterialSystem);
        u64 ArrayRequirement = sizeof(Material) * MaxMaterialCount;
        u64 HashtableRequirement = sizeof(MaterialReference) * MaxMaterialCount;
        u64 MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;
        u8* ptrMatSys = reinterpret_cast<u8*>(LinearAllocator::Instance().Allocate(MemoryRequirement));
        Material* RegisteredMaterials = reinterpret_cast<Material*>(ptrMatSys + sizeof(MaterialSystem));
        MaterialReference* HashTableBlock = reinterpret_cast<MaterialReference*>(RegisteredMaterials + MaxMaterialCount);
        state = new(ptrMatSys) MaterialSystem(MaxMaterialCount, RegisteredMaterials, HashTableBlock);
    }

    if (!state->CreateDefaultMaterial()) {
        MFATAL("Не удалось создать материал по умолчанию. Приложение не может быть продолжено.");
        return false;
    } 
    return true;
}

void MaterialSystem::Shutdown()
{
    if (state) {
        state->~MaterialSystem(); // delete state;
        state = nullptr;
    }
}

Material *MaterialSystem::Acquire(const char *name)
{
    auto ResourceSystemInst = ResourceSystem::Instance();
    // Загрузить конфигурацию материала из ресурса.
    Resource MaterialResource;
    if (!ResourceSystemInst->Load(name, ResourceType::Material, nullptr, MaterialResource)) {
        MERROR("Не удалось загрузить ресурс материала, возвращается значение nullptr.");
        return nullptr;
    }
    
    Material* m;
    if (MaterialResource.data) {
        m = Acquire(*reinterpret_cast<MaterialConfig*>(MaterialResource.data));
    }

    // Clean up
    ResourceSystemInst->Unload(MaterialResource);

    if (!m) {
        MERROR("Не удалось загрузить ресурс материала, возвращается значение nullptr.");
    }

    return m;
}

Material *MaterialSystem::Acquire(const MaterialConfig &config)
{
    // Вернуть материал по умолчанию.
    if (MString::Equali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &DefaultMaterial;
    }

    MaterialReference ref;
    if (RegisteredMaterialTable.Get(config.name, &ref)) {
        // Это можно изменить только при первой загрузке материала.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = config.AutoRelease;
        } 
        ref.ReferenceCount++;
        if (ref.handle == INVALID::ID) {
            // Это означает, что здесь нет материала. Сначала найдите бесплатный индекс.
            Material* m = nullptr;
            for (u32 i = 0; i < MaxMaterialCount; ++i) {
                if (RegisteredMaterials[i].id == INVALID::ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    m = &RegisteredMaterials[i];
                    break;
                }
            }
            
            // Убедитесь, что пустой слот действительно найден.
            if (!m || ref.handle == INVALID::ID) {
                MFATAL("MaterialSystem::Acquire — система материалов больше не может содержать материалы. Настройте конфигурацию, чтобы разрешить больше.");
                return nullptr;
            }

            // Создайте новый материал.
            if (!LoadMaterial(config, m)) {
                MERROR("Не удалось загрузить материал '%s'.", config.name);
                return nullptr;
            }

            auto ShaderSystemInst = ShaderSystem::GetInstance();
            // Получите единые индексы.
            Shader* s = ShaderSystemInst->GetShader(m->ShaderID);
            // Сохраните местоположения известных типов для быстрого поиска.
            if (MaterialShaderID == INVALID::ID && config.ShaderName == BUILTIN_SHADER_NAME_MATERIAL) {
                MaterialShaderID                  = s->id;
                MaterialLocations.projection      = ShaderSystemInst->UniformIndex(s,       "projection");
                MaterialLocations.view            = ShaderSystemInst->UniformIndex(s,             "view");
                MaterialLocations.AmbientColour   = ShaderSystemInst->UniformIndex(s,   "ambient_colour");
                MaterialLocations.ViewPosition    = ShaderSystemInst->UniformIndex(s,    "view_position");
                MaterialLocations.DiffuseColour   = ShaderSystemInst->UniformIndex(s,   "diffuse_colour");
                MaterialLocations.DiffuseTexture  = ShaderSystemInst->UniformIndex(s,  "diffuse_texture");
                MaterialLocations.SpecularTexture = ShaderSystemInst->UniformIndex(s, "specular_texture");
                MaterialLocations.NormalTexture   = ShaderSystemInst->UniformIndex(s,   "normal_texture");
                MaterialLocations.specular        = ShaderSystemInst->UniformIndex(s,         "specular");
                MaterialLocations.model           = ShaderSystemInst->UniformIndex(s,            "model");
                MaterialLocations.RenderMode      = ShaderSystemInst->UniformIndex(s,             "mode");
            } else if (UI_ShaderID == INVALID::ID && config.ShaderName == BUILTIN_SHADER_NAME_UI) {
                UI_ShaderID = s->id;
                UI_Locations.projection           = ShaderSystemInst->UniformIndex(s,      "projection");
                UI_Locations.view                 = ShaderSystemInst->UniformIndex(s,            "view");
                UI_Locations.DiffuseColour        = ShaderSystemInst->UniformIndex(s,  "diffuse_colour");
                UI_Locations.DiffuseTexture       = ShaderSystemInst->UniformIndex(s, "diffuse_texture");
                UI_Locations.model                = ShaderSystemInst->UniformIndex(s,           "model");
            }

            if (m->generation == INVALID::ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            // Также используйте дескриптор в качестве идентификатора материала.
            m->id = ref.handle;
            // MTRACE("Материал '%s' еще не существует. Создано, и теперь RefCount %i.", config.name, ref.ReferenceCount);
        } else {
            // MTRACE("Материал '%s' уже существует, RefCount увеличен до %i.", config.name, ref.ReferenceCount);
        }

        // Обновите запись.
        RegisteredMaterialTable.Set(config.name, ref);
        return &this->RegisteredMaterials[ref.handle];
    }

    // ПРИМЕЧАНИЕ: Это произойдет только в том случае, если что-то пойдет не так с состоянием.
    MERROR("MaterialSystem::Acquire не удалось получить материал '%s'. Нулевой указатель будет возвращен.", config.name);
    return nullptr;
}

void MaterialSystem::Release(const char *name)
{
    // Игнорируйте запросы на выпуск материала по умолчанию.
    if (MString::Equali(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }
    MaterialReference ref;
    if (RegisteredMaterialTable.Get(name, &ref)) {
        if (ref.ReferenceCount == 0) {
            MWARN("Пытался выпустить несуществующий материал: '%s'", name);
            return;
        }
        ref.ReferenceCount--;
        if (ref.ReferenceCount == 0 && ref.AutoRelease) {
            Material* m = &this->RegisteredMaterials[ref.handle];

            // Уничтожить/сбросить материал.
            DestroyMaterial(m);

            // Сбросьте ссылку.
            ref.handle = INVALID::ID;
            ref.AutoRelease = false;
            // MTRACE("Выпущенный материал '%s'., Материал выгружен, поскольку количество ссылок = 0 и AutoRelease = true.", name);
        } else {
            // MTRACE("Выпущенный материал '%s', теперь имеет счетчик ссылок '%i' (AutoRelease=%s).", name, ref.ReferenceCount, ref.AutoRelease ? "true" : "false");
        }

        // Обновите запись.
        RegisteredMaterialTable.Set(name, ref);
    } else {
        MERROR("MaterialSystem::Release не удалось выпустить материал '%s'.", name);
    }
    
}

#define MATERIAL_APPLY_OR_FAIL(expr)                        \
    if (!expr) {                                            \
        MERROR("Не удалось применить материал: %s", expr);  \
        return false;                                       \
    }

bool MaterialSystem::ApplyGlobal(u32 ShaderID, u64 RenderFrameNumber, const Matrix4D &projection, const Matrix4D &view, const FVec4& AmbientColour, const FVec4& ViewPosition, u32 RenderMode)
{
    auto ShaderSystemInst = ShaderSystem::GetInstance();
    Shader* shader = ShaderSystemInst->GetShader(ShaderID);
    if (!shader) {
        return false;
    }

    if (shader->RenderFrameNumber == RenderFrameNumber) {
        return true;
    }
    
    if (ShaderID == state->MaterialShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.view, &view));
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.AmbientColour, &AmbientColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.ViewPosition, &ViewPosition));
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.RenderMode, &RenderMode));
    } else if (ShaderID == state->UI_ShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->UI_Locations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->UI_Locations.view, &view));
    } else {
        MERROR("MaterialSystem::ApplyGlobal(): Неизвестный идентификатор шейдера '%d' ", ShaderID);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->ApplyGlobal());
    // Синхронизация номера кадра
    shader->RenderFrameNumber = RenderFrameNumber;
    return true;
}

bool MaterialSystem::ApplyInstance(Material *material, bool NeedsUpdate)
{
    auto ShaderSystemInst = ShaderSystem::GetInstance();
    // Примените униформу на уровне экземпляра.
    MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->BindInstance(material->InternalId));
    if(NeedsUpdate) {
        if (material->ShaderID == state->MaterialShaderID) {
            // Шейдер материала
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.DiffuseColour, &material->DiffuseColour));
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.DiffuseTexture, &material->DiffuseMap));
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.SpecularTexture, &material->SpecularMap));
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.specular, &material->NormalMap)); 
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->MaterialLocations.NormalTexture, &material->specular));
        } else if (material->ShaderID == state->UI_ShaderID) {
            // шейдер пользовательского интерфейса
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->UI_Locations.DiffuseColour, &material->DiffuseColour));
            MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->UniformSet(state->UI_Locations.DiffuseTexture, &material->DiffuseMap));
        } else {
            MERROR("MaterialSystem::ApplyInstance(): Нераспознанный идентификатор шейдера «%d» в шейдере «%s».", material->ShaderID, material->name);
            return false;
        }
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystemInst->ApplyInstance(NeedsUpdate));

    return true;
}

Material *MaterialSystem::GetDefaultMaterial()
{
     if (state) {
        return &state->DefaultMaterial;
    }

    MFATAL("MaterialSystem::GetDefaultMaterial вызывается перед инициализацией системы.");
    return nullptr;
}

bool MaterialSystem::ApplyLocal(Material *material, const Matrix4D &model)
{
    auto ShaderSystemInst = ShaderSystem::GetInstance();
    if (material->ShaderID == state->MaterialShaderID) {
        return ShaderSystemInst->UniformSet(state->MaterialLocations.model, &model);
    } else if (material->ShaderID == state->UI_ShaderID) {
        return ShaderSystemInst->UniformSet(state->UI_Locations.model, &model);
    }

    MERROR("Неизвестный идентификатор шейдера «%d»", material->ShaderID);
    return false;
}

bool MaterialSystem::CreateDefaultMaterial()
{ 
    TextureMap* maps[3] = {&DefaultMaterial.DiffuseMap, &DefaultMaterial.SpecularMap, &DefaultMaterial.NormalMap};
    Shader* s = ShaderSystem::GetInstance()->GetShader(BUILTIN_SHADER_NAME_MATERIAL);
    if (!Renderer::ShaderAcquireInstanceResources(s, maps, DefaultMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы средства рендеринга для материала по умолчанию. Приложение не может быть продолжено.");
        return false;
    }
    // Обязательно укажите идентификатор шейдера.
    state->DefaultMaterial.ShaderID = s->id;

    return true;
}

bool MaterialSystem::LoadMaterial(const MaterialConfig &config, Material *m)
{ 
    auto ShaderSystemInst = ShaderSystem::GetInstance();

    m->Reset();

    // имя
    MString::nCopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    m->ShaderID = ShaderSystemInst->GetID(config.ShaderName);

    // Рассеянный цвет
    m->DiffuseColour = config.DiffuseColour;
    m->specular = config.specular;

    // Diffuse map
    // ЗАДАЧА: настроить
    m->DiffuseMap.FilterMinify = m->DiffuseMap.FilterMagnify = TextureFilter::ModeLinear;
    m->DiffuseMap.RepeatU = m->DiffuseMap.RepeatV = m->DiffuseMap.RepeatW = TextureRepeat::Repeat;
    if (!Renderer::TextureMapAcquireResources(&m->DiffuseMap)) {
        MERROR("Невозможно получить ресурсы для карты диффузных текстур.");
        return false;
    }

    auto TextureSystemInst = TextureSystem::Instance();
    if (MString::Length(config.DiffuseMapName) > 0) {
        m->DiffuseMap.use = TextureUse::MapDiffuse;
        m->DiffuseMap.texture = TextureSystemInst->Acquire(config.DiffuseMapName, true);
        if (!m->DiffuseMap.texture) {
            MWARN("Невозможно загрузить текстуру '%s' для материала '%s', используется значение по умолчанию.", config.DiffuseMapName, m->name);
            m->DiffuseMap.texture = TextureSystemInst->GetDefaultTexture(ETexture::Diffuse);
        }
    } else {
        // ПРИМЕЧАНИЕ. Устанавливается только для ясности, поскольку вызов MMemory::ZeroMem выше уже делает это.
        m->DiffuseMap.use = TextureUse::MapDiffuse;
        m->DiffuseMap.texture = TextureSystemInst->GetDefaultTexture(ETexture::Diffuse);
    }

    // Карта блеска
    m->SpecularMap.FilterMinify = m->SpecularMap.FilterMagnify = TextureFilter::ModeLinear;
    m->SpecularMap.RepeatU = m->SpecularMap.RepeatV = m->SpecularMap.RepeatW = TextureRepeat::Repeat;
    if (!Renderer::TextureMapAcquireResources(&m->SpecularMap)) {
        MERROR("Невозможно получить ресурсы для карты зеркальных текстур.");
        return false;
    }
    if (MString::Length(config.SpecularMapName) > 0) {
        m->SpecularMap.use = TextureUse::MapSpecular;
        m->SpecularMap.texture = TextureSystemInst->Acquire(config.SpecularMapName, true);
        if (!m->SpecularMap.texture) {
            MWARN("Невозможно загрузить текстуру «%s» для материала «%s», используется значение по умолчанию.", config.SpecularMapName, m->name);
            m->SpecularMap.texture = TextureSystemInst->GetDefaultTexture(ETexture::Specular);
        }
    } else {
        // ПРИМЕЧАНИЕ: Устанавливается только для ясности, поскольку вызов MMemory::Zero() выше уже делает это.
        m->SpecularMap.use = TextureUse::MapSpecular;
        m->SpecularMap.texture = TextureSystemInst->GetDefaultTexture(ETexture::Specular);
    }

    // Карта нормалей
    m->NormalMap.FilterMinify = m->NormalMap.FilterMagnify = TextureFilter::ModeLinear;
    m->NormalMap.RepeatU = m->NormalMap.RepeatV = m->NormalMap.RepeatW = TextureRepeat::Repeat;
    if (!Renderer::TextureMapAcquireResources(&m->NormalMap)) {
        MERROR("Невозможно получить ресурсы для карты нормалей текстур.");
        return false;
    }
    if (MString::Length(config.NormalMapName) > 0) {
        m->NormalMap.use = TextureUse::MapNormal;
        m->NormalMap.texture = TextureSystemInst->Acquire(config.NormalMapName, true);
        if (!m->NormalMap.texture) {
            MWARN("Невозможно загрузить текстуру нормалей «%s» для материала «%s», используется значение по умолчанию.", config.NormalMapName, m->name);
            m->SpecularMap.texture = TextureSystemInst->GetDefaultTexture(ETexture::Normal);
        }
    } else {
        // Использование по умолчанию.
        m->NormalMap.use = TextureUse::MapNormal;
        m->NormalMap.texture = TextureSystemInst->GetDefaultTexture(ETexture::Normal);
    }
    
    // ЗАДАЧА: другие карты

    // Отправьте его рендереру для получения ресурсов.
    Shader* s = ShaderSystemInst->GetShader(config.ShaderName);
    if (!s) {
        MERROR("Невозможно загрузить материал, поскольку его шейдер не найден: «%s». Вероятно, это проблема с ассетом материала.", config.ShaderName.c_str());
        return false;
    }
    // Соберите список указателей на карты текстур.
    TextureMap* maps[3] = {&m->DiffuseMap, &m->SpecularMap, &m->NormalMap};
    if(!Renderer::ShaderAcquireInstanceResources(s, maps, m->InternalId)) {
        MERROR("Не удалось получить ресурсы средства визуализации для материала '%s'.", m->name);
        return false;
    }

    return true;
}

void MaterialSystem::DestroyMaterial(Material *m)
{
    auto TextureSystemInst = TextureSystem::Instance();
    // Выпустите ссылки на текстуры.
    if (m->DiffuseMap.texture) {
        TextureSystemInst->Release(m->DiffuseMap.texture->name);
    }

    if (m->SpecularMap.texture) {
        TextureSystemInst->Release(m->SpecularMap.texture->name);
    }

    if (m->NormalMap.texture) {
        TextureSystemInst->Release(m->NormalMap.texture->name);
    }

    // Освободите ресурсы карт текстур.
    Renderer::TextureMapReleaseResources(&m->DiffuseMap);
    Renderer::TextureMapReleaseResources(&m->SpecularMap);
    Renderer::TextureMapReleaseResources(&m->NormalMap);

    // Освободите ресурсы средства рендеринга.
    if (m->ShaderID != INVALID::ID && m->InternalId != INVALID::ID) {
        Renderer::ShaderReleaseInstanceResources(ShaderSystem::GetInstance()->GetShader(m->ShaderID), m->InternalId);
        m->ShaderID = INVALID::ID;
    }

    // Обнулить это, сделать удостоверения недействительными.
    m->Destroy();
    /*MMemory::ZeroMem(m, sizeof(Material));
    m->id = INVALID::U32ID;
    m->generation = INVALID::U32ID;
    m->InternalId = INVALID::U32ID;
    m = nullptr;*/
}
/*
void *MaterialSystem::operator new(u64 size)
{
    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 ArrayRequirement = sizeof(Material) * MaxMaterialCount;
    u64 HashtableRequirement = sizeof(MaterialReference) * MaxMaterialCount;
    return LinearAllocator::Instance().Allocate(size + ArrayRequirement + HashtableRequirement);
}
*/