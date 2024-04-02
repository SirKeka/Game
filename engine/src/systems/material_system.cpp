#include "material_system.hpp"
#include "systems/texture_system.hpp"
#include "renderer/renderer.hpp"

#include "memory/linear_allocator.hpp"
#include <new>

u32 MaterialSystem::MaxMaterialCount = 0;
MaterialSystem* MaterialSystem::state = nullptr;

MaterialSystem::MaterialSystem()// : name(), AutoRelease(false), DiffuseMapName(), DiffuseColour(), DefaultMaterial(), RegisteredMaterials()
{
    // Блок массива находится после состояния. Уже выделено, поэтому просто установите указатель.
    u8* ArrayBlock = reinterpret_cast<u8*>(this + sizeof(MaterialSystem));
    RegisteredMaterials = reinterpret_cast<Material*>(ArrayBlock);

    // Блок хеш-таблицы находится после массива.
    u64 ArrayRequirement = sizeof(Material) * MaxMaterialCount;
    MaterialReference* HashtableBlock = reinterpret_cast<MaterialReference*>(ArrayBlock + ArrayRequirement);

    // Создайте хеш-таблицу для поиска материалов.
    RegisteredMaterialTable = HashTable<MaterialReference>(MaxMaterialCount, false, HashtableBlock);

    // Заполните хеш-таблицу недопустимыми ссылками, чтобы использовать ее по умолчанию.
    MaterialReference InvalidRef;
    InvalidRef.AutoRelease = false;
    InvalidRef.handle = INVALID_ID;  // Primary reason for needing default values.
    InvalidRef.ReferenceCount = 0;
    RegisteredMaterialTable.Fill(&InvalidRef);

    // Сделать недействительными все материалы в массиве.
    new (reinterpret_cast<void*>(RegisteredMaterials)) Material[MaxMaterialCount]();
    /*for (u32 i = 0; i < 100; ++i) { MTRACE("id%u, %u", RegisteredMaterials[i].id, i);
        // this->RegisteredMaterials[i].id = INVALID_ID;
        // this->RegisteredMaterials[i].generation = INVALID_ID;
        // this->RegisteredMaterials[i].InternalId = INVALID_ID;
        // new (reinterpret_cast<void*>(RegisteredMaterials)) Material[MaxMaterialCount];
    }*/
}

void MaterialSystem::SetMaxMaterialCount(u32 value)
{
    MaxMaterialCount = value;
}

bool MaterialSystem::Initialize()
{
    if (MaxMaterialCount == 0) {
        MFATAL("MaterialSystem::Initialize — MaxMaterialCount должен быть > 0.");
        return false;
    }

    if (!state) {
        state = new MaterialSystem();
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
        // Сделать недействительными все материалы в массиве.
        for (u32 i = 0; i < MaxMaterialCount; ++i) { //MTRACE("id%u, %u", RegisteredMaterials[i].id, i)
            if (state->RegisteredMaterials[i].id != INVALID_ID) {
                DestroyMaterial(&state->RegisteredMaterials[i]);
            }
        }

        // Уничтожьте материал по умолчанию.
        DestroyMaterial(&state->DefaultMaterial);
    }

    //delete state;
}

Material *MaterialSystem::Acquire(const char *name)
{ //for (u32 i = 71; i < 73; ++i) { MTRACE("id%u, %u", RegisteredMaterials[i].id, i);}
    // Загрузите данную конфигурацию материала с диска.
    MaterialConfig config;

    // Загрузить файл с диска
    // TODO: Должен иметь возможность находиться где угодно.
    const char* FormatStr = "assets/materials/%s.%s";
    char FullFilePath[512];

    // TODO: попробуйте разные расширения
    MString::Format(FullFilePath, FormatStr, name, "mmt");
    if (!LoadConfigurationFile(FullFilePath, &config)) {
        MERROR("Не удалось загрузить файл материала: '%s'. Нулевой указатель будет возвращен.", FullFilePath);
        return 0;
    }

    // Теперь получите из загруженной конфигурации.
    return AcquireFromConfig(config);
}

