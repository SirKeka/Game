#include "scene_pass.h"
#include "core/systems_manager.h"
#include "renderer/passes/shadow_map_pass.h"
#include "renderer/rendering_system.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

ScenePass::~ScenePass()
{
    Destroy();
}

void ScenePass::Destroy()
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Уничтожьте текстурные карты/сэмплеры.
    for (u32 i = 0; i < FrameCount; ++i) {
        renderer->TextureMapReleaseResources(&ShadowMaps[i]);
    }
    renderer->RenderpassDestroy(&pass);
}

bool ScenePass::Initialize()
{    
    // Конфигурация подпрохода рендеринга
    RenderTargetAttachmentConfig attachments[2]{
        { // Привязка цвета
            .type           = RenderTargetAttachmentType::Colour,
            .source         = RenderTargetAttachmentSource::Default,
            .LoadOperation  = RenderTargetAttachmentLoadOperation::Load,
            .StoreOperation = RenderTargetAttachmentStoreOperation::Store,
            .PresentAfter   = false
        },
        { // Привязка глубины
            .type            = RenderTargetAttachmentType::Depth,
            .source          = RenderTargetAttachmentSource::Default,
            .LoadOperation   = RenderTargetAttachmentLoadOperation::DontCare,
            .StoreOperation  = RenderTargetAttachmentStoreOperation::Store,
            .PresentAfter    = false
        }
    };
    
    RenderpassConfig WorldPass{
        .depth   = 1.F,
        .stencil = 0,
        .target  = {2, attachments}
    };

    auto renderer = SystemsManager::GetRenderingSystem();
    pass.name                   = "Renderpass.Bultin.World";
    pass.ClearColour            = FVec4(0.F, 0.F, 0.2F, 1.F); 
    pass.ClearFlags             = RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer;
    pass.RenderTargetCount      = renderer->GetWindowAttachmentCount();
    pass.targets = nul_alloc(RenderTarget, pass.RenderTargetCount, Memory::Array);

    if (!renderer->RenderpassCreate(WorldPass, pass, false)) { 
        MERROR("World view — Не удалось создать проход рендеринга «%s»", pass.name.c_str());
        return false;
    };

    #pragma region Загрузка шейдера материала
    {
        const char* MaterialShaderName = "Shader.Material";
        ShaderResource MaterialShaderResource;
        if (!ResourceSystem::Load(MaterialShaderName, nullptr, MaterialShaderResource)) {
            MERROR("Не удалось загрузить ресурс шейдера материала.");
            return false;
        }
        auto& config = MaterialShaderResource.data;
        if (!(MaterialShader = ShaderSystem::CreateShader(pass, config))) {
            MERROR("Не удалось создать шейдер материала.");
            return false;
        }
        // ResourceSystem::Unload(ShaderConfigResource);
    }
    #pragma endregion
    #pragma region загрузка шейдера ландшафта.
    {
        const char* TerrainShaderName = "Shader.Builtin.Terrain";
        ShaderResource TerrainShaderResource;
        if (!ResourceSystem::Load(TerrainShaderName, nullptr, TerrainShaderResource)) {
            MERROR("Не удалось загрузить ресурс шейдера ландшафта.");
            return false;
        }
        auto& TerrainShaderConfig = TerrainShaderResource.data;
        if (!(TerrainShader = ShaderSystem::CreateShader(pass, TerrainShaderConfig))) {
            MERROR("Не удалось создать шейдер ландшафта.");
            return false;
        }
        // ResourceSystem::Unload(TerrainShaderResource);
    }
    #pragma endregion
    #pragma region Загрузка отладочного шейдера color3D.
    {
        const char* Colour3dShaderName = "Shader.Builtin.ColourShader3D";
        ShaderResource Colour3dShaderResource;
        if (!ResourceSystem::Load(Colour3dShaderName, nullptr, Colour3dShaderResource)) {
            MERROR("Не удалось загрузить ресурс шейдера color3D.");
            return false;
        }
        auto& Colour3dShaderConfig = Colour3dShaderResource.data;
        if (!(ColourShader = ShaderSystem::CreateShader(pass, Colour3dShaderConfig))) {
            MERROR("Не удалось создать шейдер color3D.");
            return false;
        }
        //ResourceSystem::Unload(&Colour3dShaderResource);
    }
    #pragma endregion
    // Получите расположение юниформ шейдера color3D.
    {
        projection = ColourShader->UniformLocation("projection");
        view       = ColourShader->UniformLocation("view");
        model      = ColourShader->UniformLocation("model");
    }

    RendergraphNode::LoadResources = LoadResources;

    return true;
}

