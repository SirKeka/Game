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
DefaultMaterial(DEFAULT_MATERIAL_NAME, Vector4D<f32>::One(), TextureUse::MapDiffuse, TextureSystem::Instance()->GetDefaultTexture()), 
RegisteredMaterials(new(RegisteredMaterials) Material[MaxMaterialCount]()),
RegisteredMaterialTable(MaxMaterialCount, false, HashTableBlock), 
MaterialShaderID(INVALID::ID), 
UI_ShaderID(INVALID::ID)
{
    // Блок массива находится после состояния. Уже выделено, поэтому просто установите указатель.
    // u8* ArrayBlock = reinterpret_cast<u8*>(this) + sizeof(MaterialSystem);
    // RegisteredMaterials = reinterpret_cast<Material*>(ArrayBlock);

    // Блок хеш-таблицы находится после массива.
    // u64 ArrayRequirement = sizeof(Material) * MaxMaterialCount;
    // MaterialReference* HashtableBlock = reinterpret_cast<MaterialReference*>(ArrayBlock + ArrayRequirement);

    // Создайте хеш-таблицу для поиска материалов.
    // RegisteredMaterialTable = HashTable<MaterialReference>(MaxMaterialCount, false, HashtableBlock);

    // Заполните хеш-таблицу недопустимыми ссылками, чтобы использовать ее по умолчанию.
    // MaterialReference InvalidRef{0, INVALID::ID, false}; // InvalidRef.handle = INVALID::ID; Основная причина необходимости использования значений по умолчанию.
    RegisteredMaterialTable.Fill(MaterialReference(0, INVALID::ID, false));

    // Сделать недействительными все материалы в массиве.
    // new (RegisteredMaterials) Material[MaxMaterialCount]();
    /*for (u32 i = 0; i < MaxMaterialCount; ++i) { //MTRACE("id%u, %u", RegisteredMaterials[i].id, i);
        // this->RegisteredMaterials[i].id = INVALID::U32ID;
        // this->RegisteredMaterials[i].generation = INVALID::U32ID;
        // this->RegisteredMaterials[i].InternalId = INVALID::U32ID;
        //this->RegisteredMaterials[i].Init();
    }*/
    // TODO: массив объектов материала инициализируется правильно, 
    // но в последствии появляется не правильно инициализированные объекты

    /*MaterialLocations.DiffuseColour = INVALID::U16ID;
    MaterialLocations.DiffuseTexture = INVALID::U16ID;

    UI_Locations.DiffuseColour = INVALID::U16ID;
    UI_Locations.DiffuseTexture = INVALID::U16ID;*/
}

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
        void* ptrMatSys = LinearAllocator::Instance().Allocate(MemoryRequirement);
        Material* RegisteredMaterials = reinterpret_cast<Material*>(reinterpret_cast<u8*>(ptrMatSys) + sizeof(MaterialSystem));
        MaterialReference* HashTableBlock = reinterpret_cast<MaterialReference*>(reinterpret_cast<u8*>(RegisteredMaterials) + sizeof(Material) * MaxMaterialCount);
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
        delete state;
    }
}

