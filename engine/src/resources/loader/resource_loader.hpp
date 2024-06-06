#pragma once
#include "defines.hpp"
#include "core/logger.hpp"

enum class ResourceType {
    Text,
    Binary,
    Image,
    Material,
    Mesh,
    Shader,
    Custom
};

class ResourceLoader
{
    friend class ResourceSystem;
protected:
    u32 id;
    ResourceType type;
    const char* CustomType;
    const char* TypePath;
public:
    ResourceLoader() : id(INVALID::ID), type(), CustomType(nullptr), TypePath(nullptr) {}
    ResourceLoader(ResourceType type, const char* CustomType, const char* TypePath) : id(INVALID::ID), type(type), CustomType(CustomType), TypePath(TypePath) {}
    virtual ~ResourceLoader() {
        id = INVALID::ID;
    };
    MINLINE void Destroy() {this->~ResourceLoader();}

    virtual bool Load(const char* name, struct Resource& OutResource) {
        MERROR("Вызывается не инициализированный загрузчик");
        return false;
    }
    virtual void Unload(struct Resource& resource){
        MERROR("Вызывается не инициализированный загрузчик");
    }
};
