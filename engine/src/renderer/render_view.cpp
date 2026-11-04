#include "render_view.h"
#include "renderer/rendering_system.h"
#include "renderer/renderpass.h"

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
