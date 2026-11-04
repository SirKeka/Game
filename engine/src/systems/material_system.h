/// @file material_system.hpp
/// @author
/// @brief Система материалов отвечает за управление материалами в движке, включая подсчет ссылок и автоматическую выгрузку.
/// @version 1.0
/// @date
/// 
/// @copyright
#pragma once
#include "containers/hashtable.hpp"
#include "resources/terrain.h"

#define MAX_SHADOW_CASCADE_COUNT 4

struct Material;
struct MaterialConfig;
struct Matrix4D;
struct FrameData;
struct EventContext;

/// @brief Имя материала по умолчанию.
#define DEFAULT_MATERIAL_NAME "default"

/// @brief Имя материала пользовательского интерфейса по умолчанию.
#define DEFAULT_UI_MATERIAL_NAME "default_ui"

/// @brief Имя материала ландшафта по умолчанию.
#define DEFAULT_TERRAIN_MATERIAL_NAME "default_terrain"

/// @brief Конфигурация для системы материалов.
struct MaterialSystemConfig
{
    /// @brief Максимальное количество загруженных материалов.
    u32 MaxMaterialCount;
};

class MaterialSystem
{
public:
    NON_COPYABLE(MaterialSystem);

    /// @return указатель на материал по умолчанию.
    MAPI static Material* GetDefaultMaterial();

    /// @brief Получает указатель на материал пользовательского интерфейса по умолчанию. Не ссылается на счетчик.
    MAPI static Material* GetDefaultUiMaterial();

    /// @brief Получает указатель на материал ландшафта по умолчанию. Не ссылается на счетчик.
    MAPI static Material* GetDefaultTerrainMaterial();

    /// @brief Функция создает объект класса и инициализирует его
    /// @param MaxMaterialCount максимальное количество загружаемых материалов.
    /// @return true если инициализаця прошла успешно или false если нет
    static MaterialSystem* Initialize(u64& MemoryRequirement, void* memory, MaterialSystemConfig config);

    void Shutdown();

    /// @brief Пытается получить материал с заданным именем. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал найден и загружен, его счетчик ссылок увеличивается.
    /// @param name имя искомого материала.
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    MAPI static Material* Acquire(const char* name);

    /// @brief Пытается получить материал из заданной конфигурации. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал _найден и загружен, его счетчик ссылок увеличивается.
    /// @param config конфигурация загружаемого материала
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    MAPI static Material* Acquire(const MaterialConfig& config);

    /// @brief Пытается получить материал ландшафта с указанным именем. 
    /// Если он еще не загружен, это запускает его загрузку с использованием предоставленных стандартных имен материалов. 
    /// Если материал не может быть загружен, возвращается указатель на материал ландшафта по умолчанию. 
    /// Если материал _найден_ и загружен, его счетчик ссылок увеличивается.
    /// @param MaterialName имя материала ландшафта, который нужно найти.
    /// @param MaterialCount количество стандартных имен исходных материалов.
    /// @param MaterialNames имена исходных материалов, которые нужно использовать.
    /// @param AutoRelease
    /// @return указатель на загруженный материал ландшафта. Может быть указателем на материал ландшафта по умолчанию, если он не найден.
    MAPI static Material* Acquire(const char* MaterialName, u32 MaterialCount, const MString* MaterialNames, bool AutoRelease);

    MAPI static void Release(const char* name);

    /// @brief Применяет данные глобального уровня для идентификатора шейдера материала.
    /// @param ShaderID идентификатор шейдера, к которому применяются глобальные переменные.
    /// @param RenderFrameNumber текущий номер кадра рендерера.
    /// @param RendererDrawIndex текущий индекс отрисовки рендерера.
    /// @param projection константная ссылка на матрицу проекции.
    /// @param view константная ссылка на матрицу представления.
    /// @param AmbientColour окружающий цвет сцены.
    /// @param ViewPosition позиция камеры.
    /// @return true в случае успеха иначе false.
    MAPI static bool ApplyGlobal(u32 ShaderID, const FrameData& rFrameData, const Matrix4D& projection, const Matrix4D& view, const FVec4& AmbientColour = FVec4(), const FVec3& ViewPosition = FVec3(), u32 RenderMode = 0);
    
    /// @brief Применяет данные материала на уровне экземпляра для данного материала.
    /// @param material указатель на материал, который будет применен.
    /// @param rFrameData ссылка на данные текущего кадра.
    /// @return true в случае успеха иначе false.
    MAPI static bool ApplyInstance(Material* material, const FrameData& rFrameData, bool NeedsUpdate);

    /// @brief Применяет данные о материале локального уровня (обычно только матрицу модели).
    /// @param material указатель на материал, который будет применен.
    /// @param model константная ссылка на применяемую матрицу модели.
    /// @return true в случае успеха иначе false.
    MAPI static bool ApplyLocal(Material* material, const Matrix4D& model, const FrameData& rFrameData);

    /// @brief Выводит все зарегистрированные материалы и их количества/маркеры.
    MAPI void Dump();

    /// @brief Устанавливает текущую матрицу светового пространства направленного света для использования в будущих вызовах привязки, которые её требуют.
    /// @param DirectionalLightSpace Матрица направленного света в пространстве.
    MAPI static void SetDirectionalLightSpace(const Matrix4D& DirectionalLightSpace, u8 index);

