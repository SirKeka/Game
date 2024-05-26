/**
 * @file material_system.h
 * @author
 * @brief Система материалов отвечает за управление материалами в движке, включая подсчет ссылок и автоматическую выгрузку.
 * @version 1.0
 * @date
 * 
 * @copyright Kohi Game Engine is Copyright (c) Travis Vroman 2021-2022
 * 
 */
#pragma once
#include "resources/material.hpp"
#include "containers/hashtable.hpp"
#include "math/matrix4d.hpp"

constexpr const char* DEFAULT_MATERIAL_NAME = "default";    // Имя материала по умолчанию.

class MaterialSystem
{
private:
    static u32 MaxMaterialCount;                            // Максимальное количество загружаемых материалов.
    
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
    } MaterialLocations;                                    // Известные местоположения шейдера материала.
    u32 MaterialShaderID;

    struct UI_ShaderUniformLocations {
        u16 projection;
        u16 view;
        u16 DiffuseColour;
        u16 DiffuseTexture;
        u16 model;
    } UI_Locations;
    u32 UI_ShaderID;

    static MaterialSystem* state;                           // Экземпляр системы материалов (синглтон)

    /// @brief Инициализирует систему материалов при создании объекта.
    MaterialSystem();
public:
    ~MaterialSystem() = default;
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator= (const MaterialSystem&) = delete;

    // Методы основного класса------------------------------------------------------------------------------------------------------

    /// @brief Предоставляет доступ к экземпляру объекта системы материалов.
    /// @return Указатель на систему материалов.
    static MINLINE MaterialSystem* Instance() { /*if(state) */return state; }
    static void SetMaxMaterialCount(u32 value);
    static Material* GetDefaultMaterial();
    static void Check() {for (u32 i = 0; i < 73; ++i) { MTRACE("id%u, %u", state->RegisteredMaterials[i].id, i);}} // TODO: временно

    bool Initialize();
    void Shutdown();
    //-------------------------------------------------------------------------------------------------------------------------------

    // Внутренние методы-------------------------------------------------------------------------------------------------------------

    /// @brief Пытается получить материал с заданным именем. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал найден и загружен, его счетчик ссылок увеличивается.
    /// @param name имя искомого материала.
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    Material* Acquire(const char* name);
    /// @brief Пытается получить материал из заданной конфигурации. Если он еще не загружен, это запускает его загрузку. 
    /// Если материал не найден, возвращается указатель на материал по умолчанию. Если материал _найден и загружен, его счетчик ссылок увеличивается.
    /// @param config конфигурация загружаемого материала
    /// @return Указатель на загруженный материал. Может быть указателем на материал по умолчанию, если он не найден.
    Material* Acquire(MaterialConfig config);
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
