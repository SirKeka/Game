#include "render_view.hpp"
#include "renderer/renderer.hpp"
#include "systems/render_view_system.hpp"

bool RenderViewOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto self = (RenderView*)ListenerInst;
    if (!self) {
        return false;
    }

    switch (code) {
        case EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED:
            RenderViewSystem::RegenerateRenderTargets(self);
            // Это должно быть использовано другими представлениями, поэтому считайте, что это _не_ обработано.
            return false;
    }

    return false;
}

RenderView::RenderView(u16 id, const Config &config)
: id(id), 
name(config.name), 
width(), height(), 
type(config.type), 
RenderpassCount(config.PassCount), 
passes(MMemory::TAllocate<Renderpass>(Memory::Renderer, RenderpassCount)), 
CustomShaderName(config.CustomShaderName) {
    for (u32 i = 0; i < RenderpassCount; ++i) {
        auto& PassConfig = config.passes[i];
        // Создаем проходы рендеринга в соответствии с конфигурацией.
        if (!Renderer::RenderpassCreate(PassConfig, this->passes[i])) {
            MERROR("RenderViewSystem::Create - Не удалось создать проход рендеринга '%s'", PassConfig.name);
            return;
        }
    }
}

constexpr RenderView::RenderView(u16 id, MString &&name, u16 width, u16 height, KnownType type, u8 RenderpassCount, const char *CustomShaderName)
    : id(id),
      name(std::move(name)),
      width(width), height(height),
      type(type),
      RenderpassCount(RenderpassCount),
      passes(MMemory::TAllocate<Renderpass>(Memory::Renderer, RenderpassCount)),
      CustomShaderName(CustomShaderName) {}

RenderView::~RenderView()
{
    for (u32 i = 0; i < RenderpassCount; i++) {
        Renderer::RenderpassDestroy(&passes[i]);
    }
    id = INVALID::U16ID;
}

bool RenderView::RegenerateAttachmentTarget(u32 PassIndex, RenderTargetAttachment *attachment)
{
    MFATAL("RENDER_TARGET_ATTACHMENT_SOURCE_VIEW настроен для вложения, представление которого не поддерживает эту операцию.");
    return false;
}
