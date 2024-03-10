#include "renderer.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"

#include "renderer/vulkan/vulkan_api.hpp"
#include "math/vector3d.hpp"
#include "math/vector4d.hpp"
#include "math/matrix4d.hpp"

RendererType *Renderer::ptrRenderer;

Renderer::~Renderer()
{
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
        ptrRenderer = new VulkanAPI();
        return ptrRenderer->Initialize(window, ApplicationName);
    }
    return false;
}

void Renderer::Shutdown()
{
    this->~Renderer();
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
        ptrRenderer->Resized(width, height);
    } else {
        MWARN("Средства визуализации (Renderer) не существует, чтобы принять изменение размера: %i %i", width, height);
    }
}

bool Renderer::DrawFrame(RenderPacket *packet)
{
    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (BeginFrame(packet->DeltaTime)) {
        Matrix4D projection = Matrix4::MakeFrustumProjection(Math::DegToRad(45.0f), 1280 / 720.0f, 0.1f, 1000.0f);
        static f32 z = -1.0f;
        z -= 0.005f;
        Matrix4D view = Matrix4::MakeTranslation(Vector3D<f32>{0, 0, z});

        ptrRenderer->UpdateGlobalState(projection, view, Vector3D<f32>::Zero(), Vector4D<f32>::Zero(), 0);

        // Завершите кадр. Если это не удастся, скорее всего, это будет невозможно восстановить.
        bool result = EndFrame(packet->DeltaTime);

        if (!result) {
            MERROR("Ошибка EndFrame. Приложение закрывается...");
            return FALSE;
        }
    }

    return false;
}

void *Renderer::operator new(u64 size)
{
    return LinearAllocator::Allocate(size);
}

/*void Renderer::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(Renderer), MEMORY_TAG_RENDERER);
}*/
