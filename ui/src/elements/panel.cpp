#include "panel.h"
#include "core/systems_manager.h"
#include "math/geometry_utils.h"
#include "renderer/rendering_system.h"
#include "systems/geometry_system.h"
#include "systems/shader_system.h"

// bool Panel::Create(const char *name, FVec2 size, const FVec4 &colour, UiElement *parent)
// {
//     this->name = name;
//     rect.Set(0, 0, size.width, size.height);
//     this->colour = colour;
//     this->parent = parent;
//     return true;
// }

bool Panel::Load()
{
    // Сгенерировать UV-развертку.
    f32 xmin, ymin, xmax, ymax;
    GenerateUvsFromImageCoords(512, 512, 44, 7, xmin, ymin);
    GenerateUvsFromImageCoords(512, 512, 73, 36, xmax, ymax);

    // Создать простую плоскость.
    GeometryConfig UiConfig{};
    Math::Geometry::GenerateQuad2D(name.c_str(), rect.width, rect.height, xmin, xmax, ymin, ymax, UiConfig);
    // Получить геометрию пользовательского интерфейса из конфигурации. ПРИМЕЧАНИЕ: эта загрузка в графический процессор
    geometry = GeometrySystem::Acquire(UiConfig, true);

    auto pState = (UiSystem*)SystemsManager::GetState(128);  // HACK: требует стандартного способа получения типов расширения.

    // Получить ресурсы экземпляра для этого элемента управления.
    TextureMap* maps[1] = {&pState->Atlas()};
    auto shader = ShaderSystem::GetShader("Shader.StandardUI");
    u16 AtlasLocation = shader->uniforms[shader->InstanceSamplerIndices[0]].index;
    // Известно количество карт этого типа.
    ShaderInstanceUniformTextureConfig AtlasTexture{
        .UniformLocation = AtlasLocation,
        .TextureMapCount = 1,
        .TextureMaps = maps
    };
    ShaderInstanceResourceConfig InstanceResourceConfig{ 1, &AtlasTexture };
    SystemsManager::GetRenderingSystem()->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, InstanceID);

    return true;
}

bool Panel::Render(FrameData &rFrameData, UiRenderData &RenderData)
{
    if (geometry) {
        UiRenderable renderable {};
        renderable.RenderData.UniqueID = id.UniqueID;
        renderable.RenderData.geometry = geometry;
        renderable.RenderData.model = xform.GetWorld();
        renderable.RenderData.DiffuseColour = colour;

        renderable.InstanceID = &InstanceID;
        renderable.FrameNumber = &FrameNumber;
        renderable.DrawIndex = &DrawIndex;

        RenderData.renderables.PushBack(renderable);
    }

    return true;
}

FVec2 Panel::Size()
{
    return FVec2(rect.width, rect.height);
}

bool Panel::Resize(FVec2 NewSize)
{
    rect.width = NewSize.x;
    rect.height = NewSize.y;
    auto vertices = (Vertex2D*)geometry->vertices;
    vertices[1].position.y = NewSize.y;
    vertices[1].position.x = NewSize.x;
    vertices[2].position.y = NewSize.y;
    vertices[3].position.x = NewSize.x;
    SystemsManager::GetRenderingSystem()->GeometryVertexUpdate(geometry, 0, geometry->VertexCount, vertices);

    return true;
}
