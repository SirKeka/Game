#pragma once
#include "defines.hpp"
#include "math/matrix4d.hpp"
#include "core/mmemory.hpp"

class GeometryID;

struct Mesh {
    u16 GeometryCount{};
    GeometryID** geometries{nullptr};
    Matrix4D model{};

    constexpr Mesh() : GeometryCount(), geometries(nullptr), model() {}
    constexpr Mesh(u16 GeometryCount, GeometryID** geometries, Matrix4D model)
    : GeometryCount(GeometryCount), geometries(geometries), model(model) {}
    Mesh(const Mesh& mesh)
    : GeometryCount(mesh.GeometryCount), geometries(new GeometryID*[GeometryCount]), model(mesh.model) {
        for (u64 i = 0; i < GeometryCount; i++) {
            geometries[i] = mesh.geometries[i];
        }
    }
    constexpr Mesh(Mesh&& mesh) : GeometryCount(mesh.GeometryCount), geometries(mesh.geometries), model(mesh.model)
    {
        mesh.GeometryCount = 0;
        mesh.geometries = nullptr;
        mesh.model = Matrix4D(0.f, 0.f, 0.f, 0.f,
                              0.f, 0.f, 0.f, 0.f, 
                              0.f, 0.f, 0.f, 0.f, 
                              0.f, 0.f, 0.f, 0.f);
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
        model = mesh.model;
    }
};