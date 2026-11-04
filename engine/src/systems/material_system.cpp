#include "material_system.h"
#include "core/event.h"
#include "core/systems_manager.h"
#include "systems/texture_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/light_system.h"
#include "renderer/rendering_system.h"

#include "memory/linear_allocator.h"

#ifndef PBR_MAP_COUNT
#define PBR_MAP_COUNT 5
#endif

// Сэмплеры
constexpr u32 SAMP_ALBEDO         = 0;
constexpr u32 SAMP_NORMAL         = 1;
constexpr u32 SAMP_COMBINED       = 2;
constexpr u32 SAMP_SHADOW_MAP     = 3;
constexpr u32 SAMP_IRRADIANCE_MAP = 4;

// Количество текстур для PBR-материала
#define PBR_MATERIAL_TEXTURE_COUNT 3

// Материалы местности теперь загружаются в один общий массив текстур.
constexpr u32 SAMP_TERRAIN_MATERIAL_ARRAY_MAP = 0;
constexpr u32 SAMP_TERRAIN_SHADOW_MAP = 1 + SAMP_TERRAIN_MATERIAL_ARRAY_MAP;
constexpr u32 SAMP_TERRAIN_IRRADIANCE_MAP = 1 + SAMP_TERRAIN_SHADOW_MAP;
// 1 массивная карта для материалов рельефа, 1 для карты теней, 1 для карты освещённости
constexpr u32 TERRAIN_SAMP_COUNT = 3;

#define MAX_TERRAIN_MATERIAL_COUNT 4

static void DestroyMaterial(Material* material);
static bool AssignMap(TextureMap& map, const Material::Map& config, const char* MaterialName, Texture* DefaultTex);

MaterialSystem::MaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, Reference* HashTableBlock) 
: 
RegisteredMaterials(new(RegisteredMaterials) Material[MaxMaterialCount]()),
IrradianceCubeTexture(TextureSystem::GetDefaultTexture(Texture::Cube)), // Возьмите текстуру кубической карты по умолчанию в качестве текстуры освещенности.
ShadowTexture(), 
MaterialShader(ShaderSystem::GetShader("Shader.PBRMaterial")),
TerrainShader(ShaderSystem::GetShader("Shader.Builtin.Terrain")),
RegisteredMaterialTable(MaxMaterialCount, false, HashTableBlock, true, Reference(0, INVALID::ID, true)), 
MaterialLocations(),
MaxMaterialCount(MaxMaterialCount),
MaterialCount(),
MaterialShaderID(INVALID::ID),
TerrainShaderID(INVALID::ID), 
DirectionalLightSpace(Matrix4D::MakeIdentity(), Matrix4D::MakeIdentity(), Matrix4D::MakeIdentity(), Matrix4D::MakeIdentity())
{}

MaterialSystem* MaterialSystem::Initialize(u64& MemoryRequirement, void* memory, MaterialSystemConfig config)
{
    if (config.MaxMaterialCount == 0) {
        MFATAL("MaterialSystem::Initialize — MaxMaterialCount должен быть > 0.");
        return nullptr;
    }

    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 StructRequirement = sizeof(MaterialSystem);
    u64 ArrayRequirement = sizeof(Material) * config.MaxMaterialCount;
    u64 HashtableRequirement = sizeof(Reference) * config.MaxMaterialCount;
    MemoryRequirement = StructRequirement + ArrayRequirement + HashtableRequirement;

    if (!memory) {
        return nullptr;
    }

    if (!state) {
        u8* ptrMatSys = (u8*)memory;
        Material* RegisteredMaterials = (Material*)(ptrMatSys + StructRequirement);
        Reference* HashTableBlock = (Reference*)(RegisteredMaterials + config.MaxMaterialCount);
        state = new(ptrMatSys) MaterialSystem(config.MaxMaterialCount, RegisteredMaterials, HashTableBlock);
    }

    // Получите единые индексы.
    // Сохраните местоположения известных типов для быстрого поиска.
    state->MaterialShaderID                  = state->MaterialShader->id;
    state->MaterialLocations.projection      = state->MaterialShader->UniformLocation(       "projection");
    state->MaterialLocations.view            = state->MaterialShader->UniformLocation(             "view");
    state->MaterialLocations.CascadeSplits   = state->MaterialShader->UniformLocation(   "cascade_splits");
    state->MaterialLocations.ViewPosition    = state->MaterialShader->UniformLocation(    "view_position");
    state->MaterialLocations.properties      = state->MaterialShader->UniformLocation(       "properties");
    state->MaterialLocations.IblCubeTexture  = state->MaterialShader->UniformLocation( "ibl_cube_texture");
    state->MaterialLocations.MaterialTexures = state->MaterialShader->UniformLocation("material_textures");
    state->MaterialLocations.ShadowTextures  = state->MaterialShader->UniformLocation(  "shadow_textures");
    state->MaterialLocations.LightSpace0     = state->MaterialShader->UniformLocation(    "light_space_0");
    state->MaterialLocations.LightSpace1     = state->MaterialShader->UniformLocation(    "light_space_1");
    state->MaterialLocations.LightSpace2     = state->MaterialShader->UniformLocation(    "light_space_2");
    state->MaterialLocations.LightSpace3     = state->MaterialShader->UniformLocation(    "light_space_3");
    state->MaterialLocations.model           = state->MaterialShader->UniformLocation(            "model");
    state->MaterialLocations.RenderMode      = state->MaterialShader->UniformLocation(             "mode");
    state->MaterialLocations.UsPcf           = state->MaterialShader->UniformLocation(          "use_pcf");
    state->MaterialLocations.bias            = state->MaterialShader->UniformLocation(             "bias");
    state->MaterialLocations.DirLight        = state->MaterialShader->UniformLocation(        "dir_light");
    state->MaterialLocations.PointLights     = state->MaterialShader->UniformLocation(         "p_lights");
    state->MaterialLocations.NumPointLights  = state->MaterialShader->UniformLocation(     "num_p_lights");

    // state->UiShaderID = shader->id;
    // state->UiLocations.projection           = shader->UniformLocation(      "projection");
    // state->UiLocations.view                 = shader->UniformLocation(            "view");
    // state->UiLocations.properties           = shader->UniformLocation(      "properties");
    // state->UiLocations.DiffuseTexture       = shader->UniformLocation( "diffuse_texture");
    // state->UiLocations.model                = shader->UniformLocation(           "model");
    
    state->TerrainShaderID = state->TerrainShader->id;
    // state->TerrainLocations.loaded          = ;
    state->TerrainLocations.projection      = state->TerrainShader->UniformLocation(       "projection");
    state->TerrainLocations.view            = state->TerrainShader->UniformLocation(             "view");
    state->TerrainLocations.CascadeSplits   = state->TerrainShader->UniformLocation(   "cascade_splits");
    state->TerrainLocations.ViewPosition    = state->TerrainShader->UniformLocation(    "view_position");
    state->TerrainLocations.model           = state->TerrainShader->UniformLocation(            "model");
    state->TerrainLocations.RenderMode      = state->TerrainShader->UniformLocation(             "mode");
    state->TerrainLocations.DirLight        = state->TerrainShader->UniformLocation(        "dir_light");
    state->TerrainLocations.PointLights     = state->TerrainShader->UniformLocation(         "p_lights");
    state->TerrainLocations.NumPointLights  = state->TerrainShader->UniformLocation(     "num_p_lights");
    state->TerrainLocations.properties      = state->TerrainShader->UniformLocation(       "properties");
    state->TerrainLocations.IblCubeTexture  = state->TerrainShader->UniformLocation( "ibl_cube_texture");
    state->TerrainLocations.ShadowTextures  = state->TerrainShader->UniformLocation(  "shadow_textures");
    state->TerrainLocations.LightSpace0     = state->TerrainShader->UniformLocation(    "light_space_0");
    state->TerrainLocations.LightSpace1     = state->TerrainShader->UniformLocation(    "light_space_1");
    state->TerrainLocations.LightSpace2     = state->TerrainShader->UniformLocation(    "light_space_2");
    state->TerrainLocations.LightSpace3     = state->TerrainShader->UniformLocation(    "light_space_3");
    state->TerrainLocations.MaterialTexures = state->TerrainShader->UniformLocation("material_textures");
    state->TerrainLocations.UsePcf          = state->TerrainShader->UniformLocation(          "use_pcf");
    state->TerrainLocations.bias            = state->TerrainShader->UniformLocation(             "bias");

    // Добавьте mvar для отслеживания включения/выключения фильтрации PCF.
    MVar::CreateInt("use_pcf", 1);  // Включено по умолчанию.
    MVar::GetInt("use_pcf", state->UsePCF);

    EventSystem::Register(EventSystem::MVarChanged, nullptr, OnEvent);

    if (!CreateDefaultMaterial()) {
        MFATAL("Не удалось создать материал по умолчанию. Приложение не может быть продолжено.");
        return nullptr;
    } 

    if (!CreateDefaultTerrainMaterial()) {
        MFATAL("Не удалось создать материал ландшафта по умолчанию. Приложение не может продолжать работу.");
        return nullptr;
    }

    return state;
}

