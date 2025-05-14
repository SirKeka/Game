#include "render_view.h"
#include "renderer/rendering_system.h"
#include "systems/render_view_system.h"

bool RenderViewOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto self = (RenderView*)ListenerInst;
    if (!self) {
        return false;
    }

    switch (code) {
        case EventSystem::DefaultRendertargetRefreshRequired:
            RenderViewSystem::RegenerateRenderTargets(self);
            // Это должно быть использовано другими представлениями, поэтому считайте, что это _не_ обработано.
            return false;
    }

    return false;
}

constexpr RenderView::RenderView(const char *name, u16 width, u16 height, u8 RenderpassCount, const char *CustomShaderName)
    :
      name(name),
      width(width), height(height),
      RenderpassCount(RenderpassCount),
      passes(MemorySystem::TAllocate<Renderpass>(Memory::Renderer, RenderpassCount)),
      CustomShaderName(CustomShaderName) {}

RenderView::~RenderView()
{
    for (u32 i = 0; i < RenderpassCount; i++) {
        RenderingSystem::RenderpassDestroy(&passes[i]);
    }
}

bool RenderView::RegenerateAttachmentTarget(u32 PassIndex, RenderTargetAttachment *attachment)
{
    MFATAL("RENDER_TARGET_ATTACHMENT_SOURCE_VIEW настроен для вложения, представление которого не поддерживает эту операцию.");
    return false;
}
