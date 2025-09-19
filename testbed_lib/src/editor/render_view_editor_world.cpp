#include "render_view_editor_world.h"
#include "renderer/camera.h"
#include "renderer/render_view.h"
#include "renderer/renderpass.h"
#include "resources/shader.h"
#include "systems/camera_system.hpp"
#include "systems/render_view_system.h"
#include "systems/shader_system.h"
#include "gizmo.h"

struct DebugColourShaderLocations {
    u16 projection;
    u16 view;
    u16 model;
};

struct sRenderViewEditorWorld {
    Shader* shader;
    f32 fov;
    f32 NearClip;
    f32 FarClip;
    Matrix4D ProjectionMatrix;
    Camera* WorldCamera;

    DebugColourShaderLocations DebugLocations;

    constexpr sRenderViewEditorWorld() : shader(nullptr), 
    fov(Math::DegToRad(45.F)), NearClip(0.1F), FarClip(4000.F), // Получить либо переопределение пользовательского шейдера, либо заданное значение по умолчанию. ЗАДАЧА: Установить из конфигурации.
    ProjectionMatrix(Matrix4D::MakeFrustumProjection(fov, 1280 / 720.F, NearClip, FarClip)), WorldCamera(nullptr), DebugLocations() {}

    void *operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Renderer);
    }

    void operator delete(void *ptr, u64 size) {
        MemorySystem::Free(ptr, size, Memory::Renderer);
    }
};

static bool RenderViewOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context) {
    auto self = (RenderView*)ListenerInst;
    if (!self) {
        return false;
    }
    auto* data = (sRenderViewEditorWorld*)self->data;
    if (!data) {
        return false;
    }

    switch (code) {
        case EventSystem::DefaultRendertargetRefreshRequired:
            RenderViewSystem::RegenerateRenderTargets(self);
            // Это необходимо для использования другими представлениями, поэтому считайте, что оно _не_ обработано.
            return false;
    }

    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

bool RenderViewEditorWorld::OnRegistered(RenderView* self)
{
    if (self) {
        auto data = new sRenderViewEditorWorld();
        self->data = data;
        
        // Загрузить отладочный шейдер Color3D.
        // Получить унифицированные расположения шейдера Color3D.
        {
            const char* Colour3dShaderName = "Shader.Builtin.ColourShader3D";
            data->shader = ShaderSystem::GetShader(Colour3dShaderName);
            if (!data->shader) {
                MERROR("Не удалось получить шейдер colour3d!");
                return false;
            }
            data->DebugLocations.projection = ShaderSystem::UniformIndex(data->shader, "projection");
            data->DebugLocations.view = ShaderSystem::UniformIndex(data->shader, "view");
            data->DebugLocations.model = ShaderSystem::UniformIndex(data->shader, "model");
        }

        data->WorldCamera = CameraSystem::GetDefault();

        if (!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, self, RenderViewOnEvent)) {
            MERROR("Не удалось обработать требуемое событие обновления, создание не удалось.");
            return false;
        }
        return true;
    }

    MERROR("RenderViewEditorWorld::OnRegistered — Требуется действительный указатель на представление.");
    return false;
}

void RenderViewEditorWorld::Destroy(RenderView *self)
{
    // Отменить регистрацию на мероприятии.
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, self, RenderViewOnEvent);

    MemorySystem::Free(self->data, sizeof(sRenderViewEditorWorld), Memory::Renderer);
    self->data = nullptr;
}

void RenderViewEditorWorld::Resize(RenderView* self, u32 width, u32 height)
{
    // Проверьте, отличается ли значение. Если да, пересоздайте проекционную матрицу.
    if (width != self->width || height != self->height) {
        auto data = (sRenderViewEditorWorld*)self->data;

        self->width = width;
        self->height = height;
        f32 aspect = (f32)self->width / self->height;
        data->ProjectionMatrix = Matrix4D::MakeFrustumProjection(data->fov, aspect, data->NearClip, data->FarClip);

        for (u32 i = 0; i < self->RenderpassCount; ++i) {
            self->passes[i].RenderArea.x = 0;
            self->passes[i].RenderArea.y = 0;
            self->passes[i].RenderArea.z = width;
            self->passes[i].RenderArea.w = height;
        }
    }
}