Material *MaterialSystem::AcquireFromConfig(MaterialConfig config)
{
    // Вернуть материал по умолчанию.
    if (StringsEquali(config.name, DEFAULT_MATERIAL_NAME)) {
        return &DefaultMaterial;
    }

    MaterialReference ref;
    if (RegisteredMaterialTable.Get(config.name, &ref)) {
        // Это можно изменить только при первой загрузке материала.
        if (ref.ReferenceCount == 0) {
            ref.AutoRelease = config.AutoRelease;
        } 
        ref.ReferenceCount++;
        if (ref.handle == INVALID_ID) {
            // Это означает, что здесь нет материала. Сначала найдите бесплатный индекс.
            Material* m = nullptr;
            for (u32 i = 0; i < MaxMaterialCount; ++i) {
                if (this->RegisteredMaterials[i].id == INVALID_ID) {
                    // Свободный слот найден. Используйте его индекс в качестве дескриптора.
                    ref.handle = i;
                    m = &this->RegisteredMaterials[i];
                    break;
                }
            }
            
            // Убедитесь, что пустой слот действительно найден.
            if (!m || ref.handle == INVALID_ID) {
                MFATAL("MaterialSystem::Acquire — система материалов больше не может содержать материалы. Настройте конфигурацию, чтобы разрешить больше.");
                return 0;
            }

            // Создайте новый материал.
            if (!LoadMaterial(config, m)) {
                MERROR("Не удалось загрузить материал '%s'.", config.name);
                return 0;
            }

            if (m->generation == INVALID_ID) {
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
    MERROR("MaterialSystem::AcquireFromConfig не удалось получить материал '%s'. Нулевой указатель будет возвращен.", config.name);
    return 0;
}

void MaterialSystem::Release(const char *name)
{
    
    // Игнорируйте запросы на выпуск материала по умолчанию.
    if (StringsEquali(name, DEFAULT_MATERIAL_NAME)) {
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
            ref.handle = INVALID_ID;
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

Material *MaterialSystem::GetDefaultMaterial()
{
     if (state) {
        return &state->DefaultMaterial;
    }

    MFATAL("MaterialSystem::GetDefaultMaterial вызывается перед инициализацией системы.");
    return nullptr;
}

bool MaterialSystem::CreateDefaultMaterial()
{ 
    this->DefaultMaterial.Set(
        DEFAULT_MATERIAL_NAME,
        Vector4D<f32>::One(), // белый
        TextureUse::MapDiffuse,
        TextureSystem::Instance()->GetDefaultTexture() 
    );
    this->DefaultMaterial.InternalId = 0;
    
    if (!Renderer::CreateMaterial(&this->DefaultMaterial)) {
        MFATAL("Не удалось получить ресурсы средства рендеринга для текстуры по умолчанию. Приложение не может быть продолжено.");
        return false;
    }

    return true;
}

bool MaterialSystem::LoadMaterial(MaterialConfig config, Material *m)
{ 
    MMemory::ZeroMem(m, sizeof(Material));

    // имя
    MString::nCopy(m->name, config.name, MATERIAL_NAME_MAX_LENGTH);

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

    // TODO: другие карты

    // Отправьте его рендереру для получения ресурсов.
    if (!Renderer::CreateMaterial(m)) {
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
    Renderer::DestroyMaterial(m);

    // Обнулить это, сделать удостоверения недействительными.
    m->Destroy();
    /*MMemory::ZeroMem(m, sizeof(Material));
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->InternalId = INVALID_ID;
    m = nullptr;*/
}

bool MaterialSystem::LoadConfigurationFile(const char *path, MaterialConfig *OutConfig)
{
    FileHandle f;
    if (!Filesystem::Open(path, FILE_MODE_READ, false, &f)) {
        MERROR("«LoadConfigurationFile» - невозможно открыть файл материала для чтения: '%s'.", path);
        return false;
    }

    // Прочтите каждую строку файла.
    char LineBuf[512] = "";
    char* p = &LineBuf[0];
    u64 LineLength = 0;
    u32 LineNumber = 1;
    while (Filesystem::ReadLine(&f, 511, &p, &LineLength)) {
        // Обрежьте строку.
        char* trimmed = MString::Trim(LineBuf);

        // Получите обрезанную длину.
        LineLength = MString::Length(trimmed);

        // Пропускайте пустые строки и комментарии.
        if (LineLength < 1 || trimmed[0] == '#') {
            LineNumber++;
            continue;
        }

        // Разделить на var/value
        i32 EqualIndex = MString::IndexOf(trimmed, '=');
        if (EqualIndex == -1) {
            MWARN("В файле обнаружена потенциальная проблема с форматированием '%s': '=' токен не найден. Пропуская линию %ui.", path, LineNumber);
            LineNumber++;
            continue;
        }

        // Предположим, что имя переменной содержит не более 64 символов.
        char RawVarName[64] {};
        MString::Mid(RawVarName, trimmed, 0, EqualIndex);
        char* TrimmedVarName = MString::Trim(RawVarName);

        // Предположим, что максимальная длина значения, учитывающего имя переменной и знак «=", составляет 511–65 (446).
        char RawValue[446] {};
        MString::Mid(RawValue, trimmed, EqualIndex + 1, -1);  // Прочтите остальную часть строки
        char* TrimmedValue = MString::Trim(RawValue);

        // Обработайте переменную.
        if (StringsEquali(TrimmedVarName, "version")) {
            // TODO: версия
        } else if (StringsEquali(TrimmedVarName, "name")) {
            MString::nCopy(OutConfig->name, TrimmedValue, MATERIAL_NAME_MAX_LENGTH);
        } else if (StringsEquali(TrimmedVarName, "diffuse_map_name")) {
            MString::nCopy(OutConfig->DiffuseMapName, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
        } else if (StringsEquali(TrimmedVarName, "diffuse_colour")) {
            // Анализ цвета
            if (!MString::ToVector4D(TrimmedValue, &OutConfig->DiffuseColour)) {
                MWARN("Ошибка анализа DiffuseColour в файле '%s'. Вместо этого используется белый цвет по умолчанию.", path);
                OutConfig->DiffuseColour = Vector4D<f32>::One();  // белый
            }
        }

        // TODO: больше полей.

        // Очистите буфер строк.
        MMemory::ZeroMem(LineBuf, sizeof(char) * 512);
        LineNumber++;
    }

    Filesystem::Close(&f);

    return true;
}

void *MaterialSystem::operator new(u64 size)
{
    // Блок памяти будет содержать структуру состояния, затем блок массива, затем блок хеш-таблицы.
    u64 ArrayRequirement = sizeof(Material) * MaxMaterialCount;
    u64 HashtableRequirement = sizeof(MaterialReference) * MaxMaterialCount;
    return LinearAllocator::Instance().Allocate(size + ArrayRequirement + HashtableRequirement);
}
