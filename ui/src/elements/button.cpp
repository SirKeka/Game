#include "button.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "resources/shader.h"
#include "resources/geometry.h"
#include "systems/shader_system.h"

void Button::Destroy()
{
}

bool Button::SetHeight(i32 height)
{
    size.y = height;
    nSlice.size.y = height;

    bounds.height = height;

    nSlice.Update();

    return true;
}

bool Button::Load()
{
    auto uiSys = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);

    // HACK: ЗАДАЧА: Удалите жестко закодированные данные.
    /* Point AtlasSize {UiAtlas.texture->width, UiAtlas.texture->height}; */

    nSlice.CornerSize.Set(10, 10);
    nSlice.CornerPxSize.Set(3, 3);
    nSlice.size = size;
    nSlice.AtlasPxMin.Set(151, 12);
    nSlice.AtlasPxMax.Set(158, 19);
    nSlice.AtlasPxSize.Set(512, 512);
    nSlice.Generate(name.c_str());

    bounds.x = 0.F;
    bounds.y = 0.F;
    bounds.width = size.x;
    bounds.height = size.y;

    // Выделите ресурсы экземпляра для этого элемента управления.
    TextureMap* maps[1] = { &uiSys->Atlas() };
    Shader* shader = nullptr;
    if (!(shader = ShaderSystem::GetShader("Shader.StandardUI"))) {
        MERROR("Не удалось получить шейдер при загрузке кнопки!");
        return false;
    }
     
    u16 AtlasLocation = shader->uniforms[shader->InstanceSamplerIndices[0]].index;
    // Известно количество карт этого типа.
    ShaderInstanceUniformTextureConfig AtlasTexture{
        .UniformLocation = AtlasLocation,
        .TextureMapCount = 1,
        .TextureMaps = maps
    };
    ShaderInstanceResourceConfig InstanceResourceConfig{ 
        .UniformConfigCount = 1,
        .UniformConfigs = &AtlasTexture
    };

    SystemsManager::GetRenderingSystem()->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, InstanceID);
    return true;
}

void Button::Unload()
{
}

bool Button::Update(FrameData &rFrameData)
{
    return false;
}

void Button::RenderPrepare(const FrameData &rFrameData)
{
    nSlice.RenderFramePrepare(rFrameData);
}

bool Button::Render(FrameData &rFrameData, UiRenderData &RenderData)
{
    if (nSlice.geometry) {
        UiRenderable* renderable = RenderData.renderables.PushBack();
        MemorySystem::ZeroMem(renderable, sizeof(UiRenderable));
        renderable->InstanceID = &InstanceID;
        renderable->FrameNumber = &FrameNumber;
        renderable->DrawIndex = &DrawIndex;

        GeometryRenderData& grd = renderable->RenderData;
        grd.UniqueID            = id.UniqueID;
        grd.model               = xform.GetWorld();
        grd.DiffuseColour       = FVec4::One();  //белый. ЗАДАЧА: извлечь данные из свойств объекта.
        grd.CopyData(nSlice.geometry);

    }

    return true;
}

void Button::OnMouseDown(UiElement* self, UiMouseEvent event)
{
    if (!self) { return; }
    auto& nSlice = ((Button*)self)->nSlice;
    nSlice.AtlasPxMin.x = 151;
    nSlice.AtlasPxMin.y = 21;
    nSlice.AtlasPxMax.x = 158;
    nSlice.AtlasPxMax.y = 28;
    nSlice.Update();
}

void Button::OnMouseUp(UiElement* self, UiMouseEvent event)
{
    if (!self) { return; }
    auto& nSlice = ((Button*)self)->nSlice;
    if (self->IsHovered) {
            nSlice.AtlasPxMin.x = 151;
            nSlice.AtlasPxMin.y = 31;
            nSlice.AtlasPxMax.x = 158;
            nSlice.AtlasPxMax.y = 37;
        } else {
            nSlice.AtlasPxMin.x = 151;
            nSlice.AtlasPxMin.y = 31;
            nSlice.AtlasPxMax.x = 158;
            nSlice.AtlasPxMax.y = 37;
        }
        nSlice.Update();
}

void Button::OnMouseOver(UiElement* self, UiMouseEvent event)
{
    if (!self) { return; }
    auto& nSlice = ((Button*)self)->nSlice;
    if (self->IsPressed) {
        nSlice.AtlasPxMin.x = 151;
        nSlice.AtlasPxMin.y = 21;
        nSlice.AtlasPxMax.x = 158;
        nSlice.AtlasPxMax.y = 28;
    } else {
        nSlice.AtlasPxMin.x = 151;
        nSlice.AtlasPxMin.y = 31;
        nSlice.AtlasPxMax.x = 158;
        nSlice.AtlasPxMax.y = 37;
    }
    nSlice.Update();
}

void Button::OnMouseOut(UiElement* self, UiMouseEvent event)
{
    if (!self) { return; }
    auto& nSlice = ((Button*)self)->nSlice;
    nSlice.AtlasPxMin.x = 151;
    nSlice.AtlasPxMin.y = 12;
    nSlice.AtlasPxMax.x = 158;
    nSlice.AtlasPxMax.y = 19;
    nSlice.Update();
}

void Button::InitFunctions()
{
    UiElement::OnMouseDown = OnMouseDown;
    UiElement::OnMouseUp   = OnMouseUp;
    UiElement::OnMouseOver = OnMouseOver;
    UiElement::OnMouseOut  = OnMouseOut;
}