void MaterialSystem::Shutdown()
{
    // Отменить все материалы в массиве.
        for (u32 i = 0; i < MaxMaterialCount; ++i) {
            if (RegisteredMaterials[i].id != INVALID::ID) {
                DestroyMaterial(&RegisteredMaterials[i]);
            }
        }

        EventSystem::Unregister(EventSystem::MVarChanged, nullptr, OnEvent);

        // Удалить материал по умолчанию.
        DestroyMaterial(&DefaultMaterial);
        DestroyMaterial(&DefaultTerrainMaterial);
}

Material* MaterialSystem::AcquireReference(const char* name, bool AutoRelease, bool& NeedsCreation)
{
    Reference ref;
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
    bool condition = false;
    Reference ref;
    if (state->RegisteredMaterialTable.Get(name, &ref)) {
        condition = ref.handle != INVALID::ID;
    }

    // Загрузить конфигурацию материала из ресурса при условии, что такого материала еще не сушествует;
    Material* material = nullptr;
    MaterialResource materialResource;
    if (condition) {
        materialResource.data.name = name;
    } else {
        if (!ResourceSystem::Load(name, nullptr, materialResource)) {
            MERROR("Не удалось загрузить материальный ресурс, возвращено nullptr.");
            return nullptr;
        }
    }
    
    // Теперь получите из загруженной конфигурации.
    material = Acquire(materialResource.data);

    // Очистить
    // ResourceSystem::Unload(materialResource);

    if (!material) {
        MERROR("Не удалось загрузить материальный ресурс, возвращено nullptr.");
    } else {
        state->MaterialCount++;
    }

    return material;
}

