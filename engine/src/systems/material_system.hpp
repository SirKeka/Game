/**
 * @file material_system.hpp
 * @author
 * @brief Система материалов отвечает за управление материалами в движке, включая подсчет ссылок и автоматическую выгрузку.
 * @version 1.0
 * @date
 * 
 * @copyright
 * 
 */
#pragma once
#include "resources/material/material.hpp"
#include "containers/hashtable.hpp"
class Matrix4D;

constexpr const char* DEFAULT_MATERIAL_NAME = "default";    // Имя материала по умолчанию.

class MaterialSystem
{
private:
    // Конфигурация материала---------------------------------------------------------------------------------
    u32 MaxMaterialCount;                                   // Максимальное количество загружаемых материалов.
    //--------------------------------------------------------------------------------------------------------
    Material DefaultMaterial;                               // Стандартный материал.
    Material* RegisteredMaterials;                          // Массив зарегистрированных материалов.

    struct MaterialReference {
        u64 ReferenceCount;
        u32 handle;
        bool AutoRelease;
        constexpr MaterialReference() : ReferenceCount(), handle(), AutoRelease(false) {}
        constexpr MaterialReference(u64 ReferenceCount, u32 handle, bool AutoRelease) 
        : ReferenceCount(ReferenceCount), handle(handle), AutoRelease(AutoRelease) {}
    };
    
    HashTable<MaterialReference> RegisteredMaterialTable;   // Хэш-таблица для поиска материалов.

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
    } MaterialLocations;                                    // Известные местоположения шейдера материала.
    u32 MaterialShaderID;

    struct UI_ShaderUniformLocations {
        u16 projection      {INVALID::U16ID};
        u16 view            {INVALID::U16ID};
        u16 DiffuseColour   {INVALID::U16ID};
        u16 DiffuseTexture  {INVALID::U16ID};
        u16 model           {INVALID::U16ID};
    } UI_Locations;
    u32 UI_ShaderID;

    MAPI static MaterialSystem* state;                       // Экземпляр системы материалов (синглтон)

    /// @brief Инициализирует систему материалов при создании объекта.
    constexpr MaterialSystem() : MaxMaterialCount(), DefaultMaterial(), RegisteredMaterials(nullptr), RegisteredMaterialTable(), MaterialLocations(), MaterialShaderID(), UI_Locations(), UI_ShaderID() {}
    MaterialSystem(u32 MaxMaterialCount, Material* RegisteredMaterials, MaterialReference* HashTableBlock);
public:
    ~MaterialSystem();
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator= (const MaterialSystem&) = delete;

    // Методы основного класса------------------------------------------------------------------------------------------------------

    /// @brief Предоставляет доступ к экземпляру объекта системы материалов.
    /// @return Указатель на систему материалов.
    MAPI static MINLINE MaterialSystem* Instance() { /*if(state) */return state; }
    MAPI static Material* GetDefaultMaterial();

    /// @brief Функция создает объект класса и инициализирует его
    /// @param MaxMaterialCount максимальное количество загружаемых материалов.
    /// @return true если инициализаця прошла успешно или false если нет
    static bool Initialize(u32 MaxMaterialCount, class LinearAllocator& SystemAllocator);
    static void Shutdown();
    //-------------------------------------------------------------------------------------------------------------------------------

    // Внутренние методы-------------------------------------------------------------------------------------------------------------

    /// @brief Пытается получить материал с заданным именем. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал найден и загружен, его счетчик ссылок увеличивается.
    /// @param name имя искомого материала.
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    MAPI Material* Acquire(const char* name);
    /// @brief Пытается получить материал из заданной конфигурации. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал _найден и загружен, его счетчик ссылок увеличивается.
    /// @param config конфигурация загружаемого материала
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    MAPI Material* Acquire(const MaterialConfig& config);
    MAPI void Release(const char* name);
    /// @brief Применяет данные глобального уровня для идентификатора шейдера материала.
    /// @param ShaderID идентификатор шейдера, к которому применяются глобальные переменные.
    /// @param RenderFrameNumber текущий номер кадра рендерера.
    /// @param projection константная ссылка на матрицу проекции.
    /// @param view константная ссылка на матрицу представления.
    /// @param AmbientColour окружающий цвет сцены.
    /// @param ViewPosition позиция камеры.
    /// @return true в случае успеха иначе false.
    MAPI bool ApplyGlobal(u32 ShaderID, u64 RenderFrameNumber, const Matrix4D& projection, const Matrix4D& view, const FVec4& AmbientColour = FVec4(), const FVec4& ViewPosition = FVec4(), u32 RenderMode = 0);
    /// @brief Применяет данные материала на уровне экземпляра для данного материала.
    /// @param material указатель на материал, который будет применен.
    /// @return true в случае успеха иначе false.
    MAPI bool ApplyInstance(Material* material, bool NeedsUpdate);
    /// @brief Применяет данные о материале локального уровня (обычно только матрицу модели).
    /// @param material указатель на материал, который будет применен.
    /// @param model константная ссылка на применяемую матрицу модели.
    /// @return true в случае успеха иначе false.
    MAPI bool ApplyLocal(Material* material, const Matrix4D& model);

private:
    bool CreateDefaultMaterial();
    bool LoadMaterial(const MaterialConfig& config, Material* m);
    void DestroyMaterial(Material* m);
};
