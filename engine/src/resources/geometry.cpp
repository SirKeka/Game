#include "geometry.hpp"
#include "renderer/renderer.hpp"

Geometry::Geometry() 
: 
id(), 
InternalID(), 
generation(), 
name(GEOMETRY_NAME_MAX_LENGTH), 
VertexCount(), 
VertexSize(),
VertexBufferOffset(),
IndexCount(),
IndexSize(),
IndexBufferOffset(),
material(nullptr) {}

Geometry::Geometry(u32 id, u32 InternalID, u32 generation, MString name, u32 VertexCount, u32 VertexSize, u32 VertexBufferOffset, u32 IndexCount, u32 IndexSize, u32 IndexBufferOffset, Material *material)
{
    this->id = id;
    this->InternalID = InternalID;
    this->generation = generation;
    this->name = name;
    this->VertexCount = VertexCount;
    this->VertexSize = VertexSize;
    this->VertexBufferOffset = VertexBufferOffset;
    this->IndexCount = IndexCount;
    this->IndexSize = IndexSize;
    this->IndexBufferOffset = IndexBufferOffset;
    this->material = material;
}

Geometry::Geometry(const Geometry &g) 
: 
id(g.id), 
InternalID(g.InternalID), 
generation(g.generation), 
name(g.name), 
VertexCount(g.VertexCount), 
VertexSize(g.VertexSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexSize(g.IndexSize),
IndexBufferOffset(g.IndexBufferOffset),
material(g.material) {}

Geometry::Geometry(Geometry &&g)
: 
id(g.id), 
InternalID(g.InternalID), 
generation(g.generation), 
name(g.name), 
VertexCount(g.VertexCount), 
VertexSize(g.VertexSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexSize(g.IndexSize),
IndexBufferOffset(g.IndexBufferOffset),
material(g.material)
{
    g.id = INVALID_ID;
    g.InternalID = INVALID_ID;
    g.generation = INVALID_ID;
    // g.name
    g.VertexCount = 0;
    g.VertexSize = 0;
    g.VertexBufferOffset = 0;
    g.IndexCount = 0;
    g.IndexSize = 0;
    g.IndexBufferOffset = 0;
    g.material = nullptr;
}

bool Geometry::Create(/*VulkanAPI* VkAPI, */u32 VertexCount, const Vertex3D &vertices, u32 IndexCount, u32 *indices)
{
    if (!VertexCount || !vertices) {
        MERROR("Geometry::Create требует данных вершин, но они не были предоставлены. VertexCount=%d, vertices=%p", VertexCount, vertices);
        return false;
    }

    // Проверьте, не повторная ли это загрузка. Если это так, необходимо впоследствии освободить старые данные.
    bool IsReupload = this->InternalID != INVALID_ID;
    Geometry OldRange;
    VulkanAPI* VkAPI = Renderer::GetRenderer();
    //Geometry* InternalData = nullptr;
    if (IsReupload) {
        this = &VkAPI.geometries[this->InternalID];

        // Скопируйте старый диапазон.
        OldRange = this;
    } else {
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (VkAPI.geometries[i].id == INVALID_ID) {
                // Найден свободный индекс.
                this->InternalID = i;
                VkAPI.geometries[i].id = i;
                InternalData = &VkAPI.geometries[i];
                break;
            }
        }
    }
    if (!InternalData) {
        MFATAL("Geometry::Create не удалось найти свободный индекс для загрузки новой геометрии. Настройте конфигурацию, чтобы обеспечить больше.");
        return false;
    }

    VkCommandPool pool = VkAPI.Device.graphics_command_pool;
    VkQueue queue = VkAPI.device.graphics_queue;

    // Vertex data.
    InternalData->vertex_buffer_offset = VkAPI.geometry_vertex_offset;
    InternalData->vertex_count = vertex_count;
    InternalData->vertex_size = sizeof(vertex_3d) * vertex_count;
    upload_data_range(&VkAPI, pool, 0, queue, &VkAPI.object_vertex_buffer, InternalData->vertex_buffer_offset, InternalData->vertex_size, vertices);
    // TODO: should maintain a free list instead of this.
    VkAPI.geometry_vertex_offset += InternalData->vertex_size;

    // Index data, if applicable
    if (index_count && indices) {
        InternalData->index_buffer_offset = VkAPI.geometry_index_offset;
        InternalData->index_count = index_count;
        InternalData->index_size = sizeof(u32) * index_count;
        upload_data_range(&VkAPI, pool, 0, queue, &VkAPI.object_index_buffer, InternalData->index_buffer_offset, InternalData->index_size, indices);
        // TODO: should maintain a free list instead of this.
        VkAPI.geometry_index_offset += InternalData->index_size;
    }

    if (InternalData->generation == INVALID_ID) {
        InternalData->generation = 0;
    } else {
        InternalData->generation++;
    }

    if (is_reupload) {
        // Free vertex data
        free_data_range(&VkAPI.object_vertex_buffer, OldRange.vertex_buffer_offset, OldRange.vertex_size);

        // Free index data, if applicable
        if (OldRange.index_size > 0) {
            free_data_range(&VkAPI.object_index_buffer, OldRange.index_buffer_offset, OldRange.index_size);
        }
    }

    return true;
}

void Geometry::Destroy()
{
    if (geometry && geometry->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(VkAPI.device.logical_device);
        vulkan_geometry_data* InternalData = &VkAPI.geometries[geometry->internal_id];

        // Free vertex data
        free_data_range(&VkAPI.object_vertex_buffer, InternalData->vertex_buffer_offset, InternalData->vertex_size);

        // Free index data, if applicable
        if (InternalData->index_size > 0) {
            free_data_range(&VkAPI.object_index_buffer, InternalData->index_buffer_offset, InternalData->index_size);
        }

        // Clean up data.
        kzero_memory(InternalData, sizeof(vulkan_geometry_data));
        InternalData->id = INVALID_ID;
        InternalData->generation = INVALID_ID;
    }
}
