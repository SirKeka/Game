#pragma once
#include "renderpass.h"
#include "math/matrix4d.h"

struct Application;
struct FrameData;

enum class RendergraphSourceType {
    RenderTargetColour,
    RenderTargetDepthStencil
};

enum class RendergraphSourceOrigin {
    Global,
    Other,
    Self
};

struct RendergraphSource {
    MString name;
    RendergraphSourceType type;
    RendergraphSourceOrigin origin;
    // Массив указателей текстур.
    Texture** textures;
};

struct RendergraphSink {
    MString name;
    RendergraphSource* BoundSource;
};

struct RendergraphPassData {
    bool DoExecute;
    struct Viewport* viewport;
    Matrix4D ViewMatrix;
    Matrix4D ProjectionMatrix;
    FVec3 ViewPosition;  // ЗАДАЧА: может, это не понадобится?
    // void* ExtendedData;
};

struct RendergraphNode {
    MString name;

    RendergraphPassData PassData;

    DArray<RendergraphSource> sources;
    DArray<RendergraphSink> sinks;

    Renderpass pass;

    bool PresentsAfter;

    virtual bool Initialize() = 0;
    virtual void Destroy() = 0;
    bool (*LoadResources)(RendergraphNode* self) = nullptr;
    bool (*AttachmentPopulate)(RendergraphNode* self, RenderTargetAttachment& attachment) = nullptr;
    bool (*AttachmentTexturesRegenerate)(RendergraphNode* self, u16 width, u16 height) = nullptr;
    bool (*SourcePopulate)(RendergraphNode* self, RendergraphSource& source) = nullptr;
    virtual bool Render(FrameData& rFrameData) = 0;
};

struct MAPI Rendergraph {
    MString name;
    Application* app;

    DArray<RendergraphSource> GlobalSources;

    DArray<RendergraphNode*> passes;

    RendergraphSink BackbufferGlobalSink;

    constexpr Rendergraph() = default;
    constexpr Rendergraph(const char* name, Application* app) : name(name), app(app), GlobalSources(), passes(), BackbufferGlobalSink() {}

    bool Create(const char* name, Application& app);
    void Destroy();

    bool AddGlobalSource(const char* name, RendergraphSourceType type, RendergraphSourceOrigin origin);

    // pass functions
    bool AddPass(RendergraphNode& RPass);
    bool AddPassSource(const char* PassName, const char* SourceName, RendergraphSourceType type, RendergraphSourceOrigin origin);
    bool AddPassSink(const char* PassName, const char* SinkName);
    void DisableRederpassNode(const char* PassName);
    void EnableRenderpassNode(const char* PassName);
    bool LoadResource();

    bool PassSetSinkLinkage(const char* PassName, const char* SinkName, const char* SourcePassName, const char* SourceName);

    bool Finalize();

    bool ExecuteFrame(FrameData& rFrameData);

    bool OnResize(u16 width, u16 height);
};