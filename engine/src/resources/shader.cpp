#include "shader.hpp"
#include "renderer/renderer.hpp"
#include "renderer/vulkan/vulkan_shader.hpp"

constexpr Shader::Shader()
    :
    id(), 
    name(),  
    RequiredUboAlignment(), 
    GlobalUboSize(), 
    GlobalUboStride(), 
    GlobalUboOffset(), 
    UboSize(), 
    UboStride(), 
    PushConstantSize(), 
    PushConstantStride(), 
    GlobalTextureMaps(), 
    InstanceTextureCount(), 
    BoundScope(), 
    BoundInstanceID(), 
    BoundUboOffset(), 
    HashtableBlock(nullptr), 
    UniformLookup(), 
    uniforms(), 
    attributes(), 
    state(), 
    PushConstantRangeCount(), 
    PushConstantRanges(), 
    AttributeStride(), 
    RenderFrameNumber(),
    ShaderData(nullptr) 
{}

Shader::Shader(u32 id, const Config &config) 
    : 
    id(id), 
    name(config.name), 
    flags(),
    RequiredUboAlignment(), 
    GlobalUboSize(), 
    GlobalUboStride(), 
    GlobalUboOffset(), 
    UboSize(), 
    UboStride(), 
    PushConstantSize(), 
    PushConstantStride(128), 
    GlobalTextureMaps(), 
    InstanceTextureCount(), 
    BoundScope(), 
    BoundInstanceID(INVALID::ID), 
    BoundUboOffset(), 
    HashtableBlock(MMemory::Allocate(1024 * sizeof(u16), Memory::Renderer, true)), 
    UniformLookup(1024, false, reinterpret_cast<u16*>(HashtableBlock), true, INVALID::U16ID), 
    uniforms(config.UniformCount), 
    attributes(), 
    state(State::NotCreated), 
    PushConstantRangeCount(), 
    PushConstantRanges(), 
    AttributeStride(), 
    RenderFrameNumber(INVALID::U64ID),
    ShaderData(nullptr)
{
    if (this->id == INVALID::ID) {
        MERROR("Невозможно найти свободный слот для создания нового шейдера. Прерывание.");
        return;
    }
}

Shader::~Shader()
{
    Renderer::Unload(this);

    // Сразу же сделайте его непригодным для использования.
    state = State::NotCreated;
}

bool Shader::Create(u32 id, const Config &config)
{
    this->id = id;
    if (this->id == INVALID::ID) {
        MERROR("Невозможно найти свободный слот для создания нового шейдера. Прерывание.");
        return false;
    }
    this->state = State::NotCreated;
    this->name = config.name;
    this->PushConstantRangeCount = 0;
    MMemory::ZeroMem(this->PushConstantRanges, sizeof(Range) * 32);
    this->BoundInstanceID = INVALID::ID;
    this->AttributeStride = 0;

    // Создайте хеш-таблицу для хранения индексов унифицированного массива. 
    // Это обеспечивает прямой индекс массива «uniforms», хранящегося в шейдере, для быстрого поиска по имени.
    u64 ElementSize = sizeof(u16);  // Индексы хранятся как u16.
    u64 ElementCount = 1024;        // Это больше униформ, чем нам когда-либо понадобится, но стол большего размера снижает вероятность столкновений.
    this->HashtableBlock = MMemory::Allocate(ElementSize * ElementCount, Memory::HashTable);
    this->UniformLookup.Create(ElementCount, false, reinterpret_cast<u16*>(this->HashtableBlock));

    // Сделайте недействительными все места в хеш-таблице.
    u16 invalid = INVALID::U16ID;
    this->UniformLookup.Fill(invalid);

    // Промежуточная сумма фактического размера объекта глобального универсального буфера.
    this->GlobalUboSize = 0;
    // Промежуточная сумма фактического размера объекта универсального буфера экземпляра.
    this->UboSize = 0;
    // ПРИМЕЧАНИЕ: Требования к выравниванию UBO установлены в серверной части средства рендеринга.

    // Это жестко запрограммировано, поскольку спецификация Vulkan гарантирует, что доступно только _минимум_ 128 байтов пространства, 
    // и драйвер должен определить, сколько доступно. Поэтому, чтобы избежать сложности, будет использоваться только наименьший общий знаменатель 128B.
    this->PushConstantStride = 128;
    this->PushConstantSize = 0;
    return true;
}

