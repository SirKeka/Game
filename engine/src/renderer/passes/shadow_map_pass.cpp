#include "shadow_map_pass.h"

#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "renderer/viewport.h"
#include "resources/shader.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"

struct ShadowMapPassShaderLocations {
    u16 ProjectionsLocation;
    u16 ViewsLocation;
    u16 ModelLocation;
    u16 ColourMapLocation;
    u32 CascadeIndexLocation;
};

struct ShadowShaderInstanceData {
    u64 RenderFrameNumber;
    u8 RenderDrawIndex;
};

struct CascadeResources {
    // Одна цель на кадр.
    RenderTarget* targets;
};

struct ShadowmapPass::Impl {
    // Конфигурация
    u16 resolution;

    Shader* shader;
    // Индексы привязки шейдера
    ShadowMapPassShaderLocations locations;

    // Пользовательская проекционная матрица для прохода теней.
    Viewport CameraViewport;

    Texture* DepthTextures;

    // По одному на каждый каскад. CascadeResources
    CascadeResources cascades[MAX_CASCADE_COUNT];

    // Отслеживание обновлений экземпляров для каждого кадра.
    bool* InstanceUpdated;
    u32 InstanceCount;
    // Карта по умолчанию, используемая при недоступности материалов.
    TextureMap DefaultColourMap;
    u32 DefaultInstanceID;
    u64 DefaultInstanceFrameNumber;
    u8 DefaultInstanceDrawIndex;

    // Отслеживание данных по каждому экземпляру.
    DArray<ShadowShaderInstanceData> instances;

    // Для ландшафтов используйте отдельную информацию о шейдерах/экземплярах.
    Shader* tShader;
    ShadowMapPassShaderLocations TerrainLocations;
    TextureMap DefaultTerrainColourMap;
    u64 TerrainInstanceFrameNumber;
    u32 TerrainInstanceID;
    u8 TerrainInstanceDrawIndex;

    void *operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Renderer); }
    void operator delete(void *ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Renderer); }
};

bool ShadowmapPass::Create(const char* name, u16 resolution)
{
    this->name = name;
    void* mem = MemorySystem::Allocate(sizeof(Impl) + sizeof(Data), Memory::Renderer, true);
    pImpl = (Impl*)mem;
    
    pImpl->resolution = resolution;

    ExtendedData = (Data*)(pImpl + 1);

    // Указатели на пользовательские функции.
    RendergraphNode::AttachmentPopulate = ShadowmapPass::AttachmentPopulate;
    RendergraphNode::LoadResources      = ShadowmapPass::LoadResources;
    RendergraphNode::SourcePopulate     = ShadowmapPass::SourcePopulate;
    return true;
}

void ShadowmapPass::Destroy()
{
    if (pImpl) {
        auto renderer = SystemsManager::GetRenderingSystem();
        // Уничтожить прикрепленные объекты, по одному на кадр.
        u8 AttachmentCount = renderer->GetWindowAttachmentCount();

        // Прикрепленные объекты прохода рендеринга.
        for (u32 i = 0; i < MAX_CASCADE_COUNT; ++i) {
            CascadeResources& cascade = pImpl->cascades[i];
            // Цели на кадр
            for (u32 f = 0; f < AttachmentCount; ++f) {
                // Один целевой объект рендеринга на проход
                RenderTarget& target = cascade.targets[f];

                // Создать базовый целевой объект рендеринга.
                renderer->RenderTargetDestroy(target, true);
            }
            MemorySystem::Free(cascade.targets, sizeof(RenderTarget) * AttachmentCount, Memory::Array);
        }

        for (u8 i = 0; i < AttachmentCount; ++i) {
            renderer->Unload(&pImpl->DepthTextures[i]);
        }
        MemorySystem::Free(pImpl->DepthTextures, sizeof(void*) * AttachmentCount, Memory::Renderer);

        renderer->TextureMapReleaseResources(&pImpl->DefaultColourMap);
        renderer->TextureMapReleaseResources(&pImpl->DefaultTerrainColourMap);
        renderer->ShaderReleaseInstanceResources(pImpl->shader, pImpl->DefaultInstanceID);
        renderer->ShaderReleaseInstanceResources(pImpl->tShader, pImpl->TerrainInstanceID);

        // Уничтожить расширенные данные.
        // if (ExtendedData) {
        //     MemorySystem::Free(ExtendedData, sizeof(Data), Memory::Renderer);
        //     ExtendedData = nullptr;
        // }

        // Destroy the pass.
        renderer->RenderpassDestroy(&pass);
        MemorySystem::Free(pImpl, sizeof(Impl) + sizeof(Data), Memory::Renderer);
        pImpl = nullptr;
    }
}

