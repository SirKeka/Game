#include "material_system.h"

#include "systems/texture_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/light_system.h"
#include "renderer/rendering_system.h"

#include "memory/linear_allocator.h"
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
    u16 properties      {INVALID::U16ID};
    u16 DiffuseTexture  {INVALID::U16ID};
    u16 SpecularTexture {INVALID::U16ID};
    u16 NormalTexture   {INVALID::U16ID};
    u16 model           {INVALID::U16ID};
    u16 RenderMode      {INVALID::U16ID};
    u16 DirLight        {INVALID::U16ID};
    u16 PointLights     {INVALID::U16ID};
    u16 NumPointLights  {INVALID::U16ID};
};

struct UiShaderUniformLocations {
    u16 projection      {INVALID::U16ID};
    u16 view            {INVALID::U16ID};
    u16 properties      {INVALID::U16ID};
    u16 DiffuseTexture  {INVALID::U16ID};
    u16 model           {INVALID::U16ID};
};

struct TerrainShaderLocations {
    bool loaded                  {false};
    u16 projection      {INVALID::U16ID};
    u16 view            {INVALID::U16ID};
    u16 AmbientColour   {INVALID::U16ID};
    u16 ViewPosition    {INVALID::U16ID};
    u16 model           {INVALID::U16ID};
    u16 RenderMode      {INVALID::U16ID};
    u16 DirLight        {INVALID::U16ID};
    u16 PointLights     {INVALID::U16ID};
    u16 NumPointLights  {INVALID::U16ID};

    u16 properties      {INVALID::U16ID};
    u16 samplers[TERRAIN_MAX_MATERIAL_COUNT * 3]{INVALID::U16ID, INVALID::U16ID, INVALID::U16ID};  // diffuse, spec, normal.
};

struct sMaterialSystem
{
    // Конфигурация материала---------------------------------------------------------------------------------
    u32 MaxMaterialCount;                                   // Максимальное количество загружаемых материалов.
    //--------------------------------------------------------------------------------------------------------
    Material DefaultMaterial;                               // Стандартный материал.
    Material DefaultUiMaterial;                             // Стандартный материал пользовательского интерфейса.
    Material DefaultTerrainMaterial;                        // Стандартный материал ландшафта.
    Material* RegisteredMaterials;                          // Массив зарегистрированных материалов.

    HashTable<MaterialReference> RegisteredMaterialTable;   // Хэш-таблица для поиска материалов.

    MaterialShaderUniformLocations MaterialLocations;       // Известные местоположения шейдера материала.
    u32 MaterialShaderID;

    UiShaderUniformLocations UiLocations;
    u32 UiShaderID;

    // Известные места для шейдера ландшафта.
    TerrainShaderLocations TerrainLocations;
    u32 TerrainShaderID;

    /// @brief Инициализирует систему материалов при создании объекта.
    constexpr sMaterialSystem() : MaxMaterialCount(), DefaultMaterial(), RegisteredMaterials(nullptr), RegisteredMaterialTable(), MaterialLocations(), MaterialShaderID(), UiLocations(), UiShaderID() {}
    sMaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, MaterialReference* HashTableBlock);
    ~sMaterialSystem();
};

static sMaterialSystem* state = nullptr;

static bool CreateDefaultMaterial();
static bool CreateDefaultUiMaterial();
static bool CreateDefaultTerrainMaterial();
static bool LoadMaterial(const Material::Config& config, Material* m);
static void DestroyMaterial(Material* material);
static bool AssignMap(TextureMap& map, const Material::Map& config, const char* MaterialName, Texture* DefaultTex);