Material *MaterialSystem::Acquire(const char *name)
{ 
    
    /* Отложенная инициализация массива материалов, т.к. после инициализации геометрической системы сбивается инициализация некоторых объектов
    if (!init) {
        new (reinterpret_cast<void*>(RegisteredMaterials)) Material[MaxMaterialCount]();
        init = true;
    }*/

    // Загрузить конфигурацию материала из ресурса.
    Resource MaterialResource;
    if (!ResourceSystem::Instance()->Load(name, ResourceType::Material, MaterialResource)) {
        MERROR("Не удалось загрузить ресурс материала, возвращается значение nullptr.");
        return nullptr;
    }
    
    Material* m;
    if (MaterialResource.data) {
        m = Acquire(*reinterpret_cast<MaterialConfig*>(MaterialResource.data));
    }

    // Clean up
    ResourceSystem::Instance()->Unload(MaterialResource);

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
                if (this->RegisteredMaterials[i].id == INVALID::ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    m = &this->RegisteredMaterials[i];
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

            // Получите единые индексы.
            Shader* s = ShaderSystem::GetInstance()->GetShader(m->ShaderID);
            // Сохраните местоположения известных типов для быстрого поиска.
            if (MaterialShaderID == INVALID::ID && config.ShaderName == BUILTIN_SHADER_NAME_MATERIAL) {
                MaterialShaderID = s->id;
                MaterialLocations.projection = ShaderSystem::GetInstance()->UniformIndex(s, "projection");
                MaterialLocations.view = ShaderSystem::GetInstance()->UniformIndex(s, "view");
                MaterialLocations.DiffuseColour = ShaderSystem::GetInstance()->UniformIndex(s, "diffuse_colour");
                MaterialLocations.DiffuseTexture = ShaderSystem::GetInstance()->UniformIndex(s, "diffuse_texture");
                MaterialLocations.model = ShaderSystem::GetInstance()->UniformIndex(s, "model");
            } else if (UI_ShaderID == INVALID::ID && config.ShaderName == BUILTIN_SHADER_NAME_UI) {
                UI_ShaderID = s->id;
                UI_Locations.projection = ShaderSystem::GetInstance()->UniformIndex(s, "projection");
                UI_Locations.view = ShaderSystem::GetInstance()->UniformIndex(s, "view");
                UI_Locations.DiffuseColour = ShaderSystem::GetInstance()->UniformIndex(s, "diffuse_colour");
                UI_Locations.DiffuseTexture = ShaderSystem::GetInstance()->UniformIndex(s, "diffuse_texture");
                UI_Locations.model = ShaderSystem::GetInstance()->UniformIndex(s, "model");
            }

            if (m->generation == INVALID::ID) {
                m->generation = 0;
            } else {
                m->generation++;
            }

            // Также используйте дескриптор в качестве идентификатора материала.
            m->id = ref.handle;
            MTRACE("Материал '%s' еще не существует. Создано, и теперь RefCount %i.", config.name, ref.ReferenceCount);
        } else {
            MTRACE("Материал '%s' уже существует, RefCount увеличен до %i.", config.name, ref.ReferenceCount);
        }

        // Обновите запись.
        RegisteredMaterialTable.Set(config.name, &ref);
        return &this->RegisteredMaterials[ref.handle];
    }

    // ПРИМЕЧАНИЕ. Это произойдет только в том случае, если что-то пойдет не так с состоянием.
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
            MTRACE("Выпущенный материал '%s'., Материал выгружен, поскольку количество ссылок = 0 и AutoRelease = true.", name);
        } else {
            MTRACE("Выпущенный материал '%s', теперь имеет счетчик ссылок '%i' (AutoRelease=%s).", name, ref.ReferenceCount, ref.AutoRelease ? "true" : "false");
        }

        // Обновите запись.
        RegisteredMaterialTable.Set(name, &ref);
    } else {
        MERROR("MaterialSystem::Release не удалось выпустить материал '%s'.", name);
    }
    
}

#define MATERIAL_APPLY_OR_FAIL(expr)                        \
    if (!expr) {                                            \
        MERROR("Не удалось применить материал: %s", expr);  \
        return false;                                       \
    }

bool MaterialSystem::ApplyGlobal(u32 ShaderID, const Matrix4D &projection, const Matrix4D &view)
{
    if (ShaderID == state->MaterialShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->MaterialLocations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->MaterialLocations.view, &view));
    } else if (ShaderID == state->UI_ShaderID) {
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->UI_Locations.projection, &projection));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->UI_Locations.view, &view));
    } else {
        MERROR("MaterialSystem::ApplyGlobal(): Неизвестный идентификатор шейдера '%d' ", ShaderID);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->ApplyGlobal());
    return true;
}

