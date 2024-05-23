#pragma once
#include "resources/material.hpp"
#include "containers/hashtable.hpp"

constexpr const char* DEFAULT_MATERIAL_NAME = "default";

class MaterialSystem
{
private:
    static u32 MaxMaterialCount;                            // Конфигурация системы материалов
    
    char name[MATERIAL_NAME_MAX_LENGTH];                    // Конфигурация материала
    bool AutoRelease;
    bool init = false; // TODO: временно
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH];
    Vector4D<f32> DiffuseColour;
    Material DefaultMaterial;
    
    Material* RegisteredMaterials;                          // Массив зарегистрированных материалов.

    struct MaterialReference {
        u64 ReferenceCount;
        u32 handle;
        bool AutoRelease;
    };
    
    HashTable<MaterialReference> RegisteredMaterialTable;   // Хэш-таблица для поиска материалов.

    struct MaterialShaderUniformLocations {
        u16 projection;
        u16 view;
        u16 DiffuseColour;
        u16 DiffuseTexture;
        u16 model;
    } MaterialLocation;                                     // Известные местоположения шейдера материала.
    u32 MaterialShaderID;

    struct UI_ShaderUniformLocations {
        u16 projection;
        u16 view;
        u16 DiffuseColour;
        u16 DiffuseTexture;
        u16 model;
    } UI_Locations;
    u32 UI_ShaderID;

    static MaterialSystem* state;

    MaterialSystem();
public:
    ~MaterialSystem() = default;
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator= (const MaterialSystem&) = delete;

    // Методы основного класса------------------------------------------------------------------------------------------------------

    static MINLINE MaterialSystem* Instance() { /*if(state) */return state; }
    static void SetMaxMaterialCount(u32 value);
    static Material* GetDefaultMaterial();
    static void Check() {for (u32 i = 0; i < 73; ++i) { MTRACE("id%u, %u", state->RegisteredMaterials[i].id, i);}} // TODO: временно

    bool Initialize();
    void Shutdown();
    //-------------------------------------------------------------------------------------------------------------------------------

    // Внутренние методы-------------------------------------------------------------------------------------------------------------

    Material* Acquire(const char* name);
    Material* AcquireFromConfig(MaterialConfig config);
    void Release(const char* name);
    /// @brief Применяет данные глобального уровня для идентификатора шейдера материала.
    /// @param ShaderID идентификатор шейдера, к которому применяются глобальные переменные.
    /// @param projection константная ссылка на матрицу проекции.
    /// @param view константная ссылка на матрицу представления.
    /// @return true в случае успеха иначе false.
    bool ApplyGlobal(u32 ShaderID, const Matrix4D& projection, const Matrix4D& view);
    /// @brief Применяет данные материала на уровне экземпляра для данного материала.
    /// @param material указатель на материал, который будет применен.
    /// @return true в случае успеха иначе false.
    bool ApplyInstance(Material* material);
    /// @brief Применяет данные о материале локального уровня (обычно только матрицу модели).
    /// @param material указатель на материал, который будет применен.
    /// @param model константная ссылка на применяемую матрицу модели.
    /// @return true в случае успеха иначе false.
    bool ApplyLocal(Material* material, const Matrix4D& model);

private:
    bool CreateDefaultMaterial();
    bool LoadMaterial(MaterialConfig config, Material* m);
    void DestroyMaterial(Material* m);

public:
    void* operator new(u64 size);
};