sMaterialSystem::sMaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, MaterialReference* HashTableBlock) 
: 
MaxMaterialCount(MaxMaterialCount),
// DefaultMaterial(), 
RegisteredMaterials(new(RegisteredMaterials) Material[MaxMaterialCount]()),
RegisteredMaterialTable(MaxMaterialCount, false, HashTableBlock, true, MaterialReference(0, INVALID::ID, true)), 
MaterialLocations(),
MaterialShaderID(INVALID::ID), 
UiLocations(),
UiShaderID(INVALID::ID)
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
    DestroyMaterial(&state->DefaultUiMaterial);
    DestroyMaterial(&state->DefaultTerrainMaterial);
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

    if (!CreateDefaultUiMaterial()) {
        MFATAL("Не удалось создать материал пользовательского интерфейса по умолчанию. Приложение не может продолжать работу.");
        return false;
    }

    if (!CreateDefaultTerrainMaterial()) {
        MFATAL("Не удалось создать материал ландшафта по умолчанию. Приложение не может продолжать работу.");
        return false;
    }

    // Получите единые индексы.
    // Сохраните местоположения известных типов для быстрого поиска.
    auto shader = ShaderSystem::GetShader("Shader.Builtin.Material"); 
    state->MaterialShaderID                  = shader->id;
    state->MaterialLocations.projection      = ShaderSystem::UniformIndex(shader,       "projection");
    state->MaterialLocations.view            = ShaderSystem::UniformIndex(shader,             "view");
    state->MaterialLocations.AmbientColour   = ShaderSystem::UniformIndex(shader,   "ambient_colour");
    state->MaterialLocations.ViewPosition    = ShaderSystem::UniformIndex(shader,    "view_position");
    state->MaterialLocations.properties      = ShaderSystem::UniformIndex(shader,       "properties");
    state->MaterialLocations.DiffuseTexture  = ShaderSystem::UniformIndex(shader,  "diffuse_texture");
    state->MaterialLocations.SpecularTexture = ShaderSystem::UniformIndex(shader, "specular_texture");
    state->MaterialLocations.NormalTexture   = ShaderSystem::UniformIndex(shader,   "normal_texture");

    state->MaterialLocations.model           = ShaderSystem::UniformIndex(shader,            "model");
    state->MaterialLocations.RenderMode      = ShaderSystem::UniformIndex(shader,             "mode");
    state->MaterialLocations.DirLight        = ShaderSystem::UniformIndex(shader,        "dir_light");
    state->MaterialLocations.PointLights     = ShaderSystem::UniformIndex(shader,         "p_lights");
    state->MaterialLocations.NumPointLights  = ShaderSystem::UniformIndex(shader,     "num_p_lights");

    shader = ShaderSystem::GetShader("Shader.Builtin.UI");
    state->UiShaderID = shader->id;
    state->UiLocations.projection           = ShaderSystem::UniformIndex(shader,      "projection");
    state->UiLocations.view                 = ShaderSystem::UniformIndex(shader,            "view");
    state->UiLocations.properties           = ShaderSystem::UniformIndex(shader,      "properties");
    state->UiLocations.DiffuseTexture       = ShaderSystem::UniformIndex(shader, "diffuse_texture");
    state->UiLocations.model                = ShaderSystem::UniformIndex(shader,           "model");
    
    shader = ShaderSystem::GetShader("Shader.Builtin.Terrain");
    state->TerrainShaderID = shader->id;
    state->TerrainLocations.projection      = ShaderSystem::UniformIndex(shader,     "projection");
    state->TerrainLocations.view            = ShaderSystem::UniformIndex(shader,           "view");
    state->TerrainLocations.AmbientColour   = ShaderSystem::UniformIndex(shader, "ambient_colour");
    state->TerrainLocations.ViewPosition    = ShaderSystem::UniformIndex(shader,  "view_position");
    state->TerrainLocations.model           = ShaderSystem::UniformIndex(shader,          "model");
    state->TerrainLocations.RenderMode      = ShaderSystem::UniformIndex(shader,           "mode");
    state->TerrainLocations.DirLight        = ShaderSystem::UniformIndex(shader,      "dir_light");
    state->TerrainLocations.PointLights     = ShaderSystem::UniformIndex(shader,       "p_lights");
    state->TerrainLocations.NumPointLights  = ShaderSystem::UniformIndex(shader,   "num_p_lights");

    state->TerrainLocations.properties      = ShaderSystem::UniformIndex(shader,     "properties");

    state->TerrainLocations.samplers[0]     = ShaderSystem::UniformIndex(shader,  "diffuse_texture_0");
    state->TerrainLocations.samplers[1]     = ShaderSystem::UniformIndex(shader, "specular_texture_0");
    state->TerrainLocations.samplers[2]     = ShaderSystem::UniformIndex(shader,   "normal_texture_0");

    state->TerrainLocations.samplers[3]     = ShaderSystem::UniformIndex(shader,  "diffuse_texture_1");
    state->TerrainLocations.samplers[4]     = ShaderSystem::UniformIndex(shader, "specular_texture_1");
    state->TerrainLocations.samplers[5]     = ShaderSystem::UniformIndex(shader,   "normal_texture_1");

    state->TerrainLocations.samplers[6]     = ShaderSystem::UniformIndex(shader,  "diffuse_texture_2");
    state->TerrainLocations.samplers[7]     = ShaderSystem::UniformIndex(shader, "specular_texture_2");
    state->TerrainLocations.samplers[8]     = ShaderSystem::UniformIndex(shader,   "normal_texture_2");

    state->TerrainLocations.samplers[9]     = ShaderSystem::UniformIndex(shader,  "diffuse_texture_3");
    state->TerrainLocations.samplers[10]    = ShaderSystem::UniformIndex(shader, "specular_texture_3");
    state->TerrainLocations.samplers[11]    = ShaderSystem::UniformIndex(shader,   "normal_texture_3");

    return true;
}

void MaterialSystem::Shutdown()
{
    if (state) {
        state->~sMaterialSystem(); // delete state;
        state = nullptr;
    }
}

static Material* AcquireReference(const char* name, bool AutoRelease, bool& NeedsCreation)
{
    MaterialReference ref;
    if (state && state->RegisteredMaterialTable.Get(name, &ref)) {
        // Это можно изменить только при первой загрузке материала.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = AutoRelease;
        }
        ref.ReferenceCount++;
        if (ref.handle == INVALID::ID) {
            // Это означает, что здесь нет материала. Сначала найдите свободный индекс.
            u32 count = state->MaxMaterialCount;
            Material* m = nullptr;
            for (u32 i = 0; i < count; ++i) {
                if (state->RegisteredMaterials[i].id == INVALID::ID) {
                    // Найден свободный слот. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    m = &state->RegisteredMaterials[i];
                    break;
                }
            }

            // Убедитесь, что действительно найден пустой слот.
            if (!m || ref.handle == INVALID::ID) {
                MFATAL("MaterialSystem::Acquire — Система материалов не может больше содержать материалы. Измените конфигурацию, чтобы разрешить больше.");
                return nullptr;
            }

            NeedsCreation = true;

            // Также используйте дескриптор в качестве идентификатора материала.
            m->id = ref.handle;
            // KTRACE("Материал '%s' еще не существует. Создан, и ref_count теперь равен %i.", config.name, ref.reference_count);
        } else {
            // KTRACE("Материал '%s' уже существует, ref_count увеличен до %i.", config.name, ref.reference_count);
            NeedsCreation = false;
        }

        // Обновите запись.
        state->RegisteredMaterialTable.Set(name, ref);
        return &state->RegisteredMaterials[ref.handle];
    }

    // ПРИМЕЧАНИЕ: Это произойдет только в том случае, если что-то пойдет не так с состоянием.
    MERROR("MaterialSystem::Acquire не удалось получить материал '%s'. Будет возвращен нулевой указатель.", name);
    return nullptr;
}

