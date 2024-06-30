#include "geometry_utils.hpp"

void Math::Geometry::GenerateNormals(u32 VertexCount, Vertex3D *vertices, u32 IndexCount, u32 *indices)
{
    for (u32 i = 0; i < IndexCount; i += 3) {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        FVec3 edge1 = vertices[i1].position - vertices[i0].position;
        FVec3 edge2 = vertices[i2].position - vertices[i0].position;

        FVec3 normal = Normalize(Cross(edge1, edge2));

        // ПРИМЕЧАНИЕ: Это просто создает нормаль лица. При желании сглаживание следует выполнить отдельным проходом.
        vertices[i0].normal = normal;
        vertices[i1].normal = normal;
        vertices[i2].normal = normal;
    }
}

void Math::Geometry::GenerateTangents(u32 VertexCount, Vertex3D *vertices, u32 IndexCount, u32 *indices)
{
    for (u32 i = 0; i < IndexCount; i += 3) {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        FVec3 edge1 = vertices[i1].position - vertices[i0].position;
        FVec3 edge2 = vertices[i2].position - vertices[i0].position;

        f32 deltaU1 = vertices[i1].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV1 = vertices[i1].texcoord.y - vertices[i0].texcoord.y;

        f32 deltaU2 = vertices[i2].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV2 = vertices[i2].texcoord.y - vertices[i0].texcoord.y;

        f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
        f32 fc = 1.f / dividend;

        FVec3 tangent = FVec3(
            (fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
            (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
            (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z)));

        tangent.Normalize();

        f32 sx = deltaU1, sy = deltaU2;
        f32 tx = deltaV1, ty = deltaV2;
        f32 handedness = ((tx * sy - ty * sx) < 0.f) ? -1.f : 1.f;
        FVec4 t4 { tangent, handedness };
        vertices[i0].tangent = t4;
        vertices[i1].tangent = t4;
        vertices[i2].tangent = t4;
    }
}
