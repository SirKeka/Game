#pragma once
#include "defines.hpp"
#include "math/transform.hpp"
#include "core/mmemory.hpp"

struct GeometryID;

struct Mesh {
    u32 UniqueID;
    u8 generation;
    u16 GeometryCount;
    GeometryID** geometries;
    Transform transform;

    constexpr Mesh() : UniqueID(), generation(), GeometryCount(), geometries(nullptr), transform() {}
    constexpr Mesh(u16 GeometryCount, GeometryID** geometries, const Transform& transform)
    : UniqueID(), generation(), GeometryCount(GeometryCount), geometries(geometries), transform(transform) {}
    Mesh(const Mesh& mesh)
    : GeometryCount(mesh.GeometryCount), geometries(new GeometryID*[GeometryCount]), transform(mesh.transform) {
        for (u64 i = 0; i < GeometryCount; i++) {
            geometries[i] = mesh.geometries[i];
        }
    }
    constexpr Mesh(Mesh&& mesh) : UniqueID(mesh.UniqueID), generation(mesh.generation), GeometryCount(mesh.GeometryCount), geometries(mesh.geometries), transform(mesh.transform)
    {
        mesh.UniqueID = INVALID::ID;
        mesh.generation = 0;
        mesh.GeometryCount = 0;
        mesh.geometries = nullptr;
        mesh.transform = Transform();
    }
    ~Mesh() {}
    Mesh& operator=(const Mesh& mesh) {
        if (GeometryCount != mesh.GeometryCount) {
            if (GeometryCount && geometries) {
                delete[] geometries;
            }
            this->GeometryCount = mesh.GeometryCount;
        }

        geometries = new GeometryID*[GeometryCount];
        for (u64 i = 0; i < GeometryCount; i++) {
            geometries[i] = mesh.geometries[i];
        }
        transform = mesh.transform;
        return *this;
    }

    struct PacketData {
        u32 MeshCount;
        Mesh** meshes;
    };

    MAPI bool LoadFromResource(const char* ResourceName);
    void Unload();
};