bool MaterialSystem::ApplyInstance(Material *material)
{
    // Примените униформу на уровне экземпляра.
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->BindInstance(material->InternalId));
    if (material->ShaderID == state->MaterialShaderID) {
        // Шейдер материала
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->MaterialLocations.DiffuseColour, &material->DiffuseColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->MaterialLocations.DiffuseTexture, material->DiffuseMap.texture));
    } else if (material->ShaderID == state->UI_ShaderID) {
        // шейдер пользовательского интерфейса
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->UI_Locations.DiffuseColour, &material->DiffuseColour));
        MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->UniformSet(state->UI_Locations.DiffuseTexture, material->DiffuseMap.texture));
    } else {
        MERROR("MaterialSystem::ApplyInstance(): Нераспознанный идентификатор шейдера «%d» в шейдере «%s».", material->ShaderID, material->name);
        return false;
    }
    MATERIAL_APPLY_OR_FAIL(ShaderSystem::GetInstance()->ApplyInstance());

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
        return ShaderSystem::GetInstance()->UniformSet(state->MaterialLocations.model, &model);
    } else if (material->ShaderID == state->UI_ShaderID) {
        return ShaderSystem::GetInstance()->UniformSet(state->UI_Locations.model, &model);
    }

    MERROR("Неизвестный идентификатор шейдера «%d»", material->ShaderID);
    return false;
}

bool MaterialSystem::CreateDefaultMaterial()
{ 
    // this->DefaultMaterial.Set(
    //     DEFAULT_MATERIAL_NAME,
    //     Vector4D<f32>::One(), // белый
    //     TextureUse::MapDiffuse,
    //     TextureSystem::Instance()->GetDefaultTexture() 
    // );

    Shader* s = ShaderSystem::GetInstance()->GetShader(BUILTIN_SHADER_NAME_MATERIAL);
    if (!Renderer::ShaderAcquireInstanceResources(s, DefaultMaterial.InternalId)) {
        MFATAL("Не удалось получить ресурсы средства рендеринга для материала по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    return true;
}

bool MaterialSystem::LoadMaterial(const MaterialConfig &config, Material *m)
{ 
    MMemory::ZeroMem(m, sizeof(Material));

    // имя
    MString::nCopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    m->ShaderID = ShaderSystem::GetInstance()->GetID(config.ShaderName);

    // Рассеянный цвет
    m->DiffuseColour = config.DiffuseColour;

    // Diffuse map
    if (MString::Length(config.DiffuseMapName) > 0) {
        m->DiffuseMap.use = TextureUse::MapDiffuse;
        m->DiffuseMap.texture = TextureSystem::Instance()->Acquire(config.DiffuseMapName, true);
        if (!m->DiffuseMap.texture) {
            MWARN("Невозможно загрузить текстуру '%s' для материала '%s', используя значение по умолчанию.", config.DiffuseMapName, m->name);
            m->DiffuseMap.texture = TextureSystem::Instance()->GetDefaultTexture();
        }
    } else {
        // ПРИМЕЧАНИЕ. Устанавливается только для ясности, поскольку вызов MMemory::ZeroMem выше уже делает это.
        m->DiffuseMap.use = TextureUse::Unknown;
        m->DiffuseMap.texture = nullptr;
    }

    // СДЕЛАТЬ: другие карты

    // Отправьте его рендереру для получения ресурсов.
    Shader* s = ShaderSystem::GetInstance()->GetShader(config.ShaderName);
    if (!s) {
        MERROR("Невозможно загрузить материал, поскольку его шейдер не найден: «%s». Вероятно, это проблема с ассетом материала.", config.ShaderName.c_str());
        return false;
    }
    if(Renderer::ShaderAcquireInstanceResources(s, m->InternalId)) {
        MERROR("Не удалось получить ресурсы средства визуализации для материала '%s'.", m->name);
        return false;
    }

    return true;
}

void MaterialSystem::DestroyMaterial(Material *m)
{
    //MTRACE("Уничтожение материала '%s'...", m->name);

    // Выпустите ссылки на текстуры.
    if (m->DiffuseMap.texture) {
        TextureSystem::Instance()->Release(m->DiffuseMap.texture->name);
    }

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