bool Shader::AddAttribute(const AttributeConfig &config)
{
    u32 size = 0;
    switch (config.type) {
        case Shader::AttributeType::Int8:
        case Shader::AttributeType::UInt8:
            size = 1;
            break;
        case Shader::AttributeType::Int16:
        case Shader::AttributeType::UInt16:
            size = 2;
            break;
        case Shader::AttributeType::Float32:
        case Shader::AttributeType::Int32:
        case Shader::AttributeType::UInt32:
            size = 4;
            break;
        case Shader::AttributeType::Float32_2:
            size = 8;
            break;
        case Shader::AttributeType::Float32_3:
            size = 12;
            break;
        case Shader::AttributeType::Float32_4:
            size = 16;
            break;
        default:
            MERROR("Неизвестный тип %d, по умолчанию имеет размер 4. Вероятно, это не то, что желательно.");
            size = 4;
            break;
    }

    AttributeStride += size;

    // Создайте/отправьте атрибут.
    attributes.EmplaceBack(config.name, config.type, size);

    return true;
}

bool Shader::AddUniform(const UniformConfig &config)
{
    if (!UniformAddStateValid() || !UniformNameValid(config.name)) {
        return false;
    }
    return UniformAdd(config.name, config.size, config.type, config.scope, 0, false);
}

bool Shader::BindGlobals()
{
    // Глобальный UBO всегда находится в начале, но все равно используйте его.
    BoundUboOffset = GlobalUboOffset;
    return true;
}

bool Shader::BindInstance(u32 InstanceID)
{
    BoundInstanceID = InstanceID;
    BoundUboOffset = ShaderData->InstanceStates[InstanceID].offset;
    return true;
}

bool Shader::UniformAdd(const MString &UniformName, u32 size, Shader::UniformType type, Shader::Scope scope, u32 SetLocation, bool IsSampler)
{
    Shader::Uniform entry{
        (u16)uniforms.Length(),                                 // Индекс сохраняется в хеш-таблице для поиска.
        IsSampler ? (u16)SetLocation : (u16)uniforms.Length(),  // Просто используйте переданное местоположение
        scope, 
        type}; 
    bool IsGlobal = (scope == Shader::Scope::Global);

    if (scope != Shader::Scope::Local) {
        entry.SetIndex = (u32)scope;
        entry.offset = IsSampler ? 0 : IsGlobal ? GlobalUboSize
                                                  : UboSize;
        entry.size = IsSampler ? 0 : size;
    } else {
        // Вставьте новый выровненный диапазон (выровняйте по 4, как того требует спецификация Vulkan).
        entry.SetIndex = INVALID::U8ID;
        Range range{PushConstantSize, size, 4};
        // использовать выровненное смещение/диапазон
        entry.offset = range.offset;
        entry.size = range.size;

        // Отслеживайте конфигурацию для использования при инициализации.
        PushConstantRanges[PushConstantRangeCount] = range;
        PushConstantRangeCount++;

        // Увеличьте размер константы push на общее значение.
        PushConstantSize += range.size;
    }

    if (!UniformLookup.Set(UniformName.c_str(), entry.index)) {
        MERROR("Не удалось добавить униформу.");
        return false;
    }
    uniforms.PushBack(entry);

    if (!IsSampler) {
        if (entry.scope == Shader::Scope::Global) {
            GlobalUboSize += entry.size;
        } else if (entry.scope == Shader::Scope::Instance) {
            UboSize += entry.size;
        }
    }

    return true;
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

bool Shader::UniformAddStateValid()
{
    if (state != Shader::State::Uninitialized) {
        MERROR("Униформы можно добавлять в шейдеры только перед инициализацией.");
        return false;
    }
    return true;
}

u16 Shader::UniformIndex(const char *UniformName)
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
    this->~Shader();
}

void Shader::Config::Clear()
{
    // name.Clear();
    CullMode = FaceCullMode::None;
    AttributeCount = 0;
    attributes.Clear();
    UniformCount = 0;                         
    uniforms.Clear();    
    // RenderpassName.Clear();                  
    StageCount = 0;                           
    stages.Clear();              
    StageNames.Clear();              
    StageFilenames.Clear();          
}
