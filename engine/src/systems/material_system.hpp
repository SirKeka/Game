#pragma once
#include "resources/material.hpp"
#include "containers/hashtable.hpp"

#define DEFAULT_MATERIAL_NAME "default"

struct MaterialConfig {
    char name[MATERIAL_NAME_MAX_LENGTH];
    bool AutoRelease;
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH];
    Vector4D<f32> DiffuseColour;
}

class MaterialSystem
{
private:
    // Конфигурация системы материалов
    static u32 MaxMaterialCount;
    // Конфигурация материала
    char name[MATERIAL_NAME_MAX_LENGTH];
    bool AutoRelease;
    char DiffuseMapName[TEXTURE_NAME_MAX_LENGTH];
    Vector4D<f32> DiffuseColour;

    Material DefaultMaterial;

    // Массив зарегистрированных материалов.
    Material* RegisteredMaterials;

    struct MaterialReference {
        u64 ReferenceCount;
        u32 handle;
        bool AutoRelease;
    };

    // Хэш-таблица для поиска материалов.
    HashTable<MaterialReference> RegisteredMaterialTable;

    static MaterialSystem* state;

    MaterialSystem();
public:
    ~MaterialSystem();
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator= (const MaterialSystem&) = delete;

    static MINLINE MaterialSystem* Instance() { if(state) return state; };

    bool Initialize();
    void Shutdown();

    Material* Acquire(const char* name);
    Material* AcquireFromConfig(MaterialConfig config);
    void Release(const char* name);

private:
    bool CreateDefaultMaterial();
    bool LoadMaterial(MaterialConfig config, Material* m);
    void DestroyMaterial(Material* m);
    bool LoadConfigurationFile(const char* path, MaterialConfig* OutConfig);

public:
    void* operator new(u64 size);
};