Material *MaterialSystem::Acquire(const char* name) 
{
    // Загрузить конфигурацию материала из ресурса;
    MaterialResource materialResource;
    if (!ResourceSystem::Load(name, eResource::Material, nullptr, materialResource)) {
        MERROR("Не удалось загрузить материальный ресурс, возвращено nullptr.");
        return nullptr;
    }

    // Теперь получите из загруженной конфигурации.
    auto material = Acquire(materialResource.data);

    // Очистить
    // ResourceSystem::Unload(materialResource);

    if (!material) {
        MERROR("Не удалось загрузить материальный ресурс, возвращено nullptr.");
    }

    return material;
}

Material *MaterialSystem::Acquire(const Material::Config &config)
{
    // Верните материал по умолчанию.
    if (config.name.Comparei(DEFAULT_MATERIAL_NAME)) {
        return &state->DefaultMaterial;
    }

    // Верните материал пользовательского интерфейса по умолчанию.
    if (config.name.Comparei(DEFAULT_UI_MATERIAL_NAME)) {
        return &state->DefaultUiMaterial;
    }

    // Верните материал ландшафта по умолчанию.
    if (config.name.Comparei(DEFAULT_TERRAIN_MATERIAL_NAME)) {
        return &state->DefaultTerrainMaterial;
    }

    bool NeedsCreation = false;
    auto material = AcquireReference(config.name.c_str(), config.AutoRelease, NeedsCreation);

    if (NeedsCreation) {
        // Создайте новый материал.
        if (!LoadMaterial(config, material)) {
            MERROR("Не удалось загрузить материал '%s'.", config.name.c_str());
            return nullptr;
        }

        if (material->generation == INVALID::ID) {
            material->generation = 0;
        } else {
            material->generation++;
        }
    }

    return material;
}

Material* MaterialSystem::Acquire(const char* MaterialName, u32 MaterialCount, const char** MaterialNames, bool AutoRelease)
{
    // Верните материал ландшафта по умолчанию.
    if (MString::Equali(MaterialName, DEFAULT_TERRAIN_MATERIAL_NAME)) {
        return &state->DefaultTerrainMaterial;
    }

    bool NeedsCreation = false;
    auto material = AcquireReference(MaterialName, AutoRelease, NeedsCreation);
    if (!material) {
        MERROR("Не удалось получить материал ландшафта '%s'", MaterialName);
        return nullptr;
    }

    if (NeedsCreation) {
        // Получите все материалы по имени;
        auto materials = reinterpret_cast<Material**>(MemorySystem::Allocate(sizeof(Material*) * MaterialCount, Memory::Array));
        for (u32 i = 0; i < MaterialCount; ++i) {
            materials[i] = Acquire(MaterialNames[i]);
        }

        // Создайте новый материал.
        // ПРИМЕЧАНИЕ: load_material, зависящий от ландшафта
        MemorySystem::ZeroMem(material, sizeof(Material));
        MString::Copy(material->name, MaterialName, MATERIAL_NAME_MAX_LENGTH);

        auto shader = ShaderSystem::GetShader("Shader.Builtin.Terrain");
        material->ShaderID = shader->id;
        material->type = Material::Type::Terrain;

        // Выделите память для карт и свойств.
        material->PropertyStructSize = sizeof(Material::TerrainProperties);
        auto properties = reinterpret_cast<Material::TerrainProperties*>(MemorySystem::Allocate(material->PropertyStructSize, Memory::MaterialInstance, true));
        material->properties = properties;
        properties->NumMaterials = MaterialCount;

        // 3 карты на материал. Выделите достаточно слотов для всех материалов.
        // u32 map_count = material_count * 3;
        u32 MaxMapCount = TERRAIN_MAX_MATERIAL_COUNT * 3;
        material->maps.Resize(MaxMapCount);

        // Имена карт и резервные текстуры по умолчанию.
        const char* MapNames[3] = {"diffuse", "specular", "normal"};
        Texture* DefaultTextures[3] = {
            TextureSystem::GetDefaultTexture(Texture::Diffuse),
            TextureSystem::GetDefaultTexture(Texture::Specular),
            TextureSystem::GetDefaultTexture(Texture::Normal) };
        // Используйте материал по умолчанию для неназначенных слотов.
        auto DefaultMaterial = GetDefaultMaterial();

        // Свойства и карты Фонга для каждого материала.
        for (u32 MaterialIdx = 0; MaterialIdx < TERRAIN_MAX_MATERIAL_COUNT; ++MaterialIdx) {
            // Свойства.
            auto& MatProps = properties->materials[MaterialIdx];
            // Используйте материал по умолчанию, если он не находится в пределах количества материалов.
            auto RefMat = DefaultMaterial;
            if (MaterialIdx < MaterialCount) {
                RefMat = materials[MaterialIdx];
            }

            auto props = (Material::PhongProperties*)RefMat->properties;
            MatProps.DiffuseColour = props->DiffuseColour;
            MatProps.specular = props->specular;
            // MatProps.padding = FVec3();

            // Карты, 3 для Фонга. Диффузный, спек, нормальный.
            for (u32 MapIdx = 0; MapIdx < 3; ++MapIdx) {
                Material::Map MapConfig = {0};
                char buf[MATERIAL_NAME_MAX_LENGTH]{};
                MapConfig.name = buf;
                MapConfig.name = MapNames[MapIdx];
                MapConfig.RepeatU = RefMat->maps[MapIdx].RepeatU;
                MapConfig.RepeatV = RefMat->maps[MapIdx].RepeatV;
                MapConfig.RepeatW = RefMat->maps[MapIdx].RepeatW;
                MapConfig.FilterMin = RefMat->maps[MapIdx].FilterMinify;
                MapConfig.FilterMag = RefMat->maps[MapIdx].FilterMagnify;
                MapConfig.TextureName = RefMat->maps[MapIdx].texture->name;
                if (!AssignMap(material->maps[(MaterialIdx * 3) + MapIdx], MapConfig, material->name, DefaultTextures[MapIdx])) {
                    MERROR("Не удалось назначить текстурную карту '%s' для индекса материала ландшафта %u", MapNames[MapIdx], MaterialIdx);
                    return nullptr;
                }
            }
        }

        // Выпустить справочные материалы.
        for (u32 i = 0; i < MaterialCount; ++i) {
            MaterialSystem::Release(MaterialNames[i]);
        }

        MemorySystem::Free(materials, sizeof(Material*) * MaterialCount, Memory::Array);

        // Приобрести ресурсы экземпляров для всех карт.

        auto maps = reinterpret_cast<TextureMap**>(MemorySystem::Allocate(sizeof(TextureMap*) * MaxMapCount, Memory::Array));
        // Назначьте карты материалов.
        for (u32 i = 0; i < MaxMapCount; ++i) {
            maps[i] = &material->maps[i];
        }
        
        bool result = RenderingSystem::ShaderAcquireInstanceResources(shader, MaxMapCount, maps, material->InternalId);
        if (!result) {
            MERROR("Не удалось получить ресурсы рендеринга для материала '%s'.", material->name);
        }

        if (maps) {
            MemorySystem::Free(maps, sizeof(TextureMap*) * MaxMapCount, Memory::Array);
        }
        // ПРИМЕЧАНИЕ: конец load_material, специфичного для местности

        if (material->generation == INVALID::ID) {
            material->generation = 0;
        } else {
            material->generation++;
        }
    }

    return material;
}