bool ShadowmapPass::Initialize()
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Создайте привязки глубины, по одной на кадр.
    u8 FrameCount = renderer->GetWindowAttachmentCount();

    pImpl->DepthTextures = nul_alloc(Texture, FrameCount, Memory::Renderer);

    for (u8 i = 0; i < FrameCount; ++i) {
        // Глубина
        Texture& dt = pImpl->DepthTextures[i];
        dt.type = TextureType::_2DArray;
        dt.flags |= Texture::Depth | Texture::IsWriteable;
        dt.width = pImpl->resolution;
        dt.height = pImpl->resolution;
        dt.ArraySize = MAX_CASCADE_COUNT;
        MString::Format(dt.name, "shadowmap_pass_res_%u_idx_%u_depth_texture", pImpl->resolution, i);
        dt.MipLevels = 1;
        dt.ChannelCount = 4;
        dt.generation = INVALID::ID;
        renderer->LoadTextureWriteable(&dt);
    }

    #pragma region Настройка прохода рендеринга.
    // Привязка глубины.
    RenderTargetAttachmentConfig attachment{
        .type           = RenderTargetAttachmentType::Depth,
        .source         = RenderTargetAttachmentSource::Self,  // Это свойство управляет привязкой.
        .LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare,
        .StoreOperation = RenderTargetAttachmentStoreOperation::Store,
        .PresentAfter   = true
    };

    RenderpassConfig ShadowmapPassConfig {
        .depth = 1.F,
        .stencil = 0,
        .target = {1, &attachment}
    };

    pass.ClearColour.z = 0.2F; pass.ClearColour.w = 1.F;
    pass.ClearFlags = RenderpassClearFlag::DepthBuffer;
    pass.name = "Renderpass.Shadowmap";
    pass.RenderTargetCount = FrameCount;
    CreateArray(pass.targets, sizeof(RenderTarget) * pass.RenderTargetCount);

    if (!renderer->RenderpassCreate(ShadowmapPassConfig, pass, false)) {
        MERROR("Проход рендеринга карты теней — Не удалось создать проход рендеринга карты теней.");
        return false;
    }
    #pragma endregion

    // Загрузка шейдера карты теней. Попытка получить уже загруженный шейдер, если он не существует.
    const char* ShadowmapShaderName = "Shader.Shadowmap";
    if (!(pImpl->shader = ShaderSystem::GetShader(ShadowmapShaderName))) {
        MTRACE("Шейдер '%s' не существует. Попытка его загрузки...", ShadowmapShaderName);
        ShaderResource ShadowmapShaderConfigResource;
        if (!ResourceSystem::Load(ShadowmapShaderName, nullptr, ShadowmapShaderConfigResource)) {
            MERROR("Не удалось загрузить ресурс шейдера карты теней.");
            return false;
        }
        ShaderConfig& ssConfig = ShadowmapShaderConfigResource.data;
        if (!(pImpl->shader = ShaderSystem::CreateShader(pass, ssConfig))) {
            MERROR("Не удалось создать шейдер карты теней.");
            return false;
        }
    } else {
        MTRACE("Шейдер '%s' уже существует, используем его.", ShadowmapShaderName);
    }

    {
        ShadowMapPassShaderLocations& loc = pImpl->locations;
        loc.ProjectionsLocation  = pImpl->shader->UniformLocation("projections");
        loc.ViewsLocation        = pImpl->shader->UniformLocation("views");
        loc.ModelLocation        = pImpl->shader->UniformLocation("model");
        loc.CascadeIndexLocation = pImpl->shader->UniformLocation("cascade_index");
        loc.ColourMapLocation    = pImpl->shader->UniformLocation("colour_map");
    }

    // Шейдер карты теней местности.
    const char* tssName = "Shader.ShadowmapTerrain";
    if (!(pImpl->tShader = ShaderSystem::GetShader(tssName))) {
        MTRACE("Шейдер '%s' не существует. Попытка его загрузки...", tssName);
        ShaderResource tsscResource;
        if (!ResourceSystem::Load(tssName, nullptr, tsscResource)) {
            MERROR("Не удалось загрузить ресурс шейдера карты теней местности.");
            return false;
        }
        ShaderConfig& tssConfig = tsscResource.data;
        if (!(pImpl->tShader = ShaderSystem::CreateShader(pass, tssConfig))) {
            MERROR("Не удалось создать шейдер карты теней местности.");
            return false;
        }
    } else {
        MTRACE("Шейдер '%s' уже существует, используем его.", tssName);
    }

    ShadowMapPassShaderLocations& tloc = pImpl->TerrainLocations;
    tloc.ProjectionsLocation  = pImpl->tShader->UniformLocation("projections");
    tloc.ViewsLocation        = pImpl->tShader->UniformLocation("views");
    tloc.ModelLocation        = pImpl->tShader->UniformLocation("model");
    tloc.CascadeIndexLocation = pImpl->tShader->UniformLocation("cascade_index");
    tloc.ColourMapLocation    = pImpl->tShader->UniformLocation("colour_map");

    return true;
}

