#pragma once
#include "defines.hpp"
#include "math/matrix4d.hpp"

struct Mesh {
    u16 GeometryCount;
    class Geometry** geometries;
    Matrix4D model;
};