void MaterialSystem::Release(const char *name)
{
    // Игнорируйте запросы на выпуск материала по умолчанию.
    if (MString::Equali(name, DEFAULT_MATERIAL_NAME)         ||
        MString::Equali(name, DEFAULT_TERRAIN_MATERIAL_NAME) || 
        MString::Equali(name, DEFAULT_UI_MATERIAL_NAME)) {
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

bool MaterialSystem::ApplyGlobal(u32 ShaderID, const FrameData& rFrameData, const Matrix4D &projection, const Matrix4D &view, const FVec4& AmbientColour, const FVec3& ViewPosition, u32 RenderMode)
{
    auto shader = ShaderSystem::GetShader(ShaderID);
    if (!shader) {
        return false;
    }

    if (shader->RenderFrameNumber == rFrameData.RendererFrameNumber && shader->DrawIndex == rFrameData.DrawIndex) {
        return true;
    }
    
    if (ShaderID == state->MaterialShaderID || ShaderID == state->TerrainShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.view, &view));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.AmbientColour, &AmbientColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.ViewPosition, &ViewPosition));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.RenderMode, &RenderMode));
    } else if (ShaderID == state->UiShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UiLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UiLocations.view, &view));
    } else {
        MERROR("MaterialSystem::ApplyGlobal(): Неизвестный идентификатор шейдера '%d' ", ShaderID);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyGlobal(true));
    // Синхронизация номера кадра
    shader->RenderFrameNumber = rFrameData.RendererFrameNumber;
    return true;
}