Material *MaterialSystem::Acquire(const MaterialConfig &config)
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
        if (!state->LoadMaterial(config, material)) {
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

Material* MaterialSystem::Acquire(const char* MaterialName, u32 MaterialCount, const MString* MaterialNames, bool AutoRelease)
{
    // Верните материал ландшафта по умолчанию.
    if (MString::Equali(MaterialName, DEFAULT_TERRAIN_MATERIAL_NAME)) {
        return &state->DefaultTerrainMaterial;
    }

    bool NeedsCreation = false;
    Material* material = AcquireReference(MaterialName, AutoRelease, NeedsCreation);
    if (!material) {
        MERROR("Не удалось получить материал ландшафта '%s'", MaterialName);
        return nullptr;
    }

    if (NeedsCreation) {
        // Соберите названия материалов.
        MString* TextureNames = AllocateTemp<MString>(sizeof(MString) * MaterialCount * PBR_MATERIAL_TEXTURE_COUNT);
        MaterialResource mResource;
        for (u32 i = 0; i < MaterialCount; ++i) {
            // Загрузить конфигурацию материала из ресурса;
            if (!ResourceSystem::Load(MaterialNames[i].c_str(), nullptr, mResource)) {
                MERROR("Не удалось загрузить ресурс материала, возвращается nullptr.");
                return nullptr;
            }

            MaterialConfig& mConfig = mResource.data;
            // ПРИМЕЧАНИЕ: Пока что для ландшафтов требуются материалы PBR.
            if (mConfig.type != Material::PBR) {
                MERROR("Материалы для ландшафта должны быть PBR-материалами.");
                return nullptr;
            }

            // Извлеките названия карт.
            for (u32 j = 0; j < PBR_MATERIAL_TEXTURE_COUNT; ++j) {
                u32 index = (i * PBR_MATERIAL_TEXTURE_COUNT) + j;
                TextureNames[index] = (MString&&)mConfig.maps[j].TextureName;
            }

            // Очистите ресурс.
            mConfig.maps.Clear();
            mConfig.properties.Clear();
        }
        ResourceSystem::Unload(mResource);

        // Создайте новый материал.
        #pragma region ПРИМЕЧАНИЕ: load_material, зависящий от ландшафта
        MemorySystem::ZeroMem(material, sizeof(Material));
        MString::Copy(material->name, MaterialName, MATERIAL_NAME_MAX_LENGTH);

        Shader* SelectedShader = state->TerrainShader;
        material->ShaderID = SelectedShader->id;
        material->type = Material::Type::Terrain;

        // Выделите память для карт и свойств.
        material->PropertyStructSize = sizeof(Material::TerrainProperties);
        auto properties = (Material::TerrainProperties*)MemorySystem::Allocate(material->PropertyStructSize, Memory::MaterialInstance, true);
        material->properties = properties;
        properties->NumMaterials = MaterialCount;

        // 3 карты на материал для PBR. Выделите достаточно слотов для всех материалов. Еще одна карта для карты освещенности.
        material->maps.Resize(TERRAIN_SAMP_COUNT);

        auto rSystem = SystemsManager::GetRenderingSystem();

        // Для всего массива материалов нужна одна карта.
        {
            u32 LayerCount = PBR_MATERIAL_TEXTURE_COUNT * MAX_TERRAIN_MATERIAL_COUNT;
            TextureMap& map = material->maps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP];
            // ЗАДАЧА: Прочитать это из конфигурации.
            map.RepeatU = map.RepeatV = map.RepeatW = TextureRepeat::Repeat;
            map.FilterMinify = map.FilterMagnify = TextureFilterMode::Linear;
            map.texture = TextureSystem::AcquireTexturesAsArrayed(material->name, LayerCount, TextureNames, true);
            if (!map.texture) {
                // Настроено, но не найдено.
                MWARN("Не удается загрузить массивную текстуру '%s' для материала '%s', используется значение по умолчанию.", material->name, MaterialName);
                map.texture = TextureSystem::GetDefaultTexture(Texture::Terrain);
            }
            if (!rSystem->TextureMapAcquireResources(&map)) {
                MERROR("Не удалось получить ресурсы для текстурной карты.");
                return nullptr;
            }
        }

        // Тени нельзя настроить, поэтому настройте их здесь.
        {
            Material::Map MapConfig{};
            MapConfig.FilterMag = MapConfig.FilterMin = TextureFilterMode::Linear;
            MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::ClampToBorder;
            MapConfig.name = "shadow_map";
            MapConfig.TextureName = "";
            if (!AssignMap(material->maps[SAMP_TERRAIN_SHADOW_MAP], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Diffuse))) {
                MERROR("Не удалось назначить текстурную карту '%s' для карты теней местности.", MapConfig.name.c_str());
                return nullptr;
            }
        }

        // IBL - кубическая карта для освещенности
        {
            Material::Map MapConfig {};
            MapConfig.FilterMag = MapConfig.FilterMin = TextureFilterMode::Linear;
            MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::Repeat;
            MapConfig.name = "ibl_cube";
            //  Всегда назначается последнему индексу.
            if (!AssignMap(material->maps[SAMP_TERRAIN_IRRADIANCE_MAP], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Cube))) {
                MERROR("Не удалось назначить карту текстуры '%s' для карты освещенности местности.", MapConfig.name.c_str());
                return nullptr;
            }
        }

        // ПРИМЕЧАНИЕ: 4 материала * 3 карты на каждый всё равно будут загружены по порядку (albedo/norm/combined (met/rough/ao) на материал)
        // Следующая группа будет посвящена отображению теней
        // Последняя карта освещённости

        // Настройте конфигурацию, чтобы получить ресурсы экземпляра для этого материала.
        ShaderInstanceResourceConfig InstanceResourceConfig{
            .UniformConfigCount = 3, // Количество карт для этого типа известно. ПРИМЕЧАНИЕ: Это включает карты материалов, карты теней и карту освещенности.
            .UniformConfigs = mem_alloc(ShaderInstanceUniformTextureConfig, 3, Memory::Array)
        }; 

        // Текстуры материала (одна массивная текстура)
        u64 ptrSize = sizeof(void*);
        ShaderInstanceUniformTextureConfig& MatTextures = InstanceResourceConfig.UniformConfigs[0];
        MatTextures.UniformLocation = state->TerrainLocations.MaterialTexures;
        MatTextures.TextureMapCount = 1;
        CreateArray(MatTextures.TextureMaps, ptrSize * MatTextures.TextureMapCount);
        MatTextures.TextureMaps[0] = &material->maps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP];

        // Тени текстур
        ShaderInstanceUniformTextureConfig& ShadowTextures = InstanceResourceConfig.UniformConfigs[1];
        ShadowTextures.UniformLocation = state->TerrainLocations.ShadowTextures;
        ShadowTextures.TextureMapCount = 1;
        CreateArray(ShadowTextures.TextureMaps, ptrSize * ShadowTextures.TextureMapCount);
        ShadowTextures.TextureMaps[0] = &material->maps[SAMP_TERRAIN_SHADOW_MAP];

        // Текстура куба IBL
        ShaderInstanceUniformTextureConfig& IblCubeTexture = InstanceResourceConfig.UniformConfigs[2];
        IblCubeTexture.UniformLocation = state->TerrainLocations.IblCubeTexture;
        IblCubeTexture.TextureMapCount = 1;
        CreateArray(IblCubeTexture.TextureMaps, ptrSize * IblCubeTexture.TextureMapCount);
        IblCubeTexture.TextureMaps[0] = &material->maps[SAMP_TERRAIN_IRRADIANCE_MAP];

        // Подготовьте ресурсы
        bool result = rSystem->ShaderAcquireInstanceResources(SelectedShader, InstanceResourceConfig, material->InternalId);
        if (!result) {
            MERROR("Не удалось получить ресурсы рендеринга для материала ландшафта '%s'.", material->name);
        }

        // Очистите конфигурацию униформы.
        // InstanceResourceConfig.Destroy();

        #pragma endregion ПРИМЕЧАНИЕ: конец load_material, специфичного для местности

        if (material->generation == INVALID::ID) {
            material->generation = 0;
        } else {
            material->generation++;
        }
    }

    state->MaterialCount++;
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
    Reference ref;
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
        state->MaterialCount--;
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
    if (ShaderID == state->MaterialShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.view, &view));
        // ЗАДАЧА: установить каскадные разделения, как у направленных источников света и карты теней и т.д.
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.CascadeSplits, &AmbientColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.ViewPosition, &ViewPosition));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.RenderMode, &RenderMode));
        // Световое пространство для карт теней. На каждую каскаду
        for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i) {
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.LightSpace0 + i, &state->DirectionalLightSpace[i]));
        }
        // Глобальные параметры шейдера.
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.UsPcf, &state->UsePCF));

        // HACK: Прочитайте это откуда-то (или используйте глобальный сеттер?);
        f32 bias = 0.00005f;
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.bias, &bias));
    } else if (ShaderID == state->TerrainShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.view, &view));
        // ЗАДАЧА: установить каскадные разделения, как у направленных источников света и карты теней и т.д.
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.CascadeSplits, &AmbientColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.ViewPosition, &ViewPosition));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.RenderMode, &RenderMode));
        // Световое пространство для карт теней. На каждый каскад
        for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i) {
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.LightSpace0 + i, &state->DirectionalLightSpace[i]));
        }
        // Направленный свет - глобальный для этого шейдера.
        DirectionalLight* DirLight = ((LightSystem*)SystemsManager::GetState(SystemsManager::Light))->GetDirectionalLight();
        if (DirLight) {
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.DirLight, &DirLight->data));
        } else {
            DirectionalLight::Data data {};
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.DirLight, &data));
        }
        // Глобальные параметры шейдера.
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.UsePcf, &state->UsePCF));
        // HACK: Прочитайте это откуда-то (или используйте глобальный сеттер?);
        f32 bias = 0.00005f;
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.bias, &bias));
    } else {
        MERROR("MaterialSystem::ApplyGlobal(): Неизвестный идентификатор шейдера '%d' ", ShaderID);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyGlobal(true, rFrameData));
    // Синхронизация номера кадра
    shader->RenderFrameNumber = rFrameData.RendererFrameNumber;
    return true;
}