bool ScenePass::Render(FrameData &rFrameData)
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Привяжите область просмотра
    renderer->SetActiveViewport(PassData.viewport);

    if (!renderer->RenderpassBegin(&pass, pass.targets[rFrameData.RenderTargetIndex])) {
        MERROR("Сбой при запуске сцены.");
        return false;
    }

    if (!MaterialSystem::SetIrradiance(IrradianceCubeTexture)) {
        MERROR("Не удалось установить текстуру освещенности, проверьте свойства указанной текстуры.");
    }

    for (u8 i = 0; i < MAX_CASCADE_COUNT; ++i) {
        Matrix4D LightSpace = DirectionalLightViews[i] * DirectionalLightProjections[i];
        MaterialSystem::SetDirectionalLightSpace(LightSpace, i);
        MaterialSystem::SetShadowMap(ShadowmapSource->textures[rFrameData.RenderTargetIndex], i);
    }

    #pragma region Ландшафт 
    // Используйте подходящий шейдер и примените глобальные юниформы.
    u32 TerrainGeometryCount = TerrainGeometries.Length();
    if (TerrainGeometryCount > 0) {
        ShaderSystem::Use(TerrainShader->id);

        if (RenderMode == Render::Wireframe) {
            TerrainShader->SetWireframe(true);
        } else {
            TerrainShader->SetWireframe(false);
        }

        // Применить глобальные переменные
        if (!MaterialSystem::ApplyGlobal(TerrainShader->id, rFrameData, PassData.ProjectionMatrix, PassData.ViewMatrix, CascadeSplits, PassData.ViewPosition, RenderMode)) {
            MERROR("Не удалось применить глобальные переменные для шейдера рельефа. Не удалось отрисовать кадр.");
            return false;
        }

        for (u32 i = 0; i < TerrainGeometryCount; ++i) {
            GeometryRenderData& TerrainGeometry = TerrainGeometries[i];
            Material* material = TerrainGeometry.material ? 
            TerrainGeometry.material : MaterialSystem::GetDefaultTerrainMaterial();

            // Обновите материал, если он ещё не был в этом кадре. 
            // Это предотвращает многократное обновление одного и того же материала. 
            // Его всё равно нужно привязать в любом случае, 
            // поэтому результат проверки передаётся в бэкенд, 
            // который либо обновляет внутренние привязки шейдера и привязывает их, 
            // либо только привязывает. Также необходимо проверить индекс отрисовки рендерера.
            // ЗАДАЧА: По крайней мере, на данный момент весь ландшафт использует один материал, 
            // поэтому многое из этого, вероятно, следует перенести на глобальный уровень 
            // (т. е. текстурные карты и свойства поверхности), но освещение оставить на уровне экземпляра.
            bool NeedsUpdate = material->RenderFrameNumber != rFrameData.RendererFrameNumber || material->RenderDrawIndex != rFrameData.DrawIndex;
            if (!MaterialSystem::ApplyInstance(material, rFrameData, NeedsUpdate)) {
                MWARN("Не удалось применить материал рельефа «%s». Пропуск отрисовки.", material->name);
                continue;
            } else {
                // Синхронизируйте номер кадра и индекс отрисовки.
                material->RenderFrameNumber = rFrameData.RendererFrameNumber;
                material->RenderDrawIndex = rFrameData.DrawIndex;
            }

            // Примените локальные переменные
            MaterialSystem::ApplyLocal(material, TerrainGeometry.model, rFrameData);

            // Отрисуйте его.
            renderer->DrawGeometry(TerrainGeometry);
        }
    }
    #pragma endregion

    #pragma region Статичные геометрии 
    u32 GeometryCount = geometries.Length();
    if (GeometryCount > 0) {
        // Обновите глобальные переменные для шейдеров материалов и PBR.
        if (!ShaderSystem::Use(MaterialShader->id)) {
            MERROR("Не удалось использовать шейдер материала. Не удалось отрисовать кадр.");
            return false;
        }

        // Применение глобальных переменных
        if (!MaterialSystem::ApplyGlobal(MaterialShader->id, rFrameData, PassData.ProjectionMatrix, PassData.ViewMatrix, CascadeSplits, PassData.ViewPosition, RenderMode)) {
            MERROR("Не удалось применить глобальные переменные для шейдера материала. Не удалось отрисовать кадр.");
            return false;
        }

        u32 CurrentMaterialID = INVALID::ID - 1;
        // Отрисовка геометрии.
        for (u32 i = 0; i < GeometryCount; ++i) {
            Material* material = nullptr;
            GeometryRenderData& geometry = geometries[i];
            if (geometry.material) {
                material = geometry.material;
            } else {
                material = MaterialSystem::GetDefaultMaterial();
            }

            // Повторно привязывать или обновлять материал следует только в том случае, если это новый материал. Дубликаты могут использовать уже привязанный материал.
            if (material->InternalId != CurrentMaterialID) {
                // Обновите материал, если он ещё не был в этом кадре. 
                // Это предотвратит многократное обновление одного и того же материала. 
                // В любом случае, его необходимо привязать, 
                // поэтому результат проверки передаётся в бэкенд, 
                // который либо обновляет внутренние привязки шейдера и привязывает их, 
                // либо только привязывает. Также необходимо проверить индекс отрисовки.
                bool NeedsUpdate = material->RenderFrameNumber != rFrameData.RendererFrameNumber || material->RenderDrawIndex != rFrameData.DrawIndex;
                if (!MaterialSystem::ApplyInstance(material, rFrameData, NeedsUpdate)) {
                    MWARN("Не удалось применить материал '%s'. Пропускаем отрисовку.", material->name);
                    continue;
                } else {
                    // Синхронизируйте номер кадра и индекс отрисовки.
                    material->RenderFrameNumber = rFrameData.RendererFrameNumber;
                    material->RenderDrawIndex = rFrameData.DrawIndex;
                }
                CurrentMaterialID = material->id;
            }

            // Примените локальные переменные
            MaterialSystem::ApplyLocal(material, geometry.model, rFrameData);

            // Инвертируйте при необходимости
            if (geometry.WindingInverted) {
                renderer->SetWinding(RendererWinding::Clockwise);
            }

            // Отрисуйте.
            renderer->DrawGeometry(geometry);

            // Верните обратно при необходимости
            if (geometry.WindingInverted) {
                renderer->SetWinding(RendererWinding::CounterClockwise);
            }
        }
    } 
    #pragma endregion

    #pragma region Отладочная геометрия
    // Отладка геометрии (т. е. сеток, линий, параллелепипедов, гизмо и т. д.)
    // Это проходит через ту же систему геометрии, что и всё остальное.
    u32 DebugGeometryCount = DebugGeometries.Length();
    if (DebugGeometryCount > 0) {
        ShaderSystem::Use(ColourShader->id);

        // Глобальные
        ColourShader->BindGlobals();
        ShaderSystem::UniformSet(projection, &PassData.ProjectionMatrix);
        ShaderSystem::UniformSet(view, &PassData.ViewMatrix);

        ShaderSystem::ApplyGlobal(true, rFrameData);

        // Каждая геометрия.
        for (u32 i = 0; i < DebugGeometryCount; ++i) {
            // ПРИМЕЧАНИЕ: Не нужно задавать юниформы на уровне экземпляра.
            auto& DebugGeometry = DebugGeometries[i];
            // Локальные
            ShaderSystem::BindLocal();
            ShaderSystem::UniformSet(model, &DebugGeometry.model);
            ShaderSystem::ApplyLocal(rFrameData);

            // Отрисовка.
            renderer->DrawGeometry(DebugGeometry);
        }

        // HACK: Это должно каким-то образом обрабатываться системой шейдеров в каждом кадре.
        ColourShader->RenderFrameNumber = rFrameData.RendererFrameNumber;
    }
    #pragma endregion

    if (!renderer->RenderpassEnd(&pass)) {
        MERROR("Проход сцены не завершён.");
        return false;
    }

    return true;
}

