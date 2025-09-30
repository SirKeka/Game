#include "render_view.h"
#include "renderer/rendering_system.h"
#include "systems/render_view_system.h"
#include "renderer/renderpass.h"

bool EditorWorldOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
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

// constexpr RenderView::RenderView(const char *name, u8 RenderpassCount, Renderpass *renderpass, const char *CustomShaderName,
//                                  bool (*OnRegistered)(RenderView *), void (*Resize)(RenderView *, u32, u32),
//                                  bool (*BuildPacket)(RenderView *, LinearAllocator &, void *, Packet &),
//                                  bool (*Render)(RenderView *, const Packet &, u64, u64, const FrameData &),
//                                  bool (*RegenerateAttachmentTarget)(RenderView *, u32, RenderTargetAttachment *))
//     : name(name),
//       width(), height(),
//       RenderpassCount(RenderpassCount),
//       passes(renderpass),
//       CustomShaderName(CustomShaderName),
//       OnRegistered(OnRegistered),
//       BuildPacket(BuildPacket),
//       Render(Render),
//       RegenerateAttachmentTarget(RegenerateAttachmentTarget)
// {}