bool MaterialSystem::ApplyInstance(Material *material, const FrameData& rFrameData, bool NeedsUpdate)
{
    // Примените униформу на уровне экземпляра.
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::BindInstance(material->InternalId));
    if(NeedsUpdate) {
        auto pLightSystem = (LightSystem*)SystemsManager::GetState(SystemsManager::Light);
        if (material->ShaderID == state->MaterialShaderID) {
            // Шейдер материала
            // Свойства
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.properties, material->properties));
            // Карты
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSetArrayed(state->MaterialLocations.MaterialTexures, SAMP_ALBEDO,   &material->maps[SAMP_ALBEDO]));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSetArrayed(state->MaterialLocations.MaterialTexures, SAMP_NORMAL,   &material->maps[SAMP_NORMAL]));
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSetArrayed(state->MaterialLocations.MaterialTexures, SAMP_COMBINED, &material->maps[SAMP_COMBINED]));

            // Карты теней
            material->maps[SAMP_SHADOW_MAP].texture = state->ShadowTexture ? state->ShadowTexture : TextureSystem::GetDefaultTexture(Texture::Diffuse);
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.ShadowTextures, &material->maps[SAMP_SHADOW_MAP]));

            // Карта освещённости
            material->maps[SAMP_IRRADIANCE_MAP].texture = material->IrradianceTexture ? material->IrradianceTexture : state->IrradianceCubeTexture;
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.IblCubeTexture, &material->maps[SAMP_IRRADIANCE_MAP]));
            
            // Направленный свет
            auto DirLight = pLightSystem->GetDirectionalLight();
            if (DirLight) {
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DirLight, &DirLight->data));
            } else {
                DirectionalLight::Data data{};
                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.DirLight, &data));
            }
            
            // Точечный свет
            u32 PointLightCount = pLightSystem->PointLightCount();
            if (PointLightCount) {
                // ЗАДАЧА: frame allocator?
                u64 PointLightsSize = sizeof(PointLight*) * PointLightCount;
                const auto PointLights = (PointLight**)rFrameData.FrameAllocator->Allocate(PointLightsSize);
                pLightSystem->GetPointLights(PointLights);

                u64 PointLightDatasSize = sizeof(PointLight::Data) * PointLightCount;
                auto PointLightDatas = (PointLight::Data*)rFrameData.FrameAllocator->Allocate(PointLightDatasSize);
                for (u32 i = 0; i < PointLightCount; ++i) {
                    PointLightDatas[i] = PointLights[i]->data;
                }

                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.PointLights, PointLightDatas));
            }

            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->MaterialLocations.NumPointLights, &PointLightCount)); 
        } else if (material->ShaderID == state->TerrainShaderID) {
            // Примените карты материалов, все как одну многослойную текстуру.
            // material->maps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP].texture = state->ShadowTexture ? state->ShadowTexture : TextureSystem::GetDefaultTexture(Texture::Terrain);
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.MaterialTexures, &material->maps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP]));

            // ПРИМЕЧАНИЕ: применяйте другие карты отдельно.

            // Теневые карты
            material->maps[SAMP_TERRAIN_SHADOW_MAP].texture = state->ShadowTexture ? state->ShadowTexture : TextureSystem::GetDefaultTexture(Texture::Diffuse);
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.ShadowTextures, &material->maps[SAMP_TERRAIN_SHADOW_MAP]));

            // Карта освещённости
            material->maps[SAMP_TERRAIN_IRRADIANCE_MAP].texture = material->IrradianceTexture ? material->IrradianceTexture : state->IrradianceCubeTexture;
            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.IblCubeTexture, &material->maps[SAMP_TERRAIN_IRRADIANCE_MAP]));

            // Применить свойства.
            ShaderSystem::UniformSet(state->TerrainLocations.properties, material->properties);

            // Точечные источники света.
            u32 PointLightCount = pLightSystem->PointLightCount();
            if (PointLightCount) {
                u64 PointLightsSize = sizeof(PointLight*) * PointLightCount;
                const auto PointLights = (PointLight**)rFrameData.FrameAllocator->Allocate(PointLightsSize);
                pLightSystem->GetPointLights(PointLights);

                u64 PointLightDatasSize = sizeof(PointLight::Data) * PointLightCount;
                auto PointLightDatas = (PointLight::Data*)rFrameData.FrameAllocator->Allocate(PointLightDatasSize);
                for (u32 i = 0; i < PointLightCount; ++i) {
                    PointLightDatas[i] = PointLights[i]->data;
                }

                MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.PointLights, PointLightDatas));
            }

            MATERIAL_APPLY_OR_FAIL(ShaderSystem::UniformSet(state->TerrainLocations.NumPointLights, &PointLightCount));
        } else {
            MERROR("MaterialSystem::ApplyInstance(): Нераспознанный идентификатор шейдера «%d» в шейдере «%s».", material->ShaderID, material->name);
            return false;
        }
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyInstance(NeedsUpdate, rFrameData));

    return true;
}