bool ShadowmapPass::LoadResources(RendergraphNode *self)
{
    if (!self) { return false; }
    auto data = ((ShadowmapPass*)self)->pImpl;

    // Создайте текстурную карту, которая будет использоваться повсеместно для образца прозрачности диффузного/альбедо.
    {
        TextureMap& dcm = data->DefaultColourMap;
        dcm.MipLevels = 1;
        dcm.generation = INVALID::ID;
        dcm.RepeatU = dcm.RepeatV = dcm.RepeatW = TextureRepeat::ClampToEdge;
        dcm.FilterMinify = dcm.FilterMagnify = TextureFilterMode::Linear;
        
        // Возьмите текстуру по умолчанию для создания текстурной карты по умолчанию.
        dcm.texture = TextureSystem::GetDefaultTexture(Texture::Diffuse);
    }

    // Создайте текстурную карту, которая будет использоваться повсеместно для образца прозрачности диффузного/альбедо.
    {
        TextureMap& dtcm = data->DefaultTerrainColourMap;
        dtcm.MipLevels = 1;
        dtcm.generation = INVALID::ID;
        dtcm.RepeatU = dtcm.RepeatV = dtcm.RepeatW = TextureRepeat::ClampToEdge;
        dtcm.FilterMinify = dtcm.FilterMagnify = TextureFilterMode::Linear;
        
        // Выберите текстуру по умолчанию для карты текстур местности по умолчанию.
        dtcm.texture = data->DefaultColourMap.texture;
    }

    auto renderer = SystemsManager::GetRenderingSystem();
    // Получение ресурсов для карты текстур по умолчанию.
    if (!renderer->TextureMapAcquireResources(&data->DefaultColourMap)) {
        MERROR("Не удалось получить ресурсы карты текстур для карты цветов по умолчанию в проходе карты теней.");
        return false;
    }

    // Получение ресурсов для карты текстур местности по умолчанию.
    if (!renderer->TextureMapAcquireResources(&data->DefaultTerrainColourMap)) {
        MERROR("Не удалось получить ресурсы карты текстур для карты цветов местности по умолчанию в проходе карты теней.");
        return false;
    }

    // Зарезервируйте идентификатор экземпляра для "материала" по умолчанию, который будет использоваться для рендеринга.
    {
        TextureMap* maps[1] = { &data->DefaultColourMap };
        Shader* shader = data->shader;
        u16 AtlasLocation = shader->uniforms[shader->InstanceSamplerIndices[0]].index;
        // Известно количество карт этого типа.
        ShaderInstanceUniformTextureConfig ColourTexture {
            .UniformLocation = AtlasLocation,
            .TextureMapCount = 1, 
            .TextureMaps = maps
        };
        
        ShaderInstanceResourceConfig InstanceResourceConfig {
            .UniformConfigCount = 1,
            .UniformConfigs = &ColourTexture
        }; renderer->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, data->DefaultInstanceID);
    }

    // Зарезервируйте идентификатор экземпляра для "материала" по умолчанию, который будет использоваться для рендеринга.
    {
        TextureMap* TerrainMaps[1] = { &data->DefaultTerrainColourMap };
        Shader* shader = data->tShader;
        u16 AtlasLocation = shader->uniforms[shader->InstanceSamplerIndices[0]].index;
        // Известно количество карт этого типа.
        ShaderInstanceUniformTextureConfig ColourTexture {
            .UniformLocation = AtlasLocation,
            .TextureMapCount = 1, 
            .TextureMaps = TerrainMaps
        };
        
        ShaderInstanceResourceConfig InstanceResourceConfig {
            .UniformConfigCount = 1,
            .UniformConfigs = &ColourTexture
        }; renderer->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, data->TerrainInstanceID);
    }

    // ПРИМЕЧАНИЕ: Настройте область просмотра по умолчанию. Единственный компонент, используемый для этого, — 
    // это базовый прямоугольник области просмотра, но его необходимо задать рендерером перед началом прохода рендеринга.
    // В данном случае проекционная матрица не используется, поэтому поле зрения и плоскости отсечения не имеют значения.
    Viewport& vp = data->CameraViewport;
    vp.rect = Rect2D(0, 0, data->resolution, data->resolution);
    vp.ProjectionMatrixType = RendererProjectionMatrixType::Orthographic;
    if (!vp.Create()) {
        MERROR("Не удалось создать область просмотра для прохода карты теней.");
        return false;
    }
    // Создайте точки привязки глубины, по одной на кадр.
    u8 FrameCount = renderer->GetWindowAttachmentCount();
    // Точки привязки прохода рендеринга.
    for (u32 i = 0; i < MAX_CASCADE_COUNT; ++i) {
        CascadeResources& cascade = data->cascades[i];
        // Целей на кадр.
        CreateArray(cascade.targets, sizeof(RenderTarget) * FrameCount, true);
            for (u32 f = 0; f < FrameCount; ++f) {
            // Одна цель рендеринга на проход.
            RenderTarget& target = cascade.targets[f];
            target.AttachmentCount = 1;
            // 2 точки привязки на цель (цвет, глубина).
            CreateArray(target.attachments, sizeof(RenderTargetAttachment) * target.AttachmentCount);
            target.attachments[0].type = RenderTargetAttachmentType::Depth;
            target.attachments[0].source = RenderTargetAttachmentSource::Self;
            target.attachments[0].LoadOperation = RenderTargetAttachmentLoadOperation::DontCare;
            target.attachments[0].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
            target.attachments[0].PresentAfter = true;
            target.attachments[0].texture = &data->DepthTextures[f];

            // Создайте базовый целевой объект рендеринга.
            renderer->RenderTargetCreate(
                target.AttachmentCount,
                nullptr, // target.attachments,
                &self->pass,
                data->resolution,
                data->resolution,
                i,
                target);
        }
    }

    return true;
}

