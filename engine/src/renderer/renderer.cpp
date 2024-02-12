#include "renderer.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"

RendererType *Renderer::ptrRenderer;

Renderer::~Renderer()
{
    delete ptrRenderer;
}

bool Renderer::Initialize(ERendererType type, const char *ApplicationName)
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
        //ptrRenderer = dynamic_cast<VulcanAPI*> (ptrRenderer);
        ptrRenderer = new VulcanAPI(ApplicationName);
        return true;
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
}

bool Renderer::DrawFrame(RenderPacket *packet)
{
    // Если начальный кадр возвращается успешно, операции в середине кадра могут продолжаться.
    if (BeginFrame(packet->DeltaTime)) {

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
    return MMemory::Allocate(size, MEMORY_TAG_RENDERER);
}

void Renderer::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(VulcanAPI), MEMORY_TAG_RENDERER);
}