bool MaterialSystem::ApplyLocal(Material *material, const Matrix4D &model, const FrameData& rFrameData)
{
    ShaderSystem::BindLocal();
    bool result = false;

    if (material->ShaderID == state->MaterialShaderID) {
        result = ShaderSystem::UniformSet(state->MaterialLocations.model, &model);
    } else if (material->ShaderID == state->TerrainShaderID) {
        result = ShaderSystem::UniformSet(state->TerrainLocations.model, &model);
    }
    ShaderSystem::ApplyLocal(rFrameData);

    if (!result) {
        MERROR("Неизвестный идентификатор шейдера «%d»", material->ShaderID);
        return false;
    }
    return result;
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

void MaterialSystem::SetDirectionalLightSpace(const Matrix4D &DirectionalLightSpace, u8 index)
{
    state->DirectionalLightSpace[index] = DirectionalLightSpace;
}

bool MaterialSystem::SetIrradiance(Texture *IrradianceCubeTexture)
{
    if (IrradianceCubeTexture) {
        if (IrradianceCubeTexture->type != TextureType::Cube) {
            MWARN("Для работы функции MaterialSystem::SetIrradiance требуется параметр IrradianceCubeTexture, указывающий на текстуру типа cubemap. Ничего не было сделано.");
            return false;
        }

        state->IrradianceCubeTexture = IrradianceCubeTexture;
    } else {
        // Значение null возвращает нас в состояние по умолчанию.
        state->IrradianceCubeTexture = TextureSystem::GetDefaultTexture(Texture::Cube);
    }
    return true;
}

bool MaterialSystem::SetShadowMap(Texture *ShadowTexture, u8 index)
{
    if (ShadowTexture) {
        state->ShadowTexture = ShadowTexture;
    }

    return true;
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
    if (!SystemsManager::GetRenderingSystem()->TextureMapAcquireResources(&map)) {
        MERROR("Невозможно получить ресурсы для карты текстуры.");
        return false;
    }
    return true;
}

bool MaterialSystem::CreateDefaultMaterial()
{
    auto& dMaterial = state->DefaultMaterial;
    MString::Copy(dMaterial.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    dMaterial.id = INVALID::ID;
    dMaterial.type = Material::Type::PBR;
    dMaterial.generation = INVALID::ID;
    dMaterial.PropertyStructSize = sizeof(Material::PhongProperties);
    dMaterial.properties = MemorySystem::Allocate(dMaterial.PropertyStructSize, Memory::MaterialInstance);
    auto properties = (Material::PhongProperties*)dMaterial.properties;
    properties->DiffuseColour = FVec4::One();  // белый
    properties->specular = 8.F;
    dMaterial.maps.Resize(PBR_MAP_COUNT);
    for (u32 i = 0; i < PBR_MAP_COUNT; ++i) {
        auto& map = dMaterial.maps[i];
        if (i == 0) {
            // ПРИМЕЧАНИЕ: установка режима «ближайший сосед» позволит избежать размытия шахматной доски.
            map.FilterMagnify = map.FilterMinify = TextureFilterMode::Nearest;
        }
        map.FilterMagnify = map.FilterMinify = TextureFilterMode::Linear;
        map.RepeatU = map.RepeatV = map.RepeatW = TextureRepeat::Repeat;
    }

    // Измените режим ограничения для карты теней по умолчанию на «граница».
    auto& ssm = state->DefaultMaterial.maps[SAMP_SHADOW_MAP];
    ssm.RepeatU = ssm.RepeatV = ssm.RepeatW = TextureRepeat::ClampToBorder;

    state->DefaultMaterial.maps[SAMP_ALBEDO].texture         = TextureSystem::GetDefaultTexture();
    state->DefaultMaterial.maps[SAMP_NORMAL].texture         = TextureSystem::GetDefaultTexture(Texture::Normal);
    state->DefaultMaterial.maps[SAMP_COMBINED].texture       = TextureSystem::GetDefaultTexture(Texture::Combined);
    state->DefaultMaterial.maps[SAMP_SHADOW_MAP].texture     = TextureSystem::GetDefaultTexture(Texture::Diffuse);
    state->DefaultMaterial.maps[SAMP_IRRADIANCE_MAP].texture = TextureSystem::GetDefaultTexture(Texture::Cube);

    // Настройте конфигурацию для получения ресурсов экземпляра для этого материала.
    auto& material = state->DefaultMaterial;
    ShaderInstanceResourceConfig InstanceResourceConfig{3}; // Количество карт для этого типа известно. // ПРИМЕЧАНИЕ: Это включает карты материалов, карты теней и карту освещенности.
    CreateArray(InstanceResourceConfig.UniformConfigs, sizeof(ShaderInstanceResourceConfig) * 3, true);

    // Текстуры материалов
    u32 ptrSize = sizeof(void*);
    auto& MatTextures = InstanceResourceConfig.UniformConfigs[0];
    MatTextures.UniformLocation = state->MaterialLocations.MaterialTexures;
    MatTextures.TextureMapCount = PBR_MATERIAL_TEXTURE_COUNT;
    CreateArray(MatTextures.TextureMaps, ptrSize * MatTextures.TextureMapCount);
    MatTextures.TextureMaps[SAMP_ALBEDO]   = &material.maps[SAMP_ALBEDO];
    MatTextures.TextureMaps[SAMP_NORMAL]   = &material.maps[SAMP_NORMAL];
    MatTextures.TextureMaps[SAMP_COMBINED] = &material.maps[SAMP_COMBINED];

    // Текстуры теней
    auto& ShadowTextures = InstanceResourceConfig.UniformConfigs[1];
    ShadowTextures.UniformLocation = state->MaterialLocations.ShadowTextures;
    ShadowTextures.TextureMapCount = 1;
    CreateArray(ShadowTextures.TextureMaps, ptrSize * ShadowTextures.TextureMapCount);
    ShadowTextures.TextureMaps[0] = &material.maps[SAMP_SHADOW_MAP];

    // Текстура куба IBL
    auto& IblCubeTexture = InstanceResourceConfig.UniformConfigs[2];
    IblCubeTexture.UniformLocation = state->MaterialLocations.IblCubeTexture;
    IblCubeTexture.TextureMapCount = 1;
    CreateArray(IblCubeTexture.TextureMaps, ptrSize * IblCubeTexture.TextureMapCount);
    IblCubeTexture.TextureMaps[0] = &material.maps[SAMP_IRRADIANCE_MAP];
    
    auto shader = ShaderSystem::GetShader(state->MaterialShaderID);
    if (!(SystemsManager::GetRenderingSystem())->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, dMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы средства рендеринга для материала по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    // Упорядочить унифицированные конфигурации.
    // InstanceResourceConfig.Destroy();

    // Обязательно укажите идентификатор шейдера.
    dMaterial.ShaderID = shader->id;

    return true;
}

bool MaterialSystem::CreateDefaultTerrainMaterial()
{
    auto& dtm = state->DefaultTerrainMaterial;
    dtm.id = INVALID::ID;
    dtm.type = Material::Type::Terrain;
    dtm.generation = INVALID::ID;
    MString::Copy(dtm.name, DEFAULT_TERRAIN_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);

    // По сути, это должно быть то же самое, что и материал по умолчанию, просто сопоставленный с «массивом» одного материала.
    dtm.PropertyStructSize = sizeof(Material::TerrainProperties);
    dtm.properties = MemorySystem::Allocate(dtm.PropertyStructSize, Memory::MaterialInstance);
    auto properties = (Material::TerrainProperties*)dtm.properties;
    properties->NumMaterials = MAX_TERRAIN_MATERIAL_COUNT;
    properties->materials[0].DiffuseColour = FVec4::One();  // белый
    properties->materials[0].specular = 8.F;
    dtm.maps.Resize(TERRAIN_SAMP_COUNT);

    dtm.maps[0].texture = TextureSystem::GetDefaultTexture(Texture::Default);
    dtm.maps[1].texture = TextureSystem::GetDefaultTexture(Texture::Specular);
    dtm.maps[2].texture = TextureSystem::GetDefaultTexture(Texture::Normal);

    // Массив текстур материалов.
    auto& map = dtm.maps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP];
    map.texture = TextureSystem::GetDefaultTexture(Texture::Terrain);
    // ПРИМЕЧАНИЕ: установите режим «ближайший сосед», чтобы шахматная доска не была размытой.
    map.FilterMagnify = map.FilterMinify = TextureFilterMode::Nearest;

    dtm.maps[SAMP_TERRAIN_SHADOW_MAP].texture = TextureSystem::GetDefaultTexture(Texture::Diffuse);

    // Измените режим ограничения для карты теней по умолчанию на «граница».
    auto& ssm = dtm.maps[SAMP_TERRAIN_SHADOW_MAP];
    ssm.RepeatU = ssm.RepeatV = ssm.RepeatW = TextureRepeat::ClampToBorder;

    // ПРИМЕЧАНИЕ: Для ландшафтов требуются PBR-материалы.
    // ПРИМЕЧАНИЕ: 4 материала * 3 карты на каждый будут загружены в указанном порядке (альбедо/норма/метеор/шероховатость/AO на каждый материал).
    // Следующая группа — карты теней.
    // Последняя — карта освещенности.

    // Настройте конфигурацию для получения ресурсов экземпляра для этого материала.
    ShaderInstanceResourceConfig InstanceResourceConfig{3}; // Количество карт этого типа известно.  ПРИМЕЧАНИЕ: Это включает карты материалов, карты теней и карту освещенности.
    CreateArray(InstanceResourceConfig.UniformConfigs, sizeof(ShaderInstanceResourceConfig) * 3);

    // Текстуры материалов
    u32 ptrSize = sizeof(void*);
    auto& MatTextures = InstanceResourceConfig.UniformConfigs[0];
    MatTextures.UniformLocation = state->TerrainLocations.MaterialTexures;
    MatTextures.TextureMapCount = 1;
    CreateArray(MatTextures.TextureMaps, ptrSize * MatTextures.TextureMapCount);
    MatTextures.TextureMaps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP] = &dtm.maps[SAMP_TERRAIN_MATERIAL_ARRAY_MAP];

    // Текстуры теней
    auto& ShadowTextures = InstanceResourceConfig.UniformConfigs[1];
    ShadowTextures.UniformLocation = state->TerrainLocations.ShadowTextures;
    ShadowTextures.TextureMapCount = 1;
    CreateArray(ShadowTextures.TextureMaps, ptrSize * ShadowTextures.TextureMapCount);
    ShadowTextures.TextureMaps[0] = &dtm.maps[SAMP_TERRAIN_SHADOW_MAP];

    // Текстура куба IBL
    auto& IblCubeTexture = InstanceResourceConfig.UniformConfigs[2];
    IblCubeTexture.UniformLocation = state->TerrainLocations.IblCubeTexture;
    IblCubeTexture.TextureMapCount = 1;
    CreateArray(IblCubeTexture.TextureMaps, ptrSize * IblCubeTexture.TextureMapCount);
    IblCubeTexture.TextureMaps[0] = &dtm.maps[SAMP_TERRAIN_IRRADIANCE_MAP];

    // Получить ресурсы
    auto shader = ShaderSystem::GetShader(state->TerrainShaderID);
    if (!(SystemsManager::GetRenderingSystem())->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, dtm.InternalId)) {
        MFATAL("Не удалось получить ресурсы рендерера для материала ландшафта по умолчанию. Приложение не может продолжать работу.");
    }

    // Обязательно назначьте идентификатор шейдера.
    dtm.ShaderID = shader->id;

    return true;
}

