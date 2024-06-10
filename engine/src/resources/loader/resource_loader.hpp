#pragma once
#include "defines.hpp"
#include "core/logger.hpp"

enum class ResourceType {
    Invalid,
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
    u32 id{INVALID::ID};
    ResourceType type;
    MString CustomType;
    MString TypePath;
public:
    // constexpr ResourceLoader() : id(INVALID::ID), type(), CustomType(), TypePath() {}
    constexpr ResourceLoader(ResourceType type, const MString& CustomType, const MString& TypePath) : id(), type(type), CustomType(CustomType), TypePath(TypePath) {}
    virtual ~ResourceLoader() {
        id = INVALID::ID;
    };
    MINLINE void Destroy() { this->~ResourceLoader(); }

    virtual bool Load(const char* name, struct Resource& OutResource) {
        MERROR("Вызывается не инициализированный загрузчик");
        return false;
    }
    virtual void Unload(struct Resource& resource){
        MERROR("Вызывается не инициализированный загрузчик");
    }
};