bool MaterialSystem::ApplyInstance(Material *material, const FrameData& rFrameData, bool NeedsUpdate)
{
    // Примените униформу на уровне экземпляра.
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::BindInstance(material->InternalId));
    if(NeedsUpdate) {
        if (material->ShaderID == state->MaterialShaderID) {
            // Шейдер материала
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.properties, material->properties));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DiffuseTexture, &material->maps[0]));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.SpecularTexture, &material->maps[1]));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.NormalTexture, &material->maps[2]));

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
                auto PointLights = reinterpret_cast<PointLight*>(rFrameData.FrameAllocator->Allocate(PointLightsSize));
                LightSystem::GetPointLights(PointLights);

                u64 PointLightDatasSize = sizeof(PointLight::Data) * PointLightCount;
                auto PointLightDatas = reinterpret_cast<PointLight::Data*>(rFrameData.FrameAllocator->Allocate(PointLightDatasSize));
                for (u32 i = 0; i < PointLightCount; ++i) {
                    PointLightDatas[i] = PointLights[i].data;
                }

                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.PointLights, PointLightDatas));
            }

            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.NumPointLights, &PointLightCount)); 
        } else if (material->ShaderID == state->UiShaderID) {
            // шейдер пользовательского интерфейса
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UiLocations.properties, material->properties));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->UiLocations.DiffuseTexture, &material->maps[0]));
        } else if (material->ShaderID == state->TerrainShaderID) {
            // Применить карты
            const u32& MapCount = material->maps.Length();
            for (u32 i = 0; i < MapCount; ++i) {
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.samplers[i], &material->maps[i]));
            }

            // Применить свойства.
            ShaderSystem::UniformSet(state->TerrainLocations.properties, material->properties);

            // ЗАДАЧА: Дублирование выше... переместить это в отдельную функцию, возможно.
            // Направленный свет.
            auto DirLight = LightSystem::GetDirectionalLight();
            if (DirLight) {
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.DirLight, &DirLight->data));
            } else {
                DirectionalLight::Data data{};
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.DirLight, &data));
            }
            // Точечные источники света.
            u32 PointLightCount = LightSystem::PointLightCount();
            if (PointLightCount) {
                // ЗАДАЧА: Распределитель кадров?
                u64 PointLightsSize = sizeof(PointLight) * PointLightCount;
                auto PointLights = reinterpret_cast<PointLight*>(rFrameData.FrameAllocator->Allocate(PointLightsSize));
                LightSystem::GetPointLights(PointLights);

                u64 PointLightDatasSize = sizeof(PointLight::Data) * PointLightCount;
                auto PointLightDatas = reinterpret_cast<PointLight::Data*>(rFrameData.FrameAllocator->Allocate(PointLightDatasSize));
                for (u32 i = 0; i < PointLightCount; ++i) {
                    PointLightDatas[i] = PointLights[i].data;
                }

                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.PointLights, PointLightDatas));
            }

            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.NumPointLights, &PointLightCount));
        } else {
            MERROR("MaterialSystem::ApplyInstance(): Нераспознанный идентификатор шейдера «%d» в шейдере «%s».", material->ShaderID, material->name);
            return false;
        }
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyInstance(NeedsUpdate));

    return true;
}

bool MaterialSystem::ApplyLocal(Material *material, const Matrix4D &model)
{
    if (material->ShaderID == state->MaterialShaderID) {
        return ShaderSystem::UniformSet(state->MaterialLocations.model, &model);
    } else if (material->ShaderID == state->UiShaderID) {
        return ShaderSystem::UniformSet(state->UiLocations.model, &model);
    } else if (material->ShaderID == state->TerrainShaderID) {
        return ShaderSystem::UniformSet(state->TerrainLocations.model, &model);
    }

    MERROR("Неизвестный идентификатор шейдера «%d»", material->ShaderID);
    return false;
}

Material *MaterialSystem::GetDefaultMaterial()
{
    if (state) {
        return &state->DefaultMaterial;
    }

    MFATAL("MaterialSystem::GetDefaultMaterial вызывается перед инициализацией системы.");
    return nullptr;
}

Material *MaterialSystem::GetDefaultUiMaterial()
{
    if (state) {
        return &state->DefaultUiMaterial;
    }

    MFATAL("MaterialSystem::GetDefaultUiMaterial вызывается перед инициализацией системы.");
    return nullptr;
}