bool RenderViewEditorWorld::BuildPacket(RenderView *self, LinearAllocator &FrameAllocator, void *data, RenderViewPacket &OutPacket)
{
    if (!self || !data) {
        MWARN("RenderViewEditorWorld::BuildPacket требует корректного указателя на вид, пакет и данные.");
        return false;
    }

    // ЗАДАЧА: использовать распределитель кадров.

    auto rvewData = (sRenderViewEditorWorld*)self->data;
    OutPacket.ProjectionMatrix = rvewData->ProjectionMatrix;
    OutPacket.ViewMatrix = rvewData->WorldCamera->GetView();

    auto PacketData = (EditorWorldPacketData*)data;
    if (PacketData->gizmo) {
        auto& geometry = PacketData->gizmo->modeData[PacketData->gizmo->mode].geometry;

        // FVec3 CameraPosition = rvewData->WorldCamera->GetPosition();
        // FVec3 GizmoPosition  = PacketData->gizmo->xform.GetPosition();
        // ЗАДАЧА: получить это из камеры/области просмотра.
        // f32 fov = Math::DegToRad(45.F);
        // f32 dist = Distance(CameraPosition, GizmoPosition);

        auto model = PacketData->gizmo->xform.GetWorld();
        // f32 FixedSize = 0.1F;  // ЗАДАЧА: сделать этот параметр настраиваемым для размера гизмо.
        f32 ScaleScalar = 1.F; // ((2.F * Math::tan(fov * 0.5F)) * dist) * FixedSize;
        PacketData->gizmo->ScaleScalar = ScaleScalar; // Сохраните копию для обнаружения попаданий.
        auto scale = Matrix4D::MakeScale(ScaleScalar);
        model = model * scale;

        GeometryRenderData RenderData;
        RenderData.model = model;
        RenderData.geometry = &geometry;
        RenderData.UniqueID = INVALID::ID;

        OutPacket.geometries.PushBack(RenderData);
    #ifdef _DEBUG
        GeometryRenderData PlaneNormalRenderData{};
        PlaneNormalRenderData.model = PacketData->gizmo->PlaneNormalLine.xform.GetWorld();
        PlaneNormalRenderData.geometry = &PacketData->gizmo->PlaneNormalLine.geometry;
        PlaneNormalRenderData.UniqueID = INVALID::ID;
        OutPacket.geometries.PushBack(PlaneNormalRenderData);
    #endif
    }

    return true;
}

bool RenderViewEditorWorld::Render(const RenderView *self, const RenderViewPacket &packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData &pFrameData)
{
    auto data = (sRenderViewEditorWorld*)self->data;
    // u32 ShaderID = data->shader->id;

    for (u32 p = 0; p < self->RenderpassCount; ++p) {
        auto& pass = self->passes[p];
        if (!RenderingSystem::RenderpassBegin(&pass, pass.targets[RenderTargetIndex])) {
            MERROR("Не удалось запустить проход RenderViewEditorWorld::Render с индексом %u.", p);
            return false;
        }

        auto shader = ShaderSystem::GetShader("Shader.Builtin.ColourShader3D");
        if (!shader) {
            MERROR("Не удалось получить шейдер colour3d.");
            return false;
        }
        ShaderSystem::Use(shader->id);

        shader->BindGlobals();
        // Globals
        bool NeedsUpdate = FrameNumber != shader->RenderFrameNumber;
        if (NeedsUpdate) {
            ShaderSystem::UniformSet(data->DebugLocations.projection, &packet.ProjectionMatrix);
            ShaderSystem::UniformSet(data->DebugLocations.view, &packet.ViewMatrix);
        }
        ShaderSystem::ApplyGlobal(NeedsUpdate);

        u32 GeometryCount = packet.geometries.Length();
        for (u32 i = 0; i < GeometryCount; ++i) {
            // ПРИМЕЧАНИЕ: Униформы на уровне экземпляра не задаются.
            auto& RenderData = packet.geometries[i];

            // Задайте матрицу модели.
            ShaderSystem::UniformSet(data->DebugLocations.model, &RenderData.model);

            // Нарисуйте её.
            RenderingSystem::DrawGeometry(RenderData);
        }

        if (!RenderingSystem::RenderpassEnd(&pass)) {
            MERROR("Не удалось завершить проход RenderViewEditorWorld::Render с индексом %u.", p);
            return false;
        }
    }

    return true;
}