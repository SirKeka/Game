#include "renderer.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/vulkan/vulkan_api.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"


RendererType *Renderer::ptrRenderer;
/*f32 Renderer::NearClip = 0.1f;
f32 Renderer::FarClip = 1000.f;
Matrix4D Renderer::projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, NearClip, FarClip);
Matrix4D Renderer::view = Matrix4::MakeTranslation(Vector3D<f32>{0, 0, -30.f});*/

Material *Renderer::TestMaterial = nullptr;

Renderer::~Renderer()
{

    //TODO: временно
    //bool (Renderer::*OnDebugEvent(u16, void*, void*, EventContext)) = ;
    Event::GetInstance()->Unregister(EVENT_CODE_DEBUG0, nullptr, EventOnDebugEvent);
    //TODO: временно

    delete ptrRenderer; //TODO: Unhandled exception at 0x00007FFEADC9B93C (engine.dll) in testbed.exe: 0xC0000005: Access violation reading location 0x0000000000000000.
}

bool Renderer::Initialize(MWindow* window, const char *ApplicationName, ERendererType type)
{
    /*switch (type)
    {
    case RENDERER_TYPE_VULKAN:
        //ptrRenderer = dynamic_cast<VulcanAPI> (ptrRenderer);
        ptrRenderer = new VulcanAPI(ApplicationName);
        break;
    case RENDERER_TYPE_DIRECTX:

        break;
    case RENDERER_TYPE_OPENGL:

        break;
    }*/
    if(type == RENDERER_TYPE_VULKAN) {
        //ptrRenderer = dynamic_cast<VulkanAPI*> (ptrRenderer);

        // TODO: временно
        Event::GetInstance()->Register(EVENT_CODE_DEBUG0, nullptr, EventOnDebugEvent);
        // TODO: временно

        ptrRenderer = new VulkanAPI();
        // Возьмите указатель на текстуры по умолчанию для использования в серверной части.
        ptrRenderer->Initialize(window, ApplicationName);
        NearClip = 0.1f;
        FarClip = 1000.f;
        projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, NearClip, FarClip);
        view = Matrix4::MakeTranslation(Vector3D<f32>{0, 0, -30.f});
        view.Inverse();

        return true;
    }
    return false;
}

void Renderer::Shutdown()
{
    //this->~Renderer();
}

bool Renderer::BeginFrame(f32 DeltaTime)
{
    return ptrRenderer->BeginFrame(DeltaTime);
}

bool Renderer::EndFrame(f32 DeltaTime)
{
    bool result = ptrRenderer->EndFrame(DeltaTime);
    ptrRenderer->FrameNumber++;
    return result;
}

void Renderer::OnResized(u16 width, u16 height)
{
    if (ptrRenderer) {
        projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), width / (f32)height, NearClip, FarClip);
        ptrRenderer->Resized(width, height);
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool Renderer::DrawFrame(RenderPacket *packet)
{
    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (BeginFrame(packet->DeltaTime)) {
        ptrRenderer->UpdateGlobalState(projection, view, Vector3D<f32>::Zero(), Vector4D<f32>::Zero(), 0);

        Matrix4D model = Matrix4::MakeIdentity();
        // angle += 0.001f;
        // quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        // mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());
        GeometryRenderData data = {};
        data.model = model;

        // TODO: временно.
        // Grab the default if does not exist.
        if (!TestMaterial) {
            // Automatic config
            this->TestMaterial = MaterialSystem::Instance()->Acquire("test_material");
            if (!this->TestMaterial) {
                MWARN("Автоматическая загрузка материала не удалась, и был выполнен возврат к материалу по умолчанию, выполненному вручную.");
                // Ручная настройка
                MaterialConfig config;
                MString::nCopy(config.name, "test_material", MATERIAL_NAME_MAX_LENGTH);
                config.AutoRelease = false;
                config.DiffuseColour = Vector4D<f32>::One();  // белый
                MString::nCopy(config.DiffuseMapName, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
                this->TestMaterial = MaterialSystem::Instance()->AcquireFromConfig(config);
            }
        }

        //data.material = this->TestMaterial;
        ptrRenderer->DrawGeometry(data);

        // Завершите кадр. Если это не удастся, скорее всего, это будет невозможно восстановить.
        bool result = EndFrame(packet->DeltaTime);

        if (!result) {
            MERROR("Ошибка EndFrame. Приложение закрывается...");
            return FALSE;
        }
    }

    return false;
}

VulkanAPI *Renderer::GetRenderer()
{
    return dynamic_cast<VulkanAPI*>(ptrRenderer);
}

bool Renderer::CreateMaterial(Material *material)
{
    return ptrRenderer->CreateMaterial(material);
}

void Renderer::DestroyMaterial(Material *material)
{
    ptrRenderer->DestroyMaterial(material);
}

bool Renderer::Load(GeometryID* gid, u32 VertexCount, const Vertex3D* vertices, u32 IndexCount, const u32* indices)
{
    return ptrRenderer->Load(gid, VertexCount, vertices, IndexCount, indices);
}

void Renderer::Unload(GeometryID *gid)
{
}

void Renderer::SetView(Matrix4D view)
{
    this->view = view;
}

void *Renderer::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size);
}

// TODO: Врменно
bool Renderer::EventOnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext data)
{
    const char* names[3] = {
        "asphalt",
        "iris",
        "uvgrid"};
    static i8 choice = 2;

    // Сохраните старое имя.
    MString OldName = names[choice];

    choice++;
    choice %= 3;

    // Приобретите новую текстуру.
    TestMaterial->DiffuseMap.texture = TextureSystem::Instance()->Acquire(names[choice], true);
    if (TestMaterial->DiffuseMap.texture) {
        MWARN("Event::OnDebugEvent нет текстуры! используется по умолчанию");
        TestMaterial->DiffuseMap.texture = TextureSystem::Instance()->GetDefaultTexture();
    }
    

    // Удалите старую текстуру.
    TextureSystem::Instance()->Release(OldName.c_str());
    return true;
}
// TODO: Временно
