#include "material_system.hpp"

#include "systems/texture_system.hpp"
#include "renderer/rendering_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/light_system.hpp"

#include "memory/linear_allocator.hpp"
#include <new>

struct MaterialReference {
    u64 ReferenceCount;
    u32 handle;
    bool AutoRelease;
    constexpr MaterialReference() : ReferenceCount(), handle(), AutoRelease(false) {}
    constexpr MaterialReference(u64 ReferenceCount, u32 handle, bool AutoRelease) 
    : ReferenceCount(ReferenceCount), handle(handle), AutoRelease(AutoRelease) {}
};

struct MaterialShaderUniformLocations {
    u16 projection      {INVALID::U16ID};
    u16 view            {INVALID::U16ID};
    u16 AmbientColour   {INVALID::U16ID};
    u16 ViewPosition    {INVALID::U16ID};
    u16 specular        {INVALID::U16ID};
    u16 DiffuseColour   {INVALID::U16ID};
    u16 DiffuseTexture  {INVALID::U16ID};
    u16 SpecularTexture {INVALID::U16ID};
    u16 NormalTexture   {INVALID::U16ID};
    u16 model           {INVALID::U16ID};
    u16 RenderMode      {INVALID::U16ID};
    u16 DirLight        {INVALID::U16ID};
    u16 PointLights     {INVALID::U16ID};
    u16 NumPointLights  {INVALID::U16ID};
};

struct UI_ShaderUniformLocations {
    u16 projection      {INVALID::U16ID};
    u16 view            {INVALID::U16ID};
    u16 DiffuseColour   {INVALID::U16ID};
    u16 DiffuseTexture  {INVALID::U16ID};
    u16 model           {INVALID::U16ID};
};

struct sMaterialSystem
{
    // Конфигурация материала---------------------------------------------------------------------------------
    u32 MaxMaterialCount;                                   // Максимальное количество загружаемых материалов.
    //--------------------------------------------------------------------------------------------------------
    Material DefaultMaterial;                               // Стандартный материал.
    Material* RegisteredMaterials;                          // Массив зарегистрированных материалов.

    HashTable<MaterialReference> RegisteredMaterialTable;   // Хэш-таблица для поиска материалов.

    MaterialShaderUniformLocations MaterialLocations;       // Известные местоположения шейдера материала.
    u32 MaterialShaderID;

    UI_ShaderUniformLocations UI_Locations;
    u32 UI_ShaderID;

    /// @brief Инициализирует систему материалов при создании объекта.
    constexpr sMaterialSystem() : MaxMaterialCount(), DefaultMaterial(), RegisteredMaterials(nullptr), RegisteredMaterialTable(), MaterialLocations(), MaterialShaderID(), UI_Locations(), UI_ShaderID() {}
    sMaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, MaterialReference* HashTableBlock);
    ~sMaterialSystem();
};

static sMaterialSystem* state = nullptr;

bool CreateDefaultMaterial();
bool LoadMaterial(const MaterialConfig& config, Material* m);
void DestroyMaterial(Material* m);

sMaterialSystem::sMaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, MaterialReference* HashTableBlock) 
: 
MaxMaterialCount(MaxMaterialCount),
DefaultMaterial(DEFAULT_MATERIAL_NAME, 
                Vector4D<f32>::One(), 
                TextureMap(TextureSystem::GetDefaultTexture(Texture::Diffuse), TextureUse::MapDiffuse), 
                TextureMap(TextureSystem::GetDefaultTexture(Texture::Specular), TextureUse::MapSpecular), 
                TextureMap(TextureSystem::GetDefaultTexture(Texture::Normal), TextureUse::MapNormal)), 
RegisteredMaterials(new(RegisteredMaterials) Material[MaxMaterialCount]()),
RegisteredMaterialTable(MaxMaterialCount, false, HashTableBlock, true, MaterialReference(0, INVALID::ID, true)), 
MaterialLocations(),
MaterialShaderID(INVALID::ID), 
UI_Locations(),
UI_ShaderID(INVALID::ID)
{}

sMaterialSystem::~sMaterialSystem()
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

bool MaterialSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<MaterialSystemConfig*>(config);
    if (pConfig->MaxMaterialCount == 0) {
        MFATAL("MaterialSystem::Initialize — MaxMaterialCount должен быть > 0.");
        return false;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(sMaterialSystem);
    u64 ArrayRequirement = sizeof(Material) * pConfig->MaxMaterialCount;
    u64 HashtableRequirement = sizeof(MaterialReference) * pConfig->MaxMaterialCount;
    MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    if (!memory) {
        return true;
    }

    if (!state) {
        u8* ptrMatSys = reinterpret_cast<u8*>(memory);
        Material* RegisteredMaterials = reinterpret_cast<Material*>(ptrMatSys + StructRequirement);
        MaterialReference* HashTableBlock = reinterpret_cast<MaterialReference*>(RegisteredMaterials + pConfig->MaxMaterialCount);
        state = new(ptrMatSys) sMaterialSystem(pConfig->MaxMaterialCount, RegisteredMaterials, HashTableBlock);
    }

    if (!CreateDefaultMaterial()) {
        MFATAL("Не удалось создать материал по умолчанию. Приложение не может быть продолжено.");
        return false;
    } 

    // Получите единые индексы.
    // Сохраните местоположения известных типов для быстрого поиска.
    auto s = ShaderSystem::GetShader("Shader.Builtin.Material"); 
    state->MaterialShaderID                  = s->id;
    state->MaterialLocations.projection      = ShaderSystem::UniformIndex(s,       "projection");
    state->MaterialLocations.view            = ShaderSystem::UniformIndex(s,             "view");
    state->MaterialLocations.AmbientColour   = ShaderSystem::UniformIndex(s,   "ambient_colour");
    state->MaterialLocations.ViewPosition    = ShaderSystem::UniformIndex(s,    "view_position");
    state->MaterialLocations.DiffuseColour   = ShaderSystem::UniformIndex(s,   "diffuse_colour");
    state->MaterialLocations.DiffuseTexture  = ShaderSystem::UniformIndex(s,  "diffuse_texture");
    state->MaterialLocations.SpecularTexture = ShaderSystem::UniformIndex(s, "specular_texture");
    state->MaterialLocations.NormalTexture   = ShaderSystem::UniformIndex(s,   "normal_texture");
    state->MaterialLocations.specular        = ShaderSystem::UniformIndex(s,         "specular");
    state->MaterialLocations.model           = ShaderSystem::UniformIndex(s,            "model");
    state->MaterialLocations.RenderMode      = ShaderSystem::UniformIndex(s,             "mode");
    state->MaterialLocations.DirLight        = ShaderSystem::UniformIndex(s,        "dir_light");
    state->MaterialLocations.PointLights     = ShaderSystem::UniformIndex(s,         "p_lights");
    state->MaterialLocations.NumPointLights  = ShaderSystem::UniformIndex(s,     "num_p_lights");

    s = ShaderSystem::GetShader("Shader.Builtin.UI");
    state->UI_ShaderID = s->id;
    state->UI_Locations.projection           = ShaderSystem::UniformIndex(s,      "projection");
    state->UI_Locations.view                 = ShaderSystem::UniformIndex(s,            "view");
    state->UI_Locations.DiffuseColour        = ShaderSystem::UniformIndex(s,  "diffuse_colour");
    state->UI_Locations.DiffuseTexture       = ShaderSystem::UniformIndex(s, "diffuse_texture");
    state->UI_Locations.model                = ShaderSystem::UniformIndex(s,           "model");
    

    return true;
}

void MaterialSystem::Shutdown()
{
    if (state) {
        state->~sMaterialSystem(); // delete state;
        state = nullptr;
    }
}

Material *MaterialSystem::Acquire(const char *name)
{
    // Загрузить конфигурацию материала из ресурса.
    MaterialResource MaterialResource;
    if (!ResourceSystem::Load(name, eResource::Type::Material, nullptr, MaterialResource)) {
        MERROR("Не удалось загрузить ресурс материала, возвращается значение nullptr.");
        return nullptr;
    }
    
    Material* m = nullptr;
    if (MaterialResource.data) {
        m = Acquire(MaterialResource.data);
    }

    // Clean up
    ResourceSystem::Unload(MaterialResource);

    if (!m) {
        MERROR("Не удалось загрузить ресурс материала, возвращается значение nullptr.");
    }

    return m;
}

