#include "shader.h"
#include "core/systems_manager.h"
#include "renderer/renderer_utils.h"
#include "renderer/rendering_system.h"
#include "systems/texture_system.h"
#include "systems/shader_system.h"

static bool UniformAddStateValid(ShaderState state);

Shader::Shader(u32 id, ShaderConfig &config) : id(id), flags(config.flags), name(static_cast<MString&&>(config.name)), 
    uniforms(config.uniforms.Length()), HashtableBlock(MemorySystem::Allocate(1023 * sizeof(u16), Memory::Renderer, true)), 
    UniformLookup(1024, false, reinterpret_cast<u16*>(HashtableBlock), true, INVALID::U16ID),
    LocalUboStride(128), RenderFrameNumber(INVALID::U64ID), BoundInstanceID(INVALID::ID) {}

Shader::~Shader()
{
    Destroy();
}

bool Shader::Create(u32 id, ShaderConfig &config)
{
    this->id = id;
    if (this->id == INVALID::ID) {
        MERROR("Невозможно найти свободный слот для создания нового шейдера. Прерывание.");
        return false;
    }
    name = (MString&&)config.name;

    // MemorySystem::ZeroMem(PushConstantRanges, sizeof(Range) * 32);
    BoundInstanceID = INVALID::ID;

    // Создайте хеш-таблицу для хранения индексов унифицированного массива. 
    // Это обеспечивает прямой индекс массива «uniforms», хранящегося в шейдере, для быстрого поиска по имени.
    u64 ElementSize = sizeof(u16);  // Индексы хранятся как u16.
    u64 ElementCount = 1023;        // Это больше униформ, чем нам когда-либо понадобится, но таблица большего размера снижает вероятность столкновений.
    HashtableBlock = MemorySystem::Allocate(ElementSize * ElementCount, Memory::HashTable);
    UniformLookup.Create(ElementCount, false, reinterpret_cast<u16*>(HashtableBlock));

    // Сделайте недействительными все места в хеш-таблице.
    u16 invalid = INVALID::U16ID;
    UniformLookup.Fill(invalid);

    LocalUboStride = 128;

    // Промежуточная сумма фактического размера объекта глобального универсального буфера.
    // GlobalUboSize = 0;
    // Промежуточная сумма фактического размера объекта универсального буфера экземпляра.
    // UboSize = 0;
    // ПРИМЕЧАНИЕ: Требования к выравниванию UBO установлены в серверной части средства рендеринга.

    return true;
}

bool Shader::AddAttribute(ShaderAttributeConfig &config)
{
    u32 size = 0;
    switch (config.type) {
        case ShaderAttribute::Int8:
        case ShaderAttribute::UInt8:
            size = 1;
            break;
        case ShaderAttribute::Int16:
        case ShaderAttribute::UInt16:
            size = 2;
            break;
        case ShaderAttribute::Float32:
        case ShaderAttribute::Int32:
        case ShaderAttribute::UInt32:
            size = 4;
            break;
        case ShaderAttribute::Float32_2:
            size = 8;
            break;
        case ShaderAttribute::Float32_3:
            size = 12;
            break;
        case ShaderAttribute::Float32_4:
            size = 16;
            break;
        default:
            MERROR("Неизвестный тип %d, по умолчанию имеет размер 4. Вероятно, это не то, что желательно.");
            size = 4;
            break;
    }

    AttributeStride += size;

    // Создайте/отправьте атрибут.
    attributes.EmplaceBack(config.name, size, config.type);

    return true;
}

bool Shader::AddSampler(const ShaderUniformConfig &config)
{
    // Образцы нельзя использовать для push-констант.
    if (config.scope == ShaderScope::Local) {
        MERROR("Shader::AddSampler невозможно добавить сэмплер в локальной области.");
        return false;
    }

    // Убедитесь, что имя действительно и уникально.
    if (!UniformNameValid(config.name) || !UniformAddStateValid(state)) {
        return false;
    }

    // Если глобальный, вставьте в глобальный список.
    u32 location = 0;
    if (config.scope == ShaderScope::Global) {
        u32 GlobalTextureCount = GlobalTextureMaps.Length();
        if (GlobalTextureCount + 1 > ShaderSystem::GetState()->MaxGlobalTextures) {
            MERROR("Глобальное количество текстур шейдера %i превышает максимальное значение %i", GlobalTextureCount, ShaderSystem::GetState()->MaxGlobalTextures);
            return false;
        }
        location = GlobalTextureCount;
        // ПРИМЕЧАНИЕ: создание карты текстур по умолчанию, которая будет использоваться здесь. Всегда можно обновить позже.
        // Выделите указатель, назначьте текстуру и вставьте ее в глобальные карты текстур.
        // ПРИМЕЧАНИЕ: Это распределение выполняется только для глобальных карт текстур.
        auto DefaultMap = new TextureMap();
        DefaultMap->texture = TextureSystem::GetDefaultTexture(Texture::Default);
        if (!SystemsManager::GetRenderingSystem()->TextureMapAcquireResources(DefaultMap)) {
            MERROR("Не удалось получить ресурсы для глобальной карты текстур во время создания шейдера.");
            return false;
        }
        GlobalTextureMaps.PushBack(DefaultMap);
    } else {
        // В противном случае это происходит на уровне экземпляра, поэтому подсчитывайте, сколько ресурсов необходимо добавить во время получения ресурса.
        if (InstanceTextureCount + 1 > ShaderSystem::GetState()->MaxInstanceTextures) {
            MERROR("Количество текстур экземпляра шейдера %i превышает максимальное значение %i", InstanceTextureCount, ShaderSystem::GetState()->MaxInstanceTextures);
            return false;
        }
        location = InstanceTextureCount;
        InstanceTextureCount++;
    }

    // Относитесь к этому как к униформе. ПРИМЕЧАНИЕ: В случае сэмплеров OutLocation используется для непосредственного определения 
    // значения поля «location» записи хэш-таблицы, а затем ему присваивается индекс универсального массива. Это позволяет осуществлять 
    // поиск местонахождения сэмплеров, как если бы они были униформой (поскольку технически это так и есть). 
    // ЗАДАЧА: возможно, придется сохранить это в другом месте
    if (!UniformAdd(config, location)) {
        MERROR("Невозможно добавить форму сэмплера.");
        return false;
    }

    return true;
}

