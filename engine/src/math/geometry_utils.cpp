#include "geometry_utils.hpp"
#include "resources/geometry.hpp"

namespace Math
{
    void ReassignIndex(u32 IndexCount, u32* indices, u32 from, u32 to) 
    {
        for (u32 i = 0; i < IndexCount; ++i) {
        if (indices[i] == from) {
            indices[i] = to;
        } else if (indices[i] > from) {
            // Поднимите все индексы выше значения «от» на 1.
            indices[i]--;
        }
    }
    }

    void Geometry::GenerateNormals(u32 VertexCount, Vertex3D *vertices, u32 IndexCount, u32 *indices)
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

    void Geometry::CalculateTangents(u32 VertexCount, Vertex3D *vertices, u32 IndexCount, u32 *indices)
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
            FVec3 t4 = tangent * handedness;
            vertices[i0].tangent = t4;
            vertices[i1].tangent = t4;
            vertices[i2].tangent = t4;
        }
    }
/*
    void Geometry::CalculateTangents(const DArray<Face>& triangles, DArray<Vertex3D> &vertices)
    {
        // Выделите временное хранилище для тангенсов и битангентов и инициализируйте их нулями.
        const u64& VCount = vertices.Length();
	    FVec3 tangent[VCount]{};
	    FVec3 bitangent[VCount]{};

	    // Вычислите тангенс и битангенс для каждого треугольника и сложите все три вершины.
	    for (u32 k = 0; k < triangles.Length(); k++) {
	    	u32 i0 = triangleArray[k].index[0];
	    	u32 i1 = triangleArray[k].index[1];
	    	u32 i2 = triangleArray[k].index[2];
	    	const FVec3& p0 = vertices[i0].position;
	    	const FVec3& p1 = vertices[i1].position;
	    	const FVec3& p2 = vertices[i2].position;
	    	const FVec2& w0 = vertices[i0].texcoord;
	    	const FVec2& w1 = vertices[i1].texcoord;
	    	const FVec2& w2 = vertices[i2].texcoord;

	    	FVec3 e1 = p1 - p0, e2 = p2 - p0;
	    	f32 x1 = w1.x - w0.x, x2 = w2.x - w0.x;
	    	f32 y1 = w1.y - w0.y, y2 = w2.y - w0.y;

	    	f32 r = 1.F / (x1 * y2 - x2 * y1);
	    	FVec3 t = (e1 * y2 - e2 * y1) * r;
	    	FVec3 b = (e2 * x1 - e1 * x2) * r;

	    	tangent[i0] += t;
	    	tangent[i1] += t;
	    	tangent[i2] += t;
	    	bitangent[i0] += b;
	    	bitangent[i1] += b;
	    	bitangent[i2] += b;
	    }

	    //Ортонормируйте каждую касательную и вычислите направленность.
	    for (u32 i = 0; i < VCount; i++) {
	    	const FVec3& t = tangent[i];
	    	const FVec3& b = bitangent[i];
	    	const FVec3& n = normalArray[i];
	    	vertices[i].tangent = FVec4(Normalize(Reject(t, n)), (Dot(Cross(t, b), n) > 0.F) ? 1.F : -1.F);
	    }

    }
*/
    void Geometry::DeduplicateVertices(GeometryConfig& geometry, u32 &OutVertexCount, Vertex3D **OutVertices)
    {
        // Создайте новые массивы для размещения коллекции.
        auto vertices = reinterpret_cast<Vertex3D*>(geometry.vertices);
        auto indices = reinterpret_cast<u32*>(geometry.indices);
        u64 VertexSize = geometry.VertexCount * geometry.VertexSize;
        auto UniqueVerts = reinterpret_cast<Vertex3D*>(MemorySystem::Allocate(VertexSize, Memory::Array, true));
        OutVertexCount = 0;

        u32 FoundCount = 0;
        for (u32 v = 0; v < geometry.VertexCount; ++v) {
            bool found = false;
            for (u32 u = 0; u < OutVertexCount; ++u) {
                if (vertices[v] == UniqueVerts[u]) {
                    // Переназначить индексы, _не_ копировать
                    ReassignIndex(geometry.IndexCount, indices, v - FoundCount, u);
                    found = true;
                    FoundCount++;
                    break;
                }
            }

            if (!found) {
                // Скопируйте в уникальный.
                UniqueVerts[OutVertexCount] = vertices[v];
                OutVertexCount++;
            }
        }

        // Выделить новый массив вершин
        *OutVertices = reinterpret_cast<Vertex3D*>(MemorySystem::Allocate(OutVertexCount * geometry.VertexSize, Memory::Array, true));
        // Копировать уникальные
        MemorySystem::CopyMem(*OutVertices, UniqueVerts, sizeof(Vertex3D) * (OutVertexCount));
        // Уничтожить временный массив
        MemorySystem::Free(UniqueVerts, VertexSize, Memory::Array);

        u32 RemovedCount = geometry.VertexCount - OutVertexCount;
        MDEBUG("Geometry::DeduplicateVertices: удалено %d вершин, изначально/сейчас %d/%d.", RemovedCount, geometry.VertexCount, OutVertexCount);
    }
} // namespace Math