Material *MaterialSystem::Acquire(const MaterialConfig &config)
{
    // Вернуть материал по умолчанию.
    if (MString::Equali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &state->DefaultMaterial;
    }

    MaterialReference ref;
    if (state->RegisteredMaterialTable.Get(config.name, &ref)) {
        // Это можно изменить только при первой загрузке материала.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = config.AutoRelease;
        } 
        ref.ReferenceCount++;
        if (ref.handle == INVALID::ID) {
            // Это означает, что здесь нет материала. Сначала найдите бесплатный индекс.
            Material* m = nullptr;
            for (u32 i = 0; i < state->MaxMaterialCount; ++i) {
                if (state->RegisteredMaterials[i].id == INVALID::ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    m = &state->RegisteredMaterials[i];
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
        state->RegisteredMaterialTable.Set(config.name, ref);
        return &state->RegisteredMaterials[ref.handle];
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
    if (state->RegisteredMaterialTable.Get(name, &ref)) {
        if (ref.ReferenceCount == 0) {
            MWARN("Пытался выпустить несуществующий материал: '%s'", name);
            return;
        }

        // Сделать копию имени, так как при уничтожении оно будет стерто 
        // (поскольку переданное имя обычно является указателем на фактическое имя материала).
        char NameCopy[MATERIAL_NAME_MAX_LENGTH];
        MString::Copy(NameCopy, name, MATERIAL_NAME_MAX_LENGTH);

        ref.ReferenceCount--;
        if (ref.ReferenceCount == 0 && ref.AutoRelease) {
            auto m = &state->RegisteredMaterials[ref.handle];

            // Уничтожить/сбросить материал.
            DestroyMaterial(m);

            // Сбросьте ссылку.
            ref.handle = INVALID::ID;
            ref.AutoRelease = false;
            // MTRACE("Выпущенный материал '%s'., Материал выгружен, поскольку количество ссылок = 0 и AutoRelease = true.", NameCopy);
        } else {
            // MTRACE("Выпущенный материал '%s', теперь имеет счетчик ссылок '%i' (AutoRelease=%s).", NameCopy, ref.ReferenceCount, ref.AutoRelease ? "true" : "false");
        }

        // Обновите запись.
        state->RegisteredMaterialTable.Set(NameCopy, ref);
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
    auto shader = ShaderSystem::GetShader(ShaderID);
    if (!shader) {
        return false;
    }

    if (shader->RenderFrameNumber == RenderFrameNumber) {
        return true;
    }
    
    if (ShaderID == state->MaterialShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.view, &view));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.AmbientColour, &AmbientColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.ViewPosition, &ViewPosition));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.RenderMode, &RenderMode));
    } else if (ShaderID == state->UI_ShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UI_Locations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UI_Locations.view, &view));
    } else {
        MERROR("MaterialSystem::ApplyGlobal(): Неизвестный идентификатор шейдера '%d' ", ShaderID);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyGlobal());
    // Синхронизация номера кадра
    shader->RenderFrameNumber = RenderFrameNumber;
    return true;
}

bool MaterialSystem::ApplyInstance(Material *material, bool NeedsUpdate)
{
    // Примените униформу на уровне экземпляра.
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::BindInstance(material->InternalId));
    if(NeedsUpdate) {
        if (material->ShaderID == state->MaterialShaderID) {
            // Шейдер материала
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DiffuseColour, &material->DiffuseColour));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DiffuseTexture, &material->DiffuseMap));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.SpecularTexture, &material->SpecularMap));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.NormalTexture, &material->NormalMap));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.specular, &material->specular));

            // Направленный свет
            auto DirLight = LightSystem::GetDirectionalLight();
            if (DirLight) {
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DirLight, &DirLight->data));
            } else {
                DirectionalLight::Data data{};
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DirLight, &data));
            }
            
            // Точечный свет
            u32 PointLightCount = LightSystem::PointLightCount();
            if (PointLightCount) {
                // ЗАДАЧА: frame allocator?
                u64 PointLightsSize = sizeof(PointLight) * PointLightCount;
                auto PointLights = reinterpret_cast<PointLight*>(MemorySystem::Allocate(PointLightsSize, Memory::Array, true));
                LightSystem::GetPointLights(PointLights);

                u64 PointLightDatasSize = sizeof(PointLight::Data) * PointLightCount;
                auto PointLightDatas = reinterpret_cast<PointLight::Data*>(MemorySystem::Allocate(PointLightDatasSize, Memory::Array, true));
                for (u32 i = 0; i < PointLightCount; ++i) {
                    PointLightDatas[i] = PointLights[i].data;
                }

                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.PointLights, PointLightDatas));

                MemorySystem::Free(PointLightDatas, PointLightDatasSize, Memory::Array);
                MemorySystem::Free(PointLights, PointLightsSize, Memory::Array);
            }

            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.NumPointLights, &PointLightCount)); 
        } else if (material->ShaderID == state->UI_ShaderID) {
            // шейдер пользовательского интерфейса
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UI_Locations.DiffuseColour, &material->DiffuseColour));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UI_Locations.DiffuseTexture, &material->DiffuseMap));
        } else {
            MERROR("MaterialSystem::ApplyInstance(): Нераспознанный идентификатор шейдера «%d» в шейдере «%s».", material->ShaderID, material->name);
            return false;
        }
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyInstance(NeedsUpdate));

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
    if (material->ShaderID == state->MaterialShaderID) {
        return ShaderSystem::UniformSet(state->MaterialLocations.model, &model);
    } else if (material->ShaderID == state->UI_ShaderID) {
        return ShaderSystem::UniformSet(state->UI_Locations.model, &model);
    }

    MERROR("Неизвестный идентификатор шейдера «%d»", material->ShaderID);
    return false;
}

void MaterialSystem::Dump()
{
    auto refs = state->RegisteredMaterialTable.memory;
    for (u32 i = 0; i < state->RegisteredMaterialTable.ElementCount; ++i) {
        auto& r = refs[i];
        if (r.ReferenceCount > 0 || r.handle != INVALID::ID) {
            MDEBUG("Найденный материал ref (handle/refCount): (%u/%u)", r.handle, r.ReferenceCount);
            if (r.handle != INVALID::ID) {
                MTRACE("Название материала: %s", state->RegisteredMaterials[r.handle].name);
            }
        }
    }
}

bool CreateDefaultMaterial()
{ 
    TextureMap* maps[3] = {&state->DefaultMaterial.DiffuseMap, &state->DefaultMaterial.SpecularMap, &state->DefaultMaterial.NormalMap};
    Shader* s = ShaderSystem::GetShader("Shader.Builtin.Material");
    if (!RenderingSystem::ShaderAcquireInstanceResources(s, maps, state->DefaultMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы средства рендеринга для материала по умолчанию. Приложение не может быть продолжено.");
        return false;
    }
    // Обязательно укажите идентификатор шейдера.
    state->DefaultMaterial.ShaderID = s->id;

    return true;
}

bool LoadMaterial(const MaterialConfig &config, Material *m)
{ 
    m->Reset();

    // имя
    MString::Copy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    m->ShaderID = ShaderSystem::GetID(config.ShaderName);

    // Рассеянный цвет
    m->DiffuseColour = config.DiffuseColour;
    m->specular = config.specular;

    // Diffuse map
    // ЗАДАЧА: настроить
    m->DiffuseMap.FilterMinify = m->DiffuseMap.FilterMagnify = TextureFilter::ModeLinear;
    m->DiffuseMap.RepeatU = m->DiffuseMap.RepeatV = m->DiffuseMap.RepeatW = TextureRepeat::Repeat;

    if (MString::Length(config.DiffuseMapName) > 0) {
        m->DiffuseMap.use = TextureUse::MapDiffuse;
        m->DiffuseMap.texture = TextureSystem::Acquire(config.DiffuseMapName, true);
        if (!m->DiffuseMap.texture) {
            MWARN("Невозможно загрузить текстуру '%s' для материала '%s', используется значение по умолчанию.", config.DiffuseMapName, m->name);
            m->DiffuseMap.texture = TextureSystem::GetDefaultTexture(Texture::Diffuse);
        }
    } else {
        // ПРИМЕЧАНИЕ. Устанавливается только для ясности, поскольку вызов MMemory::ZeroMem выше уже делает это.
        m->DiffuseMap.use = TextureUse::MapDiffuse;
        m->DiffuseMap.texture = TextureSystem::GetDefaultTexture(Texture::Diffuse);
    }

    if (!RenderingSystem::TextureMapAcquireResources(&m->DiffuseMap)) {
        MERROR("Невозможно получить ресурсы для карты диффузных текстур.");
        return false;
    }

    // Карта блеска
    m->SpecularMap.FilterMinify = m->SpecularMap.FilterMagnify = TextureFilter::ModeLinear;
    m->SpecularMap.RepeatU = m->SpecularMap.RepeatV = m->SpecularMap.RepeatW = TextureRepeat::Repeat;
    
    if (MString::Length(config.SpecularMapName) > 0) {
        m->SpecularMap.use = TextureUse::MapSpecular;
        m->SpecularMap.texture = TextureSystem::Acquire(config.SpecularMapName, true);
        if (!m->SpecularMap.texture) {
            MWARN("Невозможно загрузить текстуру «%s» для материала «%s», используется значение по умолчанию.", config.SpecularMapName, m->name);
            m->SpecularMap.texture = TextureSystem::GetDefaultTexture(Texture::Specular);
        }
    } else {
        // ПРИМЕЧАНИЕ: Устанавливается только для ясности, поскольку вызов MMemory::Zero() выше уже делает это.
        m->SpecularMap.use = TextureUse::MapSpecular;
        m->SpecularMap.texture = TextureSystem::GetDefaultTexture(Texture::Specular);
    }

    if (!RenderingSystem::TextureMapAcquireResources(&m->SpecularMap)) {
        MERROR("Невозможно получить ресурсы для карты зеркальных текстур.");
        return false;
    }

    // Карта нормалей
    m->NormalMap.FilterMinify = m->NormalMap.FilterMagnify = TextureFilter::ModeLinear;
    m->NormalMap.RepeatU = m->NormalMap.RepeatV = m->NormalMap.RepeatW = TextureRepeat::Repeat;
    
    if (MString::Length(config.NormalMapName) > 0) {
        m->NormalMap.use = TextureUse::MapNormal;
        m->NormalMap.texture = TextureSystem::Acquire(config.NormalMapName, true);
        if (!m->NormalMap.texture) {
            MWARN("Невозможно загрузить текстуру нормалей «%s» для материала «%s», используется значение по умолчанию.", config.NormalMapName, m->name);
            m->SpecularMap.texture = TextureSystem::GetDefaultTexture(Texture::Normal);
        }
    } else {
        // Использование по умолчанию.
        m->NormalMap.use = TextureUse::MapNormal;
        m->NormalMap.texture = TextureSystem::GetDefaultTexture(Texture::Normal);
    }

    if (!RenderingSystem::TextureMapAcquireResources(&m->NormalMap)) {
        MERROR("Невозможно получить ресурсы для карты нормалей текстур.");
        return false;
    }
    
    // ЗАДАЧА: другие карты

    // Отправьте его рендереру для получения ресурсов.
    Shader* s = ShaderSystem::GetShader(config.ShaderName);
    if (!s) {
        MERROR("Невозможно загрузить материал, поскольку его шейдер не найден: «%s». Вероятно, это проблема с ассетом материала.", config.ShaderName.c_str());
        return false;
    }
    // Соберите список указателей на карты текстур.
    TextureMap* maps[3] = {&m->DiffuseMap, &m->SpecularMap, &m->NormalMap};
    if(!RenderingSystem::ShaderAcquireInstanceResources(s, maps, m->InternalId)) {
        MERROR("Не удалось получить ресурсы средства визуализации для материала '%s'.", m->name);
        return false;
    }

    return true;
}

void DestroyMaterial(Material *m)
{
    // Выпустите ссылки на текстуры.
    if (m->DiffuseMap.texture) {
        TextureSystem::Release(m->DiffuseMap.texture->name);
    }

    if (m->SpecularMap.texture) {
        TextureSystem::Release(m->SpecularMap.texture->name);
    }

    if (m->NormalMap.texture) {
        TextureSystem::Release(m->NormalMap.texture->name);
    }

    // Освободите ресурсы карт текстур.
    RenderingSystem::TextureMapReleaseResources(&m->DiffuseMap);
    RenderingSystem::TextureMapReleaseResources(&m->SpecularMap);
    RenderingSystem::TextureMapReleaseResources(&m->NormalMap);

    // Освободите ресурсы средства рендеринга.
    if (m->ShaderID != INVALID::ID && m->InternalId != INVALID::ID) {
        RenderingSystem::ShaderReleaseInstanceResources(ShaderSystem::GetShader(m->ShaderID), m->InternalId);
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