bool MaterialSystem::LoadMaterial(const MaterialConfig &config, Material *material)
{ 
    MemorySystem::ZeroMem(material, sizeof(Material));

    // Название
    MString::Copy(material->name, config.name.c_str(), MATERIAL_NAME_MAX_LENGTH);

    material->type = config.type;
    Shader* SelectedShader = nullptr;
    ShaderInstanceResourceConfig InstanceResourceConfig {};

    // Обработайте конфигурацию материала по типу.
    switch (config.type) {
        case Material::Type::PBR: {
            SelectedShader = state->MaterialShader;
            material->ShaderID = state->MaterialShaderID;
            // Специфические свойства PBR.
            const u32 PropCount = config.properties.Length();

            // Значения по умолчанию
            // ЗАДАЧА: Свойства PBR
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

            // // Карты. PBR ожидает альбедо, нормали и комбинированные карты (металличность, шероховатость, AO).
            material->maps.Resize(PBR_MAP_COUNT);
            const u32 cMapCount = config.maps.Length();
            
            // Настройка отслеживания того, какие карты назначены, а какие нет.
            bool MatMapsAssigned[PBR_MATERIAL_TEXTURE_COUNT];
            for (u32 i = 0; i < PBR_MATERIAL_TEXTURE_COUNT; ++i) {
                MatMapsAssigned[i] = false;
            }
            bool ibl_cube_assigned = false;
            const char* map_names[PBR_MATERIAL_TEXTURE_COUNT] = {"albedo", "normal", "combined"};
            Texture* default_textures[PBR_MATERIAL_TEXTURE_COUNT] = {
                TextureSystem::GetDefaultTexture(),
                TextureSystem::GetDefaultTexture(Texture::Normal),
                TextureSystem::GetDefaultTexture(Texture::Combined)
            };

            // Постарайтесь сопоставить заданные имена с именами, требуемыми для PBR-материалов.
            // Это также гарантирует правильное расположение карт.
            for (u32 i = 0; i < cMapCount; ++i) {
                bool found = false;
                for (u32 TexSlot = 0; TexSlot < PBR_MATERIAL_TEXTURE_COUNT; ++TexSlot) {
                    if (config.maps[i].name.Comparei(map_names[TexSlot])) {
                        if (!AssignMap(material->maps[TexSlot], config.maps[i], material->name, default_textures[TexSlot])) {
                            return false;
                        }
                        MatMapsAssigned[TexSlot] = true;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
                // Проверьте, является ли это настроенной кубической картой IBL.
                // ЗАДАЧА: Возможно, не стоит делать это настраиваемой картой, а лучше использовать данные сцены, полученные с помощью отражательного зонда.
                if (config.maps[i].name.Comparei("ibl_cube")) {
                    // ЗАДАЧА: Пока загружается только стандартная карта куба. Необходимо получить её из зонда.
                    if (!AssignMap(material->maps[SAMP_IRRADIANCE_MAP], config.maps[i], material->name, TextureSystem::GetDefaultTexture(Texture::Cube))) {
                        return false;
                    }
                    ibl_cube_assigned = true;
                } else {
                    // ПРИМЕЧАНИЕ: Игнорировать неожиданные карты, но предупреждать о них.
                    MWARN("Конфигурация для материала '%s' содержит карту с именем '%s', которая будет игнорироваться для PBR-материалов.", config.name.c_str(), config.maps[i].name.c_str());
                }
            }

            // Убедитесь, что все карты всегда назначаются, даже если используются только настройки по умолчанию.
            for (u32 i = 0; i < PBR_MATERIAL_TEXTURE_COUNT; ++i) {
                if (!MatMapsAssigned[i]) {
                    Material::Map MapConfig {};
                    MapConfig.FilterMag = MapConfig.FilterMin = TextureFilterMode::Linear;
                    MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::Repeat;
                    MapConfig.name = map_names[i];
                    // MapConfig.TextureName = "";
                    bool AssignResult = AssignMap(material->maps[i], MapConfig, material->name, default_textures[i]);
                    if (!AssignResult) {
                        return false;
                    }
                }
            }

            // Также убедитесь, что кубическая карта всегда назначена.
            // ЗАДАЧА: Возможно, вам не захочется, чтобы это было настраиваемой картой, а чтобы она предоставлялась сценой с отражающего зонда.
            if (!ibl_cube_assigned) {
                Material::Map MapConfig {};
                MapConfig.FilterMag = MapConfig.FilterMin = TextureFilterMode::Linear;
                MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::Repeat;
                MapConfig.name = "ibl_cube";
                // MapConfig.TextureName = "";
                if (!AssignMap(material->maps[SAMP_IRRADIANCE_MAP], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Cube))) {
                    return false;
                }
            }

            // Карты теней нельзя настроить, поэтому задайте их здесь.
            {
                Material::Map MapConfig {};
                MapConfig.FilterMag = MapConfig.FilterMin = TextureFilterMode::Linear;
                MapConfig.RepeatU = MapConfig.RepeatV = MapConfig.RepeatW = TextureRepeat::ClampToBorder;
                MapConfig.name = "shadow_map";
                // MapConfig.TextureName = "";
                if (!AssignMap(material->maps[SAMP_SHADOW_MAP], MapConfig, material->name, TextureSystem::GetDefaultTexture(Texture::Diffuse))) {
                    return false;
                }
            }

            // Соберите список указателей на текстурные карты;
            // Отправьте его рендереру для получения ресурсов.
            // Количество карт этого типа известно.
            InstanceResourceConfig.UniformConfigCount = 3;  // ПРИМЕЧАНИЕ: Сюда входят карты материалов, карты теней и карта освещенности.
            InstanceResourceConfig.UniformConfigs = (ShaderInstanceUniformTextureConfig*)MemorySystem::Allocate(sizeof(ShaderInstanceUniformTextureConfig) * InstanceResourceConfig.UniformConfigCount, Memory::Array);

            // Текстуры материалов
            u32 ptrSize = sizeof(void*);
            ShaderInstanceUniformTextureConfig& MatTextures = InstanceResourceConfig.UniformConfigs[0];
            MatTextures.UniformLocation = state->MaterialLocations.MaterialTexures;
            MatTextures.TextureMapCount = PBR_MATERIAL_TEXTURE_COUNT;
            CreateArray(MatTextures.TextureMaps, ptrSize * MatTextures.TextureMapCount);
            MatTextures.TextureMaps[SAMP_ALBEDO]   = &material->maps[SAMP_ALBEDO];
            MatTextures.TextureMaps[SAMP_NORMAL]   = &material->maps[SAMP_NORMAL];
            MatTextures.TextureMaps[SAMP_COMBINED] = &material->maps[SAMP_COMBINED];

            // Текстуры теней
            ShaderInstanceUniformTextureConfig& ShadowTextures = InstanceResourceConfig.UniformConfigs[1];
            ShadowTextures.UniformLocation = state->MaterialLocations.ShadowTextures;
            ShadowTextures.TextureMapCount = 1;
            CreateArray(ShadowTextures.TextureMaps, ptrSize * ShadowTextures.TextureMapCount);
            ShadowTextures.TextureMaps[0] = &material->maps[SAMP_SHADOW_MAP];

            // Текстура куба IBL
            ShaderInstanceUniformTextureConfig& IblCubeTexture = InstanceResourceConfig.UniformConfigs[2];
            IblCubeTexture.UniformLocation = state->MaterialLocations.IblCubeTexture;
            IblCubeTexture.TextureMapCount = 1;
            CreateArray(IblCubeTexture.TextureMaps, ptrSize * IblCubeTexture.TextureMapCount);
            IblCubeTexture.TextureMaps[0] = &material->maps[SAMP_IRRADIANCE_MAP];
                
        } break;
        case  Material::Type::Custom: {
            // Составьте список указателей на текстурные карты;
            // Отправить его в рендерер для получения ресурсов.
            // Пользовательские материалы.
            if (!config.ShaderName) {
                MERROR("Для пользовательских типов материалов требуется указать имя шейдера. Материал '%s' не загрузился.", material->name);
                return false;
            }

            SelectedShader = ShaderSystem::GetShader(config.ShaderName);
            material->ShaderID = SelectedShader->id;

            // Свойства.
            const u32 prop_count = config.properties.Length();
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
                        case ShaderUniformType::Int8:
                            data = (void*)&config.properties[i].ValueI8;
                            break;
                        case ShaderUniformType::UInt8:
                            data = (void*)&config.properties[i].ValueU8;
                            break;
                        case ShaderUniformType::Int16:
                            data = (void*)&config.properties[i].ValueI16;
                            break;
                        case ShaderUniformType::UInt16:
                            data = (void*)&config.properties[i].ValueU16;
                            break;
                        case ShaderUniformType::Int32:
                            data = (void*)&config.properties[i].ValueI32;
                            break;
                        case ShaderUniformType::UInt32:
                            data = (void*)&config.properties[i].ValueU32;
                            break;
                        case ShaderUniformType::Float32:
                            data = (void*)&config.properties[i].ValueF32;
                            break;
                        case ShaderUniformType::Float32_2:
                            data = (void*)&config.properties[i].ValueV2;
                            break;
                        case ShaderUniformType::Float32_3:
                            data = (void*)&config.properties[i].ValueV3;
                            break;
                        case ShaderUniformType::Float32_4:
                            data = (void*)&config.properties[i].ValueV4;
                            break;
                        case ShaderUniformType::Matrix4:
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
            const u32 MapCount = config.maps.Length();
            material->maps.Resize(MapCount);
            for (u32 i = 0; i < MapCount; ++i) {
                // Нет известного сопоставления, поэтому просто сопоставляйте их по порядку.
                // Недействительные текстуры будут использовать текстуру по умолчанию, поскольку тип карты неизвестен.
                if (!AssignMap(material->maps[i], config.maps[i], material->name, TextureSystem::GetDefaultTexture(Texture::Default))) {
                    return false;
                }
            }

            u32 GlobalSamplerCount   = SelectedShader->GlobalUniformSamplerCount;
            u32 InstanceSamplerCount = SelectedShader->InstanceUniformSamplerCount;

            // ПРИМЕЧАНИЕ: Порядок отображения для пользовательских материалов должен соответствовать порядку унифицированных сэмплеров, определенному в шейдере. 
            // Обработка всегда начинается с глобального сэмплера, затем переходит к экземпляру.
            InstanceResourceConfig.UniformConfigCount = GlobalSamplerCount + InstanceSamplerCount;
            InstanceResourceConfig.UniformConfigs = (ShaderInstanceUniformTextureConfig*)MemorySystem::Allocate(sizeof(ShaderInstanceUniformTextureConfig) * InstanceResourceConfig.UniformConfigCount, Memory::Array);

            // Сначала отследите количество карт, используемых глобальными униформами, а затем скорректируйте результаты с учетом этого количества.
            u32 MapOffset = 0;
            for (u32 i = 0; i < GlobalSamplerCount; ++i) {
                MapOffset++;
            }
            u32 ptrSize = sizeof(void*);
            for (u32 i = 0; i < InstanceSamplerCount; ++i) {
                ShaderUniform& u = SelectedShader->uniforms[SelectedShader->InstanceSamplerIndices[i]];
                ShaderInstanceUniformTextureConfig& UniformConfig = InstanceResourceConfig.UniformConfigs[i];
                UniformConfig.UniformLocation = u.location;
                UniformConfig.TextureMapCount = MMAX(u.ArrayLength, 1);
                UniformConfig.TextureMaps = (TextureMap**)MemorySystem::Allocate(ptrSize * UniformConfig.TextureMapCount, Memory::Array);
                for (u32 j = 0; j < UniformConfig.TextureMapCount; ++j) {
                    UniformConfig.TextureMaps[j] = &material->maps[i + MapOffset];
                }
            }
        } break;
        default: {
            MERROR("Неизвестный тип материала: %d. Материал '%s' не может быть загружен.", config.type, material->name);
            return false;
        }
    }

    // Получить ресурсы экземпляра для этого материала.
    bool result = (SystemsManager::GetRenderingSystem())->ShaderAcquireInstanceResources(SelectedShader, InstanceResourceConfig, material->InternalId);
    if (!result) {
        MERROR("Не удалось получить ресурсы рендерера для материала '%s'.", material->name);
    }

    // Очистить конфигурации униформ.
    // InstanceResourceConfig.Destroy();

    return result;
}

void DestroyMaterial(Material *material)
{
    // MTRACE("Уничтожение материала '%s'...", material->name);
    auto renderer = SystemsManager::GetRenderingSystem();
    const u32 length = material->maps.Length();
    for (u32 i = 0; i < length; ++i) {
        // Освободить ссылки на текстуры.
        if (material->maps[i].texture) {
            TextureSystem::Release(material->maps[i].texture->name);
        }
        // Освободить ресурсы карты текстур.
        renderer->TextureMapReleaseResources(&material->maps[i]);
    }

    // Освободить ресурсы рендерера.
    if (material->ShaderID != INVALID::ID && material->InternalId != INVALID::ID) {
        renderer->ShaderReleaseInstanceResources(ShaderSystem::GetShader(material->ShaderID), material->InternalId);
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

bool MaterialSystem::OnEvent(u16 code, void *sender, void *ListenerInst, const EventContext &context)
{
    if (code == EventSystem::MVarChanged) {
        if (MString::Comparei("use_pcf", context.data.c)) {
            MVar::GetInt("use_pcf", state->UsePCF);
            return true;
        }
    }

    return false;
}
