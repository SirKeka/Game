#pragma once

#include "core/identifier.h"
#include "core/memory_system.h"
#include "math/extents.h"
#include "math/transform.h"

struct Geometry;

struct MAPI Mesh {
    MString name;
    MString ParentName;
    MString ResourceName;

    Identifier id;
    u8 generation;
    u16 GeometryCount;
    Geometry** geometries;
    Transform transform;
    Extents3D extents;
    void* DebugData;

    constexpr Mesh() : name(), ParentName(), ResourceName(), id(INVALID::ID), generation(INVALID::U8ID), GeometryCount(), geometries(nullptr), transform() {}
    constexpr Mesh(struct MeshConfig& config);
    Mesh(u16 GeometryCount, Geometry** geometries, const Transform& transform)
    : id(), generation(), GeometryCount(GeometryCount), geometries(geometries), transform(transform) {}
    constexpr Mesh(const Mesh& mesh);

    ~Mesh() {}

    // Mesh& operator=(const Mesh& mesh) {
    //     if (GeometryCount != mesh.GeometryCount) {
    //         if (GeometryCount && geometries) {
    //             delete[] geometries;
    //         }
    //         this->GeometryCount = mesh.GeometryCount;
    //     }

    //     geometries = new Geometry*[GeometryCount];
    //     for (u64 i = 0; i < GeometryCount; i++) {
    //         geometries[i] = mesh.geometries[i];
    //     }
    //     transform = mesh.transform;
    //     return *this;
    // }

    // Mesh& operator=(Mesh&& mesh) {
    //     config = (Config&&)mesh.config;
    //     id = mesh.id;
    //     generation = mesh.generation;
    //     GeometryCount = mesh.GeometryCount;
    //     geometries = mesh.geometries;
    //     transform = mesh.transform;
    //     MemorySystem::ZeroMem(&mesh, sizeof(Mesh));
    //     return *this;
    // }

    struct PacketData {
        u32 MeshCount;
        Mesh** meshes;
    };

    // bool Load(const char* ResourceName);
    bool Create(MeshConfig& config);
    bool Initialize();
    bool Load();
    bool Unload();
    bool Destroy();
};