Material *MaterialSystem::GetDefaultTerrainMaterial()
{
    if (state) {
        return &state->DefaultTerrainMaterial;
    }

    MFATAL("MaterialSystem::GetDefaultTerrainMaterial вызывается перед инициализацией системы.");
    return nullptr;
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

static bool AssignMap(TextureMap& map, const Material::Map& config, const char* MaterialName, Texture* DefaultTex) {
    map.FilterMinify = config.FilterMin;
    map.FilterMagnify = config.FilterMag;
    map.RepeatU = config.RepeatU;
    map.RepeatV = config.RepeatU;
    map.RepeatW = config.RepeatW;

    if (config.TextureName.Length() > 0) {
        map.texture = TextureSystem::Acquire(config.TextureName.c_str(), true);
        if (!map.texture) {
            // Настроено, но не найдено.
            MWARN("Невозможно загрузить текстуру «%s» для материала «%s», используются значения по умолчанию.", config.TextureName.c_str(), MaterialName);
            map.texture = DefaultTex;
        }
    } else {
        // Это делается, когда текстура не настроена, в отличие от случая, когда она настроена и не найдена (выше).
        map.texture = DefaultTex;
    }
    if (!RenderingSystem::TextureMapAcquireResources(&map)) {
        MERROR("Невозможно получить ресурсы для карты текстуры.");
        return false;
    }
    return true;
}

bool CreateDefaultMaterial()
{
    MString::Copy(state->DefaultMaterial.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->DefaultMaterial.id = INVALID::ID;
    state->DefaultMaterial.type = Material::Type::Phong;
    state->DefaultMaterial.generation = INVALID::ID;
    state->DefaultMaterial.PropertyStructSize = sizeof(Material::PhongProperties);
    state->DefaultMaterial.properties = MemorySystem::Allocate(state->DefaultMaterial.PropertyStructSize, Memory::MaterialInstance);
    auto properties = (Material::PhongProperties*)state->DefaultMaterial.properties;
    properties->DiffuseColour = FVec4::One();  // белый
    properties->specular = 8.F;
    state->DefaultMaterial.maps.Resize(3);
    for (u32 i = 0; i < 3; ++i) {
        auto& map = state->DefaultMaterial.maps[i];
        map.FilterMagnify = map.FilterMinify = TextureFilter::ModeLinear;
        map.RepeatU = map.RepeatV = map.RepeatW = TextureRepeat::Repeat;
    }
    state->DefaultMaterial.maps[0].texture = TextureSystem::GetDefaultTexture(Texture::Default);
    state->DefaultMaterial.maps[1].texture = TextureSystem::GetDefaultTexture(Texture::Specular);
    state->DefaultMaterial.maps[2].texture = TextureSystem::GetDefaultTexture(Texture::Normal);
    // ПРИМЕЧАНИЕ: материал Фонга используется по умолчанию.
    TextureMap* maps[3] = {&state->DefaultMaterial.maps[0], &state->DefaultMaterial.maps[1], &state->DefaultMaterial.maps[2]};
    
    Shader* shader = ShaderSystem::GetShader("Shader.Builtin.Material");
    if (!RenderingSystem::ShaderAcquireInstanceResources(shader, 3, maps, state->DefaultMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы средства рендеринга для материала по умолчанию. Приложение не может быть продолжено.");
        return false;
    }
    // Обязательно укажите идентификатор шейдера.
    state->DefaultMaterial.ShaderID = shader->id;

    return true;
}

bool CreateDefaultUiMaterial()
{
    MString::Copy(state->DefaultUiMaterial.name, DEFAULT_UI_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->DefaultUiMaterial.id = INVALID::ID;
    state->DefaultUiMaterial.type = Material::Type::UI;
    state->DefaultUiMaterial.generation = INVALID::ID;
    state->DefaultUiMaterial.PropertyStructSize = sizeof(Material::UiProperties);
    state->DefaultUiMaterial.properties = MemorySystem::Allocate(state->DefaultUiMaterial.PropertyStructSize, Memory::MaterialInstance);
    auto properties = (Material::UiProperties*)state->DefaultUiMaterial.properties;
    properties->DiffuseColour = FVec4::One();  // белый
    state->DefaultUiMaterial.maps.Resize(1);
    state->DefaultUiMaterial.maps[0].texture = TextureSystem::GetDefaultTexture(Texture::Default);

    TextureMap* maps[1] = {&state->DefaultUiMaterial.maps[0]};

    Shader* shader = ShaderSystem::GetShader("Shader.Builtin.UI");
    if (!RenderingSystem::ShaderAcquireInstanceResources(shader, 1, maps, state->DefaultUiMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы рендерера для материала пользовательского интерфейса по умолчанию. Приложение не может продолжать работу.");
        return false;
    }

    // Обязательно назначьте идентификатор шейдера.
    state->DefaultUiMaterial.ShaderID = shader->id;

    return true;
}

bool CreateDefaultTerrainMaterial()
{
    state->DefaultTerrainMaterial.id = INVALID::ID;
    state->DefaultTerrainMaterial.type = Material::Type::Terrain;
    state->DefaultTerrainMaterial.generation = INVALID::ID;
    MString::Copy(state->DefaultTerrainMaterial.name, DEFAULT_TERRAIN_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);

    // По сути, это должно быть то же самое, что и материал по умолчанию, просто сопоставленный с «массивом» одного материала.
    state->DefaultTerrainMaterial.PropertyStructSize = sizeof(Material::TerrainProperties);
    state->DefaultTerrainMaterial.properties = MemorySystem::Allocate(state->DefaultTerrainMaterial.PropertyStructSize, Memory::MaterialInstance);
    auto properties = (Material::TerrainProperties*)state->DefaultTerrainMaterial.properties;
    properties->NumMaterials = 1;
    properties->materials[0].DiffuseColour = FVec4::One();  // белый
    properties->materials[0].specular = 8.F;
    state->DefaultTerrainMaterial.maps.Resize(12);
    state->DefaultTerrainMaterial.maps[0].texture = TextureSystem::GetDefaultTexture(Texture::Default);
    state->DefaultTerrainMaterial.maps[1].texture = TextureSystem::GetDefaultTexture(Texture::Specular);
    state->DefaultTerrainMaterial.maps[2].texture = TextureSystem::GetDefaultTexture(Texture::Normal);

    // ПРИМЕЧАНИЕ: материал Phong используется по умолчанию.
    TextureMap* maps[3] = {
        &state->DefaultTerrainMaterial.maps[0],
        &state->DefaultTerrainMaterial.maps[1],
        &state->DefaultTerrainMaterial.maps[2],
    };

    Shader* shader = ShaderSystem::GetShader("Shader.Builtin.Terrain");
    if (!RenderingSystem::ShaderAcquireInstanceResources(shader, 3, maps, state->DefaultTerrainMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы рендерера для материала ландшафта по умолчанию. Приложение не может продолжать работу.");
        return false;
    }

    // Обязательно назначьте идентификатор шейдера.
    state->DefaultTerrainMaterial.ShaderID = shader->id;

    return true;
}

static bool LoadMaterial(const Material::Config &config, Material *material)
{ 
    MemorySystem::ZeroMem(material, sizeof(Material));

    // Название
    MString::Copy(material->name, config.name.c_str(), MATERIAL_NAME_MAX_LENGTH);

    material->ShaderID = ShaderSystem::GetID(config.ShaderName);
    material->type = config.type;

    // Свойства и карты Фонга.
    switch (config.type) {
        case Material::Type::Phong: {
            // Специфические свойства Фонга.
            const u32& PropCount = config.properties.Length();

            // Значения по умолчанию
            material->PropertyStructSize = sizeof(Material::PhongProperties);
            auto properties = (Material::PhongProperties*)MemorySystem::Allocate(material->PropertyStructSize, Memory::MaterialInstance, true);
            material->properties = properties;
            properties->DiffuseColour = FVec4::One();
            properties->specular = 32.F;

            for (u32 i = 0; i < PropCount; ++i) {
                if (config.properties[i].name.Comparei("diffuse_colour")) {
                    // Рассеянный цвет
                    properties->DiffuseColour = config.properties[i].ValueV4;
                } else if (config.properties[i].name.Comparei("specular")) {
                    // Блеск
                    properties->specular = config.properties[i].ValueF32;
                }
            }

            // Карты. Фонг ожидает диффузный, зеркальный и нормальный.
            material->maps.Resize(3);
            const u32& MapCount = config.maps.Length();
            bool DiffuseAssigned = false;
            bool SpecAssigned = false;
            bool NormAssigned = false;
            for (u32 i = 0; i < MapCount; ++i) {
                if (config.maps[i].name.Comparei("diffuse")) {
                    if (!AssignMap(material->maps[0], config.maps[i], material->name, TextureSystem::GetDefaultTexture(Texture::Diffuse))) {
                        return false;
                    }
                    DiffuseAssigned = true;
                } else if (config.maps[i].name.Comparei("specular")) {
                    if (!AssignMap(material->maps[1], config.maps[i], material->name, TextureSystem::GetDefaultTexture(Texture::Specular))) {
                        return false;
                    }
                    SpecAssigned = true;
                } else if (config.maps[i].name.Comparei("normal")) {
                    if (!AssignMap(material->maps[2], config.maps[i], material->name, TextureSystem::GetDefaultTexture(Texture::Normal))) {
                        return false;
                    }
                    NormAssigned = true;
                }
                // ЗАДАЧА: другие карты
                // ПРИМЕЧАНИЕ: игнорируйте неожиданные карты.
            }
            if (!DiffuseAssigned) {
                // Убедитесь, что диффузная карта всегда назначена.
                Material::Map MapConfig{};
                MapConfig.FilterMag = MapConfig.FilterMin = TextureFilter::ModeLinear;
                MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::Repeat;
                MapConfig.name = "diffuse";
                //MapConfig.TextureName = "";
                if (!AssignMap(material->maps[0], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Diffuse))) {
                    return false;
                }
            }
            if (!SpecAssigned) {
                // Убедитесь, что зеркальная карта всегда назначена.
                Material::Map MapConfig{};
                MapConfig.FilterMag = MapConfig.FilterMin = TextureFilter::ModeLinear;
                MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::Repeat;
                MapConfig.name = "specular";
                // MapConfig.TextureName = "";
                if (!AssignMap(material->maps[1], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Specular))) {
                    return false;
                }
            }
            if (!NormAssigned) {
                // Убедитесь, что нормальная карта всегда назначена.
                Material::Map MapConfig{};
                MapConfig.FilterMag = MapConfig.FilterMin = TextureFilter::ModeLinear;
                MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::Repeat;
                MapConfig.name = "normal";
                // MapConfig.TextureName = "";
                if (!AssignMap(material->maps[2], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Normal))) {
                    return false;
                }
            }
        }
            break;
        case Material::Type::UI: {
            // ПРИМЕЧАНИЕ: только одна карта и свойство, поэтому просто используйте первое.
            // ЗАДАЧА: Если это изменится, обновите это, чтобы оно работало так, как указано выше.
            material->maps.Resize(1);
            material->PropertyStructSize = sizeof(Material::UiProperties);
            material->properties = MemorySystem::Allocate(material->PropertyStructSize, Memory::MaterialInstance);
            auto properties = reinterpret_cast<Material::UiProperties*>(material->properties);
            properties->DiffuseColour = config.properties[0].ValueV4;
            if (!AssignMap(material->maps[0], config.maps[0], material->name, TextureSystem::GetDefaultTexture(Texture::Diffuse))) {
                return false;
            }
        }
            break;
        case  Material::Type::Custom: {
            // Свойства.
            const u32& prop_count = config.properties.Length();
            // Начните с получения общего размера всех свойств.
            material->PropertyStructSize = 0;
            for (u32 i = 0; i < prop_count; ++i) {
                if (config.properties[i].size > 0) {
                    material->PropertyStructSize += config.properties[i].size;
                }
            }
            // Выделите достаточно места для структуры.
            material->properties = MemorySystem::Allocate(material->PropertyStructSize, Memory::MaterialInstance);

            // Повторите цикл и скопируйте значения в структуру. ПРИМЕЧАНИЕ: для пользовательских униформ материалов нет значений по умолчанию.
            u32 offset = 0;
            for (u32 i = 0; i < prop_count; ++i) {
                if (config.properties[i].size > 0) {
                    void* data = nullptr;
                    switch (config.properties[i].type) {
                        case Shader::UniformType::Int8:
                            data = (void*)&config.properties[i].ValueI8;
                            break;
                        case Shader::UniformType::UInt8:
                            data = (void*)&config.properties[i].ValueU8;
                            break;
                        case Shader::UniformType::Int16:
                            data = (void*)&config.properties[i].ValueI16;
                            break;
                        case Shader::UniformType::UInt16:
                            data = (void*)&config.properties[i].ValueU16;
                            break;
                        case Shader::UniformType::Int32:
                            data = (void*)&config.properties[i].ValueI32;
                            break;
                        case Shader::UniformType::UInt32:
                            data = (void*)&config.properties[i].ValueU32;
                            break;
                        case Shader::UniformType::Float32:
                            data = (void*)&config.properties[i].ValueF32;
                            break;
                        case Shader::UniformType::Float32_2:
                            data = (void*)&config.properties[i].ValueV2;
                            break;
                        case Shader::UniformType::Float32_3:
                            data = (void*)&config.properties[i].ValueV3;
                            break;
                        case Shader::UniformType::Float32_4:
                            data = (void*)&config.properties[i].ValueV4;
                            break;
                        case Shader::UniformType::Matrix4:
                            data = (void*)&config.properties[i].ValueMatrix4D;
                            break;
                        default:
                            // ЗАДАЧА: пользовательский размер?
                            MWARN("Невозможно обработать тип однородной шейдерной модели %d (индекс %u) для материала '%s'. Пропуск.", config.properties[i].type, i, material->name);
                            continue;
                    }

                    // Копируем блок и перемещаемся вверх.
                    MemorySystem::CopyMem((u8*)material->properties + offset, data, config.properties[i].size);
                    offset += config.properties[i].size;
                }
            }

            // Карты. Пользовательские материалы могут иметь любое количество карт.
            const u32& MapCount = config.maps.Length();
            material->maps.Resize(MapCount);
            for (u32 i = 0; i < MapCount; ++i) {
                // Нет известного сопоставления, поэтому просто сопоставляйте их по порядку.
                // Недействительные текстуры будут использовать текстуру по умолчанию, поскольку тип карты неизвестен.
                if (!AssignMap(material->maps[i], config.maps[i], material->name, TextureSystem::GetDefaultTexture(Texture::Default))) {
                    return false;
                }
            }
        }
            break;
        case Material::Type::PBR: case Material::Type::Terrain: case Material::Type::Unknown: break;
    }

    // Отправьте его в рендерер для получения ресурсов.
    Shader* shader = nullptr;
    switch (config.type) {
        case Material::Type::Phong: shader = ShaderSystem::GetShader(config.ShaderName ? config.ShaderName : "Shader.Builtin.Material");
            break;
        case Material::Type::UI: shader = ShaderSystem::GetShader(config.ShaderName ? config.ShaderName : "Shader.Builtin.UI");
            break;
        case Material::Type::Terrain: shader = ShaderSystem::GetShader(config.ShaderName ? config.ShaderName : "Shader.Builtin.Terrain");
            break;
        case Material::Type::PBR: 
            MFATAL("PBR пока не поддерживается.");
            return false;
        case Material::Type::Custom: 
            if (!config.ShaderName) {
                MERROR("Имя шейдера требуется для пользовательских типов материалов. Материал '%s' не удалось загрузить", material->name);
                return false;
            }
            shader = ShaderSystem::GetShader(config.ShaderName);

        default: MERROR("Неизвестный тип материала: %d. Материал '%s' не может быть загружен.", config.type, material->name);
            return false;
            // break;
    }
    
    if (!shader) {
        MERROR("Не удалось загрузить материал, так как его шейдер не найден: '%s'. Вероятно, это проблема с материальным активом.", config.ShaderName.c_str());
        return false;
    }

    // Соберите список указателей на текстурные карты;
    u32 MapCount = 0;
    switch (config.type) {
        case Material::Type::Phong: /* Количество карт для этого типа известно. */
            MapCount = 3;
            break;
        case  Material::Type::UI: /* Количество карт для этого типа известно. */
            MapCount = 1;
            break;
        case Material::Type::PBR: MFATAL("PBR пока не поддерживается.");
            return false;
        case Material::Type::Custom: /* Количество карт предоставлено конфигурацией. */
            MapCount = config.maps.Length();
            break;
        case Material::Type::Terrain: case Material::Type::Unknown: break;
    }

    auto maps = reinterpret_cast<TextureMap**>(MemorySystem::Allocate(sizeof(TextureMap*) * MapCount, Memory::Array));
    for (u32 i = 0; i < MapCount; ++i) {
        maps[i] = &material->maps[i];
    }

    bool result = RenderingSystem::ShaderAcquireInstanceResources(shader, MapCount, maps, material->InternalId);
    if (!result) {
        MERROR("Не удалось получить ресурсы рендеринга для материала '%s'.", material->name);
    }

    if (maps) {
        MemorySystem::Free(maps, sizeof(TextureMap*) * MapCount, Memory::Array);
    }

    return result;
}

void DestroyMaterial(Material *material)
{
    // MTRACE("Уничтожение материала '%s'...", material->name);

    const u32& length = material->maps.Length();
    for (u32 i = 0; i < length; ++i) {
        // Освободить ссылки на текстуры.
        if (material->maps[i].texture) {
            TextureSystem::Release(material->maps[i].texture->name);
        }
        // Освободить ресурсы карты текстур.
        RenderingSystem::TextureMapReleaseResources(&material->maps[i]);
    }

    // Освободить ресурсы рендерера.
    if (material->ShaderID != INVALID::ID && material->InternalId != INVALID::ID) {
        RenderingSystem::ShaderReleaseInstanceResources(ShaderSystem::GetShader(material->ShaderID), material->InternalId);
        material->ShaderID = INVALID::ID;
    }

    // Свойства релиза
    if (material->properties && material->PropertyStructSize) {
        MemorySystem::Free(material->properties, material->PropertyStructSize, Memory::MaterialInstance);
    }

    // Обнулить это, сделать удостоверения недействительными.
    MemorySystem::ZeroMem(material, sizeof(Material));
    material->id = INVALID::ID;
    material->generation = INVALID::ID;
    material->InternalId = INVALID::ID;
    material = nullptr;
}
