#include "geometry.hpp"
#include "core/logger.hpp"
#include "renderer/vulkan/vulkan_api.hpp"

Geometry::Geometry() 
: 
id(INVALID_ID), 
InternalID(INVALID_ID), 
generation(INVALID_ID), 
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

Geometry &Geometry::operator=(const Geometry &g)
{
    VertexCount = g.VertexCount;
    VertexSize = g.VertexSize;
    VertexBufferOffset = g.VertexBufferOffset;
    IndexCount = g.IndexCount;
    IndexSize = g.IndexSize;
    IndexBufferOffset = g.IndexBufferOffset;
    return *this;
}

Geometry &Geometry::operator=(const Geometry *g)
{
    VertexCount = g->VertexCount;
    VertexSize = g->VertexSize;
    VertexBufferOffset = g->VertexBufferOffset;
    IndexCount = g->IndexCount;
    IndexSize = g->IndexSize;
    IndexBufferOffset = g->IndexBufferOffset;
    return *this;
}

void Geometry::SetVertexData(u32 VertexCount, u32 VertexSize, u32 VertexBufferOffset) 
{
    this->VertexCount = VertexCount;
    this->VertexSize = VertexSize;
    this->VertexBufferOffset = VertexBufferOffset;
}

void Geometry::SetIndexData(u32 IndexCount, u32 IndexSize, u32 IndexBufferOffset)
{
    this->IndexCount = IndexCount;
    this->IndexSize = IndexSize;
    this->IndexBufferOffset = IndexBufferOffset;
}

/*bool Geometry::Create(VulkanAPI* VkAPI, u32 VertexCount, const Vertex3D &vertices, u32 IndexCount, u32 *indices)
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
}*/

bool Geometry::SendToRender(RendererType *rType, u32 VertexCount, const Vertex3D *vertices, u32 IndexCount, const u32 *indices)
{
    if (!VertexCount || !vertices) {
        MERROR("VulkanAPI::CreateGeometry требует данных вершин, но они не были предоставлены. VertexCount=%d, vertices=%p", VertexCount, vertices);
        return false;
    }

    // Проверьте, не повторная ли это загрузка. Если это так, необходимо впоследствии освободить старые данные.
    bool IsReupload = this->InternalID != INVALID_ID;
    Geometry OldRange;
    VulkanAPI* pRenderer = dynamic_cast<VulkanAPI*>(rType);

    Geometry* TempGeometry = nullptr;
    if (IsReupload) {
        TempGeometry = &pRenderer->geometries[this->InternalID];

        // Скопируйте старый диапазон.
        /*old_range.index_buffer_offset = this->index_buffer_offset;
        old_range.index_count = this->index_count;
        old_range.index_size = this->index_size;
        old_range.vertex_buffer_offset = this->vertex_buffer_offset;
        old_range.VertexCount = this->VertexCount;
        old_range.vertex_size = this->vertex_size;*/
        OldRange = TempGeometry;
    } else {
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (pRenderer->geometries[i].id == INVALID_ID) {
                // Найден свободный индекс.
                this->InternalID = i;
                pRenderer->geometries[i].id = i;
                this = &pRenderer->geometries[i];
                break;
            }
        }
    }
    if (!this) {
        MFATAL("VulkanRenderer::CreateGeometry не удалось найти свободный индекс для загрузки новой геометрии. Настройте конфигурацию, чтобы обеспечить больше.");
        return false;
    }

    VkCommandPool &pool = pRenderer->Device.GraphicsCommandPool;
    VkQueue &queue = pRenderer->Device.GraphicsQueue;

    // Данные вершин.
    this->SetVertexData(VertexCount, sizeof(Vertex3D) * VertexCount, pRenderer->GeometryVertexOffset);

    UploadDataRange(&pRenderer, pool, 0, queue, &pRenderer->ObjectVertexBuffer, this->VertexBufferOffset, this->VertexSize, vertices);
    // TODO: вместо этого следует поддерживать список свободной памяти
    pRenderer->GeometryVertexOffset += this->VertexSize;

    // Данные индексов, если применимо
    if (IndexCount && indices) {
        this->SetIndexData(IndexCount, sizeof(u32) * IndexCount, pRenderer->GeometryIndexOffset);
        UploadDataRange(&pRenderer, pool, 0, queue, &pRenderer->ObjectIndexBuffer, this->IndexBufferOffset, this->IndexSize, indices);
        // TODO: вместо этого следует поддерживать список свободной памяти
        pRenderer->GeometryIndexOffset += sizeof(u32) * IndexCount;
    }

    if (this->generation == INVALID_ID) {
        this->generation = 0;
    } else {
        this->generation++;
    }

    if (IsReupload) {
        // Освобождение данных вершин
        FreeDataRange(&pRenderer->ObjectVertexBuffer, OldRange.VertexBufferOffset, OldRange.VertexSize);

        // Освобождение данных индексов, если применимо
        if (OldRange.IndexSize > 0) {
            FreeDataRange(&pRenderer->ObjectIndexBuffer, OldRange.IndexBufferOffset, OldRange.IndexSize);
        }
    }

    return true;
}

void Geometry::Destroy()
{
    if (this && this->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(VkAPI.device.logical_device);
        vulkan_geometry_data* InternalData = &VkAPI.geometries[this->internal_id];

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
