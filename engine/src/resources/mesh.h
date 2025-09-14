#pragma once

#include "math/transform.h"
#include "math/extents.h"
#include "core/memory_system.h"

struct Geometry;

struct MAPI Mesh {
    struct Config {
        MString name;
        MString ParentName;
        MString ResourceName;
        u16 GeometryCount;
        struct GeometryConfig* GConfigs;
    } config;

    u32 UniqueID;
    u8 generation;
    u16 GeometryCount;
    Geometry** geometries;
    Transform transform;
    Extents3D extents;
    void* DebugData;

    constexpr Mesh() : config(), UniqueID(), generation(), GeometryCount(), geometries(nullptr), transform() {}
    constexpr Mesh(Config& config) : config(static_cast<Config&&>(config)), UniqueID(INVALID::U8ID), generation(), GeometryCount(), geometries(nullptr), transform() {}
    Mesh(u16 GeometryCount, Geometry** geometries, const Transform& transform)
    : UniqueID(), generation(), GeometryCount(GeometryCount), geometries(geometries), transform(transform) {}
    Mesh(const Mesh& mesh)
    : config(mesh.config), UniqueID(mesh.UniqueID), generation(mesh.generation), GeometryCount(mesh.GeometryCount), geometries(new Geometry*[GeometryCount]), transform(mesh.transform) {
        for (u64 i = 0; i < GeometryCount; i++) {
            geometries[i] = mesh.geometries[i];
        }
    }
    Mesh(Mesh&& mesh) : config(static_cast<Config&&>(mesh.config)), UniqueID(mesh.UniqueID), generation(mesh.generation), GeometryCount(mesh.GeometryCount), geometries(mesh.geometries), transform(mesh.transform)
    {
        MemorySystem::ZeroMem(&mesh, sizeof(Mesh));
    }
    ~Mesh() {}
    Mesh& operator=(const Mesh& mesh) {
        if (GeometryCount != mesh.GeometryCount) {
            if (GeometryCount && geometries) {
                delete[] geometries;
            }
            this->GeometryCount = mesh.GeometryCount;
        }

        geometries = new Geometry*[GeometryCount];
        for (u64 i = 0; i < GeometryCount; i++) {
            geometries[i] = mesh.geometries[i];
        }
        transform = mesh.transform;
        return *this;
    }
    Mesh& operator=(Mesh&& mesh) {
        config = static_cast<Config&&>(mesh.config);
        UniqueID = mesh.UniqueID;
        generation = mesh.generation;
        GeometryCount = mesh.GeometryCount;
        geometries = mesh.geometries;
        transform = mesh.transform;
        MemorySystem::ZeroMem(&mesh, sizeof(Mesh));
        return *this;
    }

    struct PacketData {
        u32 MeshCount;
        Mesh** meshes;
    };

    // bool Load(const char* ResourceName);
    bool Create(Config& config);
    bool Initialize();
    bool Load();
    bool Unload();
    bool Destroy();
};