bool ShadowmapPass::Render(FrameData &rFrameData)
{
    auto renderer = SystemsManager::GetRenderingSystem();
    // Привяжите внутренний видовой экран — не используйте тот, который указан в данных прохода.
    renderer->SetActiveViewport(&pImpl->CameraViewport);

    for (u32 p = 0; p < MAX_CASCADE_COUNT; ++p) {
        // ShadowMapCascadeData& cascade = ExtendedData->cascades[p];
        /* if (!renderer->RenderpassBegin(pass, pass.targets[rFrameData.RenderTargetIndex])) {
            MERROR("Не удалось запустить проход Shadowmap.");
            return false;
        } */

        if (!renderer->RenderpassBegin(&pass, pImpl->cascades[p].targets[rFrameData.RenderTargetIndex])) {
            MERROR("Не удалось запустить проход Shadowmap.");
            return false;
        }

        // Используйте стандартный шейдер карты теней.
        ShaderSystem::Use(pImpl->shader->id);

        ShadowMapPassShaderLocations& location = pImpl->locations;

        // Примените глобальные переменные один раз для каждого каскада.
        bool NeedsUpdate = p == 0;
        if (NeedsUpdate) {
            renderer->ShaderBindGlobals(pImpl->shader);
            for (u32 i = 0; i < MAX_CASCADE_COUNT; ++i) {
                if (!ShaderSystem::UniformSetArrayed(location.ProjectionsLocation, i, &ExtendedData->cascades[i].projection)) {
                    MERROR("Не удалось применить униформу проекции карты теней.");
                    return false;
                }
                if (!ShaderSystem::UniformSetArrayed(location.ViewsLocation, i, &ExtendedData->cascades[i].view)) {
                    MERROR("Не удалось применить униформу вида карты теней.");
                    return false;
                }
            }
        }
        ShaderSystem::ApplyGlobal(NeedsUpdate, rFrameData);

        u32 GeometryCount = ExtendedData->geometries.Length();
        u32 TerrainGeometryCount = ExtendedData->TerrainGeometries.Length();

        // Убедитесь, что для этого кадра достаточно ресурсов экземпляра.
        // Это достигается путем выбора наивысшего идентификатора экземпляра материала и 
        // использования его в качестве счетчика. Это гарантирует наличие достаточного 
        // количества ресурсов для кадра, а также позволяет быстро сопоставить экземпляр 
        // шейдера для привязки текстур, а также отслеживать обновления экземпляров для каждого кадра.
        u32 HighestID = 0;
        for (u32 i = 0; i < GeometryCount; ++i) {
            Material* material = ExtendedData->geometries[i].material;
            if (material->InternalId > HighestID) {
                // ПРИМЕЧАНИЕ: +1, чтобы учесть, что первый идентификатор используется экземпляром по умолчанию.
                HighestID = material->InternalId + 1;
            }
        }
        // Ландшафты будут немного отличаться, поскольку образец текстуры на самом деле не нужен, 
        // так как ландшафты никогда не бывают прозрачными. Поэтому потребуется только еще один экземпляр, 
        // который может использовать ту же самую текстуру белого цвета по умолчанию в качестве образца.
        HighestID++;

        if (HighestID > pImpl->InstanceCount) {
            if (pImpl->instances) {
                pImpl->instances.Clear();
            }
            pImpl->instances.Resize(HighestID + 1);
            // При необходимости получите дополнительные ресурсы, начиная с предыдущего пикового уровня.
            for (u32 i = pImpl->InstanceCount; i < HighestID; i++) {
                u32 InstanceID;

                // Используйте одну и ту же карту для всех.
                TextureMap* maps[1] = { &pImpl->DefaultColourMap };
                Shader* shader = pImpl->shader;

                // Количество карт этого типа известно.
                ShaderInstanceUniformTextureConfig ColourTexture{ 
                    .UniformLocation = shader->uniforms[shader->InstanceSamplerIndices[0]].index,
                    .TextureMapCount = 1,
                    .TextureMaps = maps 
                };
                
                ShaderInstanceResourceConfig InstanceResourceConfig{
                    .UniformConfigCount = 1,
                    .UniformConfigs = &ColourTexture 
                };
                renderer->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, InstanceID);

                ShadowShaderInstanceData& instance = pImpl->instances[InstanceID];
                instance.RenderFrameNumber = INVALID::U64ID;
                instance.RenderDrawIndex = INVALID::U8ID;
            }
            pImpl->InstanceCount = HighestID;
        }

        // Статические геометрические формы.
        for (u32 i = 0; i < GeometryCount; ++i) {
            GeometryRenderData& grd = ExtendedData->geometries[i];
            Material* material = grd.material;
            // auto& g = cascade.geometries[i].geometry;

            u32 BindID = INVALID::ID;
            TextureMap* BindMap = nullptr;
            u64* RenderNumber = nullptr;
            u8* DrawIndex = nullptr;

            // Определите, какие привязки использовать.
            if (material && material->maps) {
                // Используйте внутренний идентификатор текущего материала.
                // ПРИМЕЧАНИЕ: +1, чтобы учесть, что первый идентификатор занят экземпляром по умолчанию.
                BindID = material->InternalId + 1;
                // Используйте карту диффузии/альбедо текущего материала.
                BindMap = &material->maps[0];
                // ПРИМЕЧАНИЕ: невозможно обновить номер кадра/индекс отрисовки _материала_, поскольку он по-прежнему должен использоваться для фактического рендеринга сцены.
                ShadowShaderInstanceData& instance = pImpl->instances[material->InternalId + 1];
                RenderNumber = &instance.RenderFrameNumber;
                DrawIndex = &instance.RenderDrawIndex;
            } else {
                // Используйте экземпляр по умолчанию.
                BindID = pImpl->DefaultInstanceID;
                // Используйте карту цвета по умолчанию.
                BindMap = &pImpl->DefaultColourMap;
                RenderNumber = &pImpl->DefaultInstanceFrameNumber;
                DrawIndex = &pImpl->DefaultInstanceDrawIndex;
            }

            // LEFTOFF: Этот шейдер используется 4 раза за кадр, а это значит, что его нужно обновлять 4 раза, 
            // чего сделать нельзя, поскольку дескрипторы уже будут обновлены.
            // Это означает, что теперь необходимо выполнить это за один проход, 
            // если только я каким-то образом не найду способ сделать это без дублирования дескрипторов.
            // Это также потребуется для перемещения глобальных переменных обратно на их законные места.
            // Краткосрочным решением может быть создание массива матриц и их индексация по номеру прохода, 
            // что, возможно, лучше сделать до полной рефакторизации. Тогда их можно будет обновлять один раз в начале.

            // Чушь.
            bool NeedsUpdate = *RenderNumber != rFrameData.RendererFrameNumber || *DrawIndex != rFrameData.DrawIndex;

            // Используйте привязки.
            ShaderSystem::BindInstance(BindID);
            if (!ShaderSystem::UniformSet(location.ColourMapLocation, BindMap)) {
                MERROR("Не удалось применить униформу color_map карты теней (shadowmap СolorЬap uniform) к статической геометрии.");
                return false;
            }
            ShaderSystem::ApplyInstance(NeedsUpdate, rFrameData);

            // Синхронизируйте номер кадра и индекс отрисовки.
            *RenderNumber = rFrameData.RendererFrameNumber;
            *DrawIndex = rFrameData.DrawIndex;

            // примените локали
            ShaderSystem::BindLocal();
            ShaderSystem::UniformSet(location.ModelLocation, &grd.model);
            ShaderSystem::UniformSet(location.CascadeIndexLocation, &p);
            ShaderSystem::ApplyLocal(rFrameData);

            // При необходимости переверните.
            if (grd.WindingInverted) {
                renderer->SetWinding(RendererWinding::Clockwise);
            }

            // Отрисуйте
            renderer->DrawGeometry(grd);

            // При необходимости верните все как было.
            if (grd.WindingInverted) {
                renderer->SetWinding(RendererWinding::CounterClockwise);
            }
        }

        // Рельеф — используйте специальный шейдер карты теней рельефа.
        ShaderSystem::Use(pImpl->tShader->id);

        ShadowMapPassShaderLocations& tloc = pImpl->TerrainLocations;

        // Применяйте глобальные переменные один раз для каждого каскада.
        renderer->ShaderBindGlobals(pImpl->tShader);
        if (NeedsUpdate) {
            for (u32 i = 0; i < MAX_CASCADE_COUNT; ++i) {
                ShadowMapCascadeData& cascades = ExtendedData->cascades[i];
                // ПРИМЕЧАНИЕ: используется внутренняя матрица проекции, а не та, которая была передана.
                if (!ShaderSystem::UniformSetArrayed(tloc.ProjectionsLocation, i, &cascades.projection)) {
                    MERROR("Не удалось применить униформу проекции карты теней рельефа.");
                    return false;
                }
                if (!ShaderSystem::UniformSetArrayed(tloc.ViewsLocation, i, &cascades.view)) {
                    MERROR("Не удалось применить униформу отображения карты теней местности.");
                    return false;
                }
            }
        }
        ShaderSystem::ApplyGlobal(NeedsUpdate, rFrameData);

        for (u32 i = 0; i < TerrainGeometryCount; ++i) {
            GeometryRenderData& terrain = ExtendedData->TerrainGeometries[i];

            // Просто отрисуйте их, используя экземпляр по умолчанию и текстурную карту.
            TextureMap* BindMap = &pImpl->DefaultTerrainColourMap;
            u64& RenderNumber = pImpl->TerrainInstanceFrameNumber;
            u8& DrawIndex = pImpl->TerrainInstanceDrawIndex;

            bool NeedsUpdate = RenderNumber != rFrameData.RendererFrameNumber || DrawIndex != rFrameData.DrawIndex;

            ShaderSystem::BindInstance(pImpl->TerrainInstanceID);
            if (!ShaderSystem::UniformSet(tloc.ColourMapLocation, BindMap)) {
                MERROR("Не удалось применить унифицированную цветовую карту тени к геометрии местности.");
                return false;
            }
            ShaderSystem::ApplyInstance(NeedsUpdate, rFrameData);

            // Синхронизируйте номер кадра и индекс отрисовки.
            RenderNumber = rFrameData.RendererFrameNumber;
            DrawIndex = rFrameData.DrawIndex;

            // Apply the locals
            ShaderSystem::BindLocal();
            ShaderSystem::UniformSet(tloc.ModelLocation, &terrain.model);
            ShaderSystem::UniformSet(tloc.CascadeIndexLocation, &p);
            ShaderSystem::ApplyLocal(rFrameData);

            // Отрисуйте.
            renderer->DrawGeometry(terrain);
        }

        if (!renderer->RenderpassEnd(&pass)) {
            MERROR("Завершение прохода карты теней не удалось.");
            return false;
        }

    }  // Завершение каскадного прохода

    return true;
}

bool ShadowmapPass::AttachmentPopulate(RendergraphNode *self, RenderTargetAttachment &attachment)
{
    if (!self) { return false; }

    auto data = ((ShadowmapPass*)self)->pImpl;
    if (attachment.type == RenderTargetAttachmentType::Depth) {
        attachment.texture = &data->DepthTextures[0];
        return true;
    }

    return false;
}

bool ShadowmapPass::SourcePopulate(RendergraphNode *self, RendergraphSource &source)
{
    if (!self) { return false; }

    auto data = ((ShadowmapPass*)self)->pImpl;
    u32 FrameCount = SystemsManager::GetRenderingSystem()->GetWindowAttachmentCount();
    if (!source.textures) {
        CreateArray(source.textures, sizeof(void*) * FrameCount, true);
    }
    if (source.name == "depthbuffer") {
        for (u32 i = 0; i < FrameCount; ++i) {
            source.textures[i] = &data->DepthTextures[i];
        }
        return true;
    }
    MERROR("Метод ShadowMapPass::SourcePopulate не смог заполнить источник '%s', поскольку он не распознан.", source.name.c_str());
    return false;
}