bool Shader::AddUniform(const ShaderUniformConfig &config)
{
    if (!UniformAddStateValid(state) || !UniformNameValid(config.name)) {
        return false;
    }
    return UniformAdd(config, 0);
}

bool Shader::BindGlobals()
{
    // Глобальный UBO всегда находится в начале, но все равно используйте его.
    BoundUboOffset = GlobalUboOffset;
    return true;
}

ShaderUniform* Shader::GetUniform(const char *name)
{
    if (!name) { return nullptr; }

    u16 UniformIndex;
    if (!UniformLookup.Get(name, &UniformIndex)) {
        MERROR("Шейдер '%s' не содержит униформу с именем '%s'.", this->name.c_str(), name);
        return nullptr;
    }

    return &uniforms[UniformIndex];
}

bool Shader::Reload()
{
    return SystemsManager::GetRenderingSystem()->ShaderReload(this);
}

bool Shader::SetWireframe(bool WireframeEnabled)
{
    // Отключение поддерживается всегда, поскольку по сути это операция, не выполняющая никаких действий (no-op).
    if (!WireframeEnabled) {
        IsWireframe = false;
        return true;
    }

    return SystemsManager::GetRenderingSystem()->ShaderSetWireframe(this, WireframeEnabled);
}

bool Shader::UniformAdd(const ShaderUniformConfig& config, u32 location)
{
    if (!UniformAddStateValid(state) || !UniformNameValid(config.name)) {
        return false;
    }

    u32 UniformCount = uniforms.Length();
    if (UniformCount + 1 > ShaderSystem::GetState()->MaxUniformCount) {
        MERROR("Шейдер может принимать максимум %d униформ и сэмплеров в глобальной, экземпляровой и локальной областях видимости.", ShaderSystem::GetState()->MaxUniformCount);
        return false;
    }

    bool IsSampler = UniformTypeIsSampler(config.type);
    ShaderUniform entry {
        .ArrayLength = config.ArrayLength,
        .index = (u16)UniformCount,                           // Индекс сохраняется в хеш-таблице для поиска.
        .location = IsSampler ? (u16)location : entry.index,  // Просто используйте переданное местоположение
        .scope = config.scope,
        .type = config.type
    };

    bool IsGlobal = (config.scope == ShaderScope::Global);

    if (config.scope == ShaderScope::Local) {
        entry.SetIndex = 2;
        entry.offset = LocalUboSize;
        entry.size = config.size;
    } else {
        entry.SetIndex = (u8)config.scope;
        entry.offset = IsSampler ? 0 : IsGlobal ? GlobalUboSize : UboSize;
        entry.size   = IsSampler ? 0 : config.size;
    }

    if (!UniformLookup.Set(config.name.c_str(), entry.index)) {
        MERROR("Не удалось добавить униформу.");
        return false;
    }
    uniforms.PushBack(entry);

    if (!IsSampler) {
        if (entry.scope == ShaderScope::Global) {
            GlobalUboSize += (entry.size * entry.ArrayLength);
        } else if (entry.scope == ShaderScope::Instance) {
            UboSize += (entry.size * entry.ArrayLength);
        } else if(entry.scope == ShaderScope::Local) {
            LocalUboSize += (entry.size * entry.ArrayLength);
        }
    }

    return true;
}

u16 Shader::UniformLocation(const char *UniformName)
{
    if (id == INVALID::ID) {
        MERROR("ShaderSystem::UniformIndex вызывается с недопустимым шейдером.");
        return INVALID::U16ID;
    }

    u16 index = INVALID::U16ID;
    if (!UniformLookup.Get(UniformName, &index) || index == INVALID::U16ID) {
        MERROR("Шейдер '%s' не имеет зарегистрированной униформы с именем '%s'", name.c_str(), UniformName);
        return INVALID::U16ID;
    }
    return uniforms[index].index;
}

void Shader::Destroy()
{
    SystemsManager::GetRenderingSystem()->ShaderDestroy(this);

    MemorySystem::ZeroMem(this, sizeof(Shader));
}

void ShaderConfig::Clear()
{
    name.Destroy();
    CullMode = FaceCullMode::None;
    TopologyTypes = 1;
    attributes.Destroy();                       
    uniforms.Destroy();
    StageConfigs.Destroy();                
    flags = 0;
}

bool Shader::UniformNameValid(const MString &UniformName)
{
    if (!UniformName) {
        MERROR("Единое имя должно существовать.");
        return false;
    }
    u16 location;
    if (UniformLookup.Get(UniformName.c_str(), &location) && location != INVALID::U16ID) {
        MERROR("Униформа с именем «%s» уже существует в шейдере «%s».", UniformName.c_str(), name.c_str());
        return false;
    }
    return true;
}

bool UniformAddStateValid(ShaderState state)
{
    if (state != ShaderState::Uninitialized) {
        MERROR("Униформы можно добавлять в шейдеры только до инициализации.");
        return false;
    }
    return true;
}