    /// @brief Устанавливает указанный кубический текстурный ресурс для использования в будущих вызовах привязки/отрисовки до его изменения.
    /// NOTE: Предоставленная текстура должна быть текстурой кубической карты, иначе эта функция не сработает.
    /// @param IrradianceCubeTexture Указатель на текстуру кубической карты освещенности, которая будет использоваться. Если значение равно null, система использует текстуру кубической карты по умолчанию.
    /// @returns true в случае успеха иначе false.
    MAPI static bool SetIrradiance(Texture* IrradianceCubeTexture);

    /// @brief Устанавливает указанный текстурный объект карты теней для использования в последующих вызовах привязки/рисования до тех пор, пока он не будет изменён.
    /// @param ShadowTexture Указатель на текстуру карты теней, которая будет использоваться.
    /// @returns true в случае успеха иначе false.
    MAPI static bool SetShadowMap(Texture* ShadowTexture, u8 index);
private:
    struct Reference {
        u64 ReferenceCount;
        u32 handle;
        bool AutoRelease;
        constexpr Reference() : ReferenceCount(), handle(), AutoRelease(false) {}
        constexpr Reference(u64 ReferenceCount, u32 handle, bool AutoRelease) 
        : ReferenceCount(ReferenceCount), handle(handle), AutoRelease(AutoRelease) {}
    };

    struct ShaderUniformLocations {
        u16 projection      {INVALID::U16ID};
        u16 view            {INVALID::U16ID};
        u16 CascadeSplits   {INVALID::U16ID};
        u16 ViewPosition    {INVALID::U16ID};
        u16 properties      {INVALID::U16ID};
        u16 IblCubeTexture  {INVALID::U16ID};
        u16 MaterialTexures {INVALID::U16ID};
        u16 ShadowTextures  {INVALID::U16ID};
        u16 LightSpace0     {INVALID::U16ID};
        u16 LightSpace1     {INVALID::U16ID};
        u16 LightSpace2     {INVALID::U16ID};
        u16 LightSpace3     {INVALID::U16ID};
        u16 model           {INVALID::U16ID};
        u16 RenderMode      {INVALID::U16ID};
        u16 UsPcf           {INVALID::U16ID};
        u16 bias            {INVALID::U16ID};
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
        u16 CascadeSplits   {INVALID::U16ID};
        u16 ViewPosition    {INVALID::U16ID};
        u16 model           {INVALID::U16ID};
        u16 RenderMode      {INVALID::U16ID};
        u16 DirLight        {INVALID::U16ID};
        u16 PointLights     {INVALID::U16ID};
        u16 NumPointLights  {INVALID::U16ID};

        u16 properties      {INVALID::U16ID};
        u16 IblCubeTexture  {INVALID::U16ID};
        u16 ShadowTextures  {INVALID::U16ID};
        u16 LightSpace0     {INVALID::U16ID};
        u16 LightSpace1     {INVALID::U16ID};
        u16 LightSpace2     {INVALID::U16ID};
        u16 LightSpace3     {INVALID::U16ID};
        u16 MaterialTexures {INVALID::U16ID};
        u16 UsePcf          {INVALID::U16ID};
        u16 bias            {INVALID::U16ID};
    };

    Material DefaultMaterial;                    // Стандартный материал.
    Material DefaultUiMaterial;                  // Стандартный материал пользовательского интерфейса.
    Material DefaultTerrainMaterial;             // Стандартный материал ландшафта.
    
    Material* RegisteredMaterials;               // Массив зарегистрированных материалов.
    
    Texture* IrradianceCubeTexture;              // Текущая текстура кубической карты освещенности, которая будет использоваться.
    Texture* ShadowTexture;                      // Текущая текстура тени, которая будет использоваться при следующем рисовании.
    
    Shader* MaterialShader;
    Shader* TerrainShader;
    
    HashTable<Reference> RegisteredMaterialTable;// Хэш-таблица для поиска материалов.
    
    ShaderUniformLocations MaterialLocations;    // Известные местоположения шейдера материала.
    TerrainShaderLocations TerrainLocations;     // Известные места для шейдера ландшафта.
    // Конфигурация материала---------------------------------------------------------------------------------
    u32 MaxMaterialCount;                        // Максимальное количество загружаемых материалов.
    u32 MaterialCount;
    //--------------------------------------------------------------------------------------------------------
    u32 MaterialShaderID;
    u32 TerrainShaderID;
    i32 UsePCF;
    
    Matrix4D DirectionalLightSpace[MAX_SHADOW_CASCADE_COUNT];

    static inline MaterialSystem* state;

    /// @brief Инициализирует систему материалов при создании объекта.
    MaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, Reference* HashTableBlock);
    ~MaterialSystem() { Shutdown(); }

    static Material* AcquireReference(const char* name, bool AutoRelease, bool& NeedsCreation);
    static bool CreateDefaultMaterial();
    static bool CreateDefaultTerrainMaterial();
    bool LoadMaterial(const MaterialConfig& config, Material* m);
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, const EventContext& context);
};