bool ScenePass::LoadResources(RendergraphNode *self)
{
    if (!self) { return false; }
    auto scenePass = ((ScenePass*)self);

    // Убедитесь, что источник(source) подключен к приемникам карты теней.
    const u32 SinkCount = self->sinks.Length();

    // Убедитесь, что к текущему sink подключен источник(source).
    for (u32 i = 0; i < SinkCount; ++i) {
        RendergraphSink& sink = self->sinks[i];
        if (sink.name == "shadowmap") {
            scenePass->ShadowmapSource = sink.BoundSource;
            break;
        }
    }
    if (!scenePass->ShadowmapSource) {
        MERROR("Необходимый источник '%s' не подключен к проходу сцены. Создание не удается.", "shadowmap");
        return false;
    }

    // Для использования текстур из исходных карт теней необходима текстурная карта (т.е. сэмплер). По одной на кадр.
    auto renderer = SystemsManager::GetRenderingSystem();
    scenePass->FrameCount = renderer->GetWindowAttachmentCount();
    CreateArray(scenePass->ShadowMaps, sizeof(TextureMap) * scenePass->FrameCount);
    for (u32 i = 0; i < scenePass->FrameCount; ++i) {
        TextureMap* sm = &scenePass->ShadowMaps[i];
        sm->RepeatU = sm->RepeatV = sm->RepeatW =  TextureRepeat::ClampToBorder;
        sm->FilterMinify = sm->FilterMagnify = TextureFilterMode::Linear;
        sm->texture = scenePass->ShadowmapSource->textures[i];
        sm->generation = INVALID::ID;

        if (!renderer->TextureMapAcquireResources(sm)) {
            MERROR("Не удалось получить ресурсы текстурной карты для карты теней в проходе сцены. Инициализация не удалась.");
            return false;
        }
    }

    return true;
}
