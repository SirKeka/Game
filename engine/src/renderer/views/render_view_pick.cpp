#include "render_view_pick.h"
#include "memory/linear_allocator.hpp"
#include "core/uuid.hpp"
#include "renderer/rendering_system.h"
#include "resources/ui_text.h"
#include "systems/camera_system.hpp"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

RenderViewPick::RenderViewPick()
: RenderView(),
ColoureTargetAttachmentTexture(),
DepthTargetAttachmentTexture(),
InstanceCount(),
MouseX(), MouseY()
{
    // ПРИМЕЧАНИЕ: В этом сильно настроенном представлении точное количество проходов известно, поэтому эти предположения индекса верны.
    WorldShaderInfo.pass   = &passes[0];
    TerrainShaderInfo.pass = &passes[0];
    UiShaderInfo.pass      = &passes[1];

    // Встроенный шейдер UI Pick.
    const char* UiShaderName = "Shader.Builtin.UIPick";
    ShaderResource ConfigResource;
    if (!ResourceSystem::Load(UiShaderName, eResource::Shader, nullptr, ConfigResource)) {
        MERROR("Не удалось загрузить встроенный шейдер UI Pick.");
        return;
    }

    if (!ShaderSystem::Create(*UiShaderInfo.pass, ConfigResource.data)) {
        MERROR("Не удалось загрузить встроенный шейдер UI Pick.");
        return;
    }
    ResourceSystem::Unload(ConfigResource);
    UiShaderInfo.s = ShaderSystem::GetShader(UiShaderName);

    // Извлечь однородные местоположения
    UiShaderInfo.IdColourLocation   = ShaderSystem::UniformIndex(UiShaderInfo.s, "id_colour");
    UiShaderInfo.ModelLocation      = ShaderSystem::UniformIndex(UiShaderInfo.s, "model");
    UiShaderInfo.ProjectionLocation = ShaderSystem::UniformIndex(UiShaderInfo.s, "projection");
    UiShaderInfo.ViewLocation       = ShaderSystem::UniformIndex(UiShaderInfo.s, "view");

    // Свойства UI по умолчанию
    UiShaderInfo.NearClip = -100.F;
    UiShaderInfo.FarClip = 100.F;
    UiShaderInfo.fov = 0;
    UiShaderInfo.projection = Matrix4D::MakeOrthographicProjection(0.F, 1280.F, 720.F, 0.F, UiShaderInfo.NearClip, UiShaderInfo.FarClip);
    UiShaderInfo.view = Matrix4D::MakeIdentity();

    // Встроенный шейдер World Pick.
    const char* WorldShaderName = "Shader.Builtin.WorldPick";
    if (!ResourceSystem::Load(WorldShaderName, eResource::Shader, nullptr, ConfigResource)) {
        MERROR("Не удалось загрузить встроенный шейдер World Pick.");
        return;
    }
    
    if (!ShaderSystem::Create(*WorldShaderInfo.pass, ConfigResource.data)) {
        MERROR("Не удалось загрузить встроенный шейдер World Pick.");
        return;
    }
    ResourceSystem::Unload(ConfigResource);
    WorldShaderInfo.s = ShaderSystem::GetShader(WorldShaderName);

    // Извлечь однородные местоположения.
    WorldShaderInfo.IdColourLocation   = ShaderSystem::UniformIndex(WorldShaderInfo.s, "id_colour");
    WorldShaderInfo.ModelLocation      = ShaderSystem::UniformIndex(WorldShaderInfo.s, "model");
    WorldShaderInfo.ProjectionLocation = ShaderSystem::UniformIndex(WorldShaderInfo.s, "projection");
    WorldShaderInfo.ViewLocation       = ShaderSystem::UniformIndex(WorldShaderInfo.s, "view");

    // Свойства мира по умолчанию
    WorldShaderInfo.NearClip = 0.1F;
    WorldShaderInfo.FarClip = 4000.F;
    WorldShaderInfo.fov = Math::DegToRad(45.0f);
    WorldShaderInfo.projection = Matrix4D::MakeFrustumProjection(WorldShaderInfo.fov, 1280 / 720.F, WorldShaderInfo.NearClip, WorldShaderInfo.FarClip);
    WorldShaderInfo.view = Matrix4D::MakeIdentity();

    // Встроенный шейдер Terrain Pick.
    const char* TerrainShaderName = "Shader.Builtin.TerrainPick";
    if (!ResourceSystem::Load(TerrainShaderName, eResource::Shader, nullptr, ConfigResource)) {
        MERROR("Не удалось загрузить встроенный шейдер Terrain Pick.");
        return;
    }

    if (!ShaderSystem::Create(*TerrainShaderInfo.pass, ConfigResource.data)) {
        MERROR("Не удалось загрузить встроенный шейдер Terrain Pick.");
        return;
    }
    ResourceSystem::Unload(ConfigResource);
    TerrainShaderInfo.s = ShaderSystem::GetShader(TerrainShaderName);

    // Извлечь однородные местоположения.
    TerrainShaderInfo.IdColourLocation   = ShaderSystem::UniformIndex(TerrainShaderInfo.s, "id_colour");
    TerrainShaderInfo.ModelLocation      = ShaderSystem::UniformIndex(TerrainShaderInfo.s, "model");
    TerrainShaderInfo.ProjectionLocation = ShaderSystem::UniformIndex(TerrainShaderInfo.s, "projection");
    TerrainShaderInfo.ViewLocation       = ShaderSystem::UniformIndex(TerrainShaderInfo.s, "view");

    // Свойства ландшафта по умолчанию.
    TerrainShaderInfo.NearClip = 0.1F;
    TerrainShaderInfo.FarClip = 4000.F;
    TerrainShaderInfo.fov = Math::DegToRad(45.F);
    TerrainShaderInfo.projection = Matrix4D::MakeFrustumProjection(TerrainShaderInfo.fov, 1280 / 720.F, TerrainShaderInfo.NearClip, TerrainShaderInfo.FarClip);
    TerrainShaderInfo.view = Matrix4D::MakeIdentity();

    // Регистрация для события перемещения мыши.
    if (!EventSystem::Register(EventSystem::MouseMoved, this, OnMouseMoved)) {
        MERROR("Не удалось прослушать событие перемещения мыши, создание не удалось.");
        return;
    }

    if (!EventSystem::Register(EventSystem::DefaultRendertargetRefreshRequired, this, RenderViewOnEvent)) {
        MERROR("Не удалось прослушать требуемое событие обновления, создание не удалось.");
        return;
    }

}

RenderViewPick::~RenderViewPick()
{
    EventSystem::Unregister(EventSystem::MouseMoved, this, OnMouseMoved);
    EventSystem::Unregister(EventSystem::DefaultRendertargetRefreshRequired, this, RenderViewOnEvent);

    ReleaseShaderInstances();

    RenderingSystem::Unload(&ColoureTargetAttachmentTexture);
    RenderingSystem::Unload(&DepthTargetAttachmentTexture);
}

void RenderViewPick::Resize(u32 width, u32 height)
{
    this->width = width;
    this->height = height;

    // Пользовательский интерфейс
    UiShaderInfo.projection = Matrix4D::MakeOrthographicProjection(0.F, (f32)width, (f32)height, 0.F, UiShaderInfo.NearClip, UiShaderInfo.FarClip);

    // Мир
    f32 aspect = (f32)this->width / this->height;
    WorldShaderInfo.projection = Matrix4D::MakeFrustumProjection(WorldShaderInfo.fov, aspect, WorldShaderInfo.NearClip, WorldShaderInfo.FarClip);

    // Ландшафт
    TerrainShaderInfo.projection = Matrix4D::MakeFrustumProjection(TerrainShaderInfo.fov, aspect, TerrainShaderInfo.NearClip, TerrainShaderInfo.FarClip);

    for (u32 i = 0; i < RenderpassCount; ++i) {
        passes[i].RenderArea.x = 0;
        passes[i].RenderArea.y = 0;
        passes[i].RenderArea.z = width;
        passes[i].RenderArea.w = height;
    }
}

bool RenderViewPick::BuildPacket(LinearAllocator& FrameAllocator, void *data, Packet &OutPacket)
{
    if (!data) {
        MWARN("RenderViewPick::BuildPacket требует действительный указатель на вид, пакет и данные.");
        return false;
    }

    auto PacketData = reinterpret_cast<RenderViewPick::PacketData*>(data);

    //OutPacket.geometries = darray_create(geometry_render_data);
    OutPacket.view = this;

    // ЗАДАЧА: Получить активную камеру.
    auto WorldCamera = CameraSystem::Instance()->GetDefault();
    WorldShaderInfo.view = WorldCamera->GetView();
    TerrainShaderInfo.view = WorldCamera->GetView();

    // Установить данные пакета выбора на расширенные данные.
    PacketData->UiGeometryCount = 0;
    OutPacket.ExtendedData = FrameAllocator.Allocate(sizeof(RenderViewPick::PacketData));

    auto& WorldMeshData = *PacketData->WorldMeshData;
    const u64& WorldGeometryCount = WorldMeshData.Length();

    u32 HighestInstanceID = 0;
    // Итерировать все сетки в мировых данных.
    for (u32 i = 0; i < WorldGeometryCount; ++i) {
        // Подсчитать все геометрии как один идентификатор.
        OutPacket.geometries.PushBack(WorldMeshData[i]);
        if (WorldMeshData[i].UniqueID > HighestInstanceID) {
            HighestInstanceID = WorldMeshData[i].UniqueID;
        }
    }

    // Итерировать все ландшафты в данных мира.
    auto& TerrainMeshData = *PacketData->TerrainMeshData;
    u32 TerrainGeometryCount = TerrainMeshData.Length();

    // Итерировать все геометрии в данных ландшафта.
    for (u32 i = 0; i < TerrainGeometryCount; ++i) {
        OutPacket.TerrainGeometries.PushBack(TerrainMeshData[i]);

        // Подсчитать все геометрии как один идентификатор.
        if (TerrainMeshData[i].UniqueID > HighestInstanceID) {
            HighestInstanceID = TerrainMeshData[i].UniqueID;
        }
    }

    // Итерировать все сетки в данных пользовательского интерфейса.
    for (u32 i = 0; i < PacketData->UiMeshData.MeshCount; ++i) {
        auto m = PacketData->UiMeshData.meshes[i];
        for (u32 j = 0; j < m->GeometryCount; ++j) {
            GeometryRenderData RenderData;
            RenderData.geometry = m->geometries[j];
            RenderData.model = m->transform.GetWorld();
            RenderData.UniqueID = m->UniqueID;
            OutPacket.geometries.PushBack(RenderData);
            // OutPacket.GeometryCount++;
        }
        // Подсчитать все геометрии как один идентификатор.
        if (m->UniqueID > HighestInstanceID) {
            HighestInstanceID = m->UniqueID;
        }
    }

    // Также подсчитать тексты.
    for (u32 i = 0; i < PacketData->TextCount; ++i) {
        if (PacketData->texts[i]->UniqueID > HighestInstanceID) {
            HighestInstanceID = PacketData->texts[i]->UniqueID;
        }
    }

    u32 RequiredInstanceCount = HighestInstanceID + 1;

    // ЗАДАЧА: здесь необходимо учитывать самый высокий идентификатор, а не количество, потому что они могут пропускать и пропускают идентификаторы.
    // Проверить существование ресурсов экземпляра.
    if (RequiredInstanceCount > InstanceCount) {
        u32 diff = RequiredInstanceCount - InstanceCount;
        for (u32 i = 0; i < diff; ++i) {
            AcquireShaderInstances();
        }
    }

    // Копируем данные пакета.
    MemorySystem::CopyMem(OutPacket.ExtendedData, PacketData, sizeof(RenderViewPick::PacketData));

    return true;
}

bool RenderViewPick::Render(const Packet &packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData)
{
    u32 p = 0;
    auto* pass = &passes[p];  // Первый проход отрисовщика

    if (RenderTargetIndex == 0) {
        // Reset.
        const u64& count = InstanceUpdate.Length();
        for (u64 i = 0; i < count; ++i) {
            InstanceUpdate[i] = false;
        }

        if (!RenderingSystem::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewPick::Render Не удалось запустить индекс прохода рендеринга %u.", p);
            return false;
        }

        auto PacketData = reinterpret_cast<RenderViewPick::PacketData*>(packet.ExtendedData);

        i32 CurrentInstanceID = 0;

        // Мир
        if (!ShaderSystem::Use(WorldShaderInfo.s->id)) {
            MERROR("Не удалось использовать шейдер выбора мира. Не удалось выполнить рендеринг кадра.");
            return false;
        }

        // Применить глобальные переменные
        if (!ShaderSystem::UniformSet(WorldShaderInfo.ProjectionLocation, &WorldShaderInfo.projection)) {
            MERROR("Не удалось применить проекционную матрицу");
        }
        if (!ShaderSystem::UniformSet(WorldShaderInfo.ViewLocation, &WorldShaderInfo.view)) {
            MERROR("Не удалось применить матрицу вида");
        }
        ShaderSystem::ApplyGlobal();

        const u64& WorldGeometryCount = PacketData->WorldMeshData->Length();
        // Нарисовать геометрию. Начните с 0, так как геометрия мира добавляется первой, и остановитесь на количестве геометрий мира.
        for (u32 i = 0; i < WorldGeometryCount; ++i) {
            const auto& geo = packet.geometries[i];
            CurrentInstanceID = geo.UniqueID;

            ShaderSystem::BindInstance(CurrentInstanceID);

            // Получить цвет на основе идентификатора
            u32 r, g, b;
            Math::u32ToRGB(geo.UniqueID, r, g, b);
            FVec3 IdColour(r, g, b);
            IdColour /= 255.F;
            if (!ShaderSystem::UniformSet(WorldShaderInfo.IdColourLocation, &IdColour)) {
                MERROR("Не удалось применить цветовую униформу идентификатора.");
                return false;
            }

            bool NeedsUpdate = !InstanceUpdate[CurrentInstanceID];
            ShaderSystem::ApplyInstance(NeedsUpdate);
            InstanceUpdate[CurrentInstanceID] = true;

            // Применить локальные переменные
            if (!ShaderSystem::UniformSet(WorldShaderInfo.ModelLocation, &geo.model)) {
                MERROR("Не удалось применить матрицу модели для геометрии мира.");
            }

            // Нарисовать ее.
            RenderingSystem::DrawGeometry(packet.geometries[i]);
        }
        // Геометрии конечного мира

        //Геометрии ландшафта
        if (!ShaderSystem::Use(TerrainShaderInfo.s->id)) {
            MERROR("Не удалось использовать шейдер выбора ландшафта. Не удалось отрисовать кадр.");
            return false;
        }

        //Применить глобальные переменные
        if (!ShaderSystem::UniformSet(TerrainShaderInfo.ProjectionLocation, &TerrainShaderInfo.projection)) {
            MERROR("Не удалось применить матрицу проекции");
        }
        if (!ShaderSystem::UniformSet(TerrainShaderInfo.ViewLocation, &TerrainShaderInfo.view)) {
            MERROR("Не удалось применить матрицу вида");
        }
        ShaderSystem::ApplyGlobal();

        // Нарисовать геометрию. Начните с 0, так как геометрия ландшафта добавляется первой, и остановитесь на количестве геометрий ландшафта.
        u32 TerrainGeometryCount = PacketData->TerrainMeshData->Length();
        for (u32 i = 0; i < TerrainGeometryCount; ++i) {
            const auto& geo = packet.TerrainGeometries[i];
            CurrentInstanceID = geo.UniqueID;

            ShaderSystem::BindInstance(CurrentInstanceID);

            // Получить цвет на основе идентификатора
            u32 r, g, b;
            Math::u32ToRGB(geo.UniqueID, r, g, b);
            FVec3 IdColour(r, g, b);
            IdColour /= 255.F;
            if (!ShaderSystem::UniformSet(TerrainShaderInfo.IdColourLocation, &IdColour)) {
                MERROR("Не удалось применить цветовую униформу идентификатора.");
                return false;
            }

            bool NeedsUpdate = !InstanceUpdate[CurrentInstanceID];
            ShaderSystem::ApplyInstance(NeedsUpdate);
            InstanceUpdate[CurrentInstanceID] = true;

            // Применить локальные переменные
            if (!ShaderSystem::UniformSet(TerrainShaderInfo.ModelLocation, &geo.model)) {
                MERROR("Не удалось применить матрицу модели для геометрии ландшафта.");
            }

            // Нарисовать ее.
            RenderingSystem::DrawGeometry(packet.TerrainGeometries[i]);
        }

        if (!RenderingSystem::RenderpassEnd(pass)) {
            MERROR("RenderViewPick::Render Индекс прохода рендеринга %u не удалось завершить.", p);
            return false;
        }

        p++;
        pass = &passes[p];  // Второй проход

        if (!RenderingSystem::RenderpassBegin(pass, pass->targets[RenderTargetIndex])) {
            MERROR("RenderViewPick::Render индекс прохода %u не удалось запустить.", p);
            return false;
        }

        // пользовательский интерфейс
        if (!ShaderSystem::Use(UiShaderInfo.s->id)) {
            MERROR("Не удалось использовать шейдер материала. Не удалось выполнить рендеринг кадра.");
            return false;
        }

        // Применить глобальные переменные
        if (!ShaderSystem::UniformSet(UiShaderInfo.ProjectionLocation, &UiShaderInfo.projection)) {
            MERROR("Не удалось применить проекционную матрицу");
        }
        if (!ShaderSystem::UniformSet(UiShaderInfo.ViewLocation, &UiShaderInfo.view)) {
            MERROR("Не удалось применить матрицу вида");
        }
        ShaderSystem::ApplyGlobal();

        // Нарисовать геометрию. Начните с того места, где остановились мировые геометрии.
        for (u32 i = WorldGeometryCount; i < packet.geometries.Length(); ++i) {
            auto& geo = packet.geometries[i];
            CurrentInstanceID = geo.UniqueID;

            ShaderSystem::BindInstance(CurrentInstanceID);

            // Получить цвет на основе идентификатора
            u32 r, g, b;
            Math::u32ToRGB(geo.UniqueID, r, g, b);
            FVec3 IdColour(r, g, b);
            IdColour /= 255.F;
            if (!ShaderSystem::UniformSet(UiShaderInfo.IdColourLocation, &IdColour)) {
                MERROR("Не удалось применить цветовую униформу идентификатора.");
                return false;
            }

            bool NeedsUpdate = !InstanceUpdate[CurrentInstanceID];
            ShaderSystem::ApplyInstance(NeedsUpdate);
            InstanceUpdate[CurrentInstanceID] = true;

            // Применить локальные переменные
            if (!ShaderSystem::UniformSet(UiShaderInfo.ModelLocation, &geo.model)) {
                MERROR("Не удалось применить матрицу модели для текста");
            }

            // Нарисовать ее.
            RenderingSystem::DrawGeometry(packet.geometries[i]);
        }

        // Нарисовать растровый текст
        for (u32 i = 0; i < PacketData->TextCount; ++i) {
            auto text = PacketData->texts[i];
            CurrentInstanceID = text->UniqueID;
            ShaderSystem::BindInstance(CurrentInstanceID);

            // Получить цвет на основе идентификатора
            
            u32 r, g, b;
            Math::u32ToRGB(text->UniqueID, r, g, b);
            FVec3 IdColour(r, g, b);
            IdColour /= 255.F;
            if (!ShaderSystem::UniformSet(UiShaderInfo.IdColourLocation, &IdColour)) {
                MERROR("Не удалось применить униформу цвета идентификатора.");
                return false;
            }

            ShaderSystem::ApplyInstance(true);

            // Применить локальные переменные
            auto model = text->transform.GetWorld();
            if (!ShaderSystem::UniformSet(UiShaderInfo.ModelLocation, &model)) {
                MERROR("Не удалось применить матрицу модели для текста");
            }

            text->Draw();
        }

        if (!RenderingSystem::RenderpassEnd(pass)) {
            MERROR("RenderViewPick::Render индекс прохода %u не завершился.", p);
            return false;
        }
    }

    // Прочитать пиксель в координатах мыши.
    u8 PixelRGBA[4]{};
    u8* pixel = &PixelRGBA[0];

    // Прижать к размеру изображения
    // u16 xCoord = MCLAMP(MouseX, 0, width - 1);
    // u16 yCoord = MCLAMP(MouseY, 0, height - 1);
    RenderingSystem::TextureReadPixel(&ColoureTargetAttachmentTexture, MouseX, MouseY, &pixel);

    // Извлечь идентификатор из выбранного цвета.
    u32 id = INVALID::ID;
    Math::RgbuToU32(pixel[0], pixel[1], pixel[2], id);
    if (id == 0x00FFFFFF) {
        // Это чистый белый.
        id = INVALID::ID;
    }

    EventContext context;
    context.data.u32[0] = id;
    EventSystem::Fire(EventSystem::OojectHoverIdChanged, nullptr, context);

    return true;
}

bool RenderViewPick::RegenerateAttachmentTarget(u32 PassIndex, RenderTargetAttachment *attachment)
{
    if (!PassIndex && !attachment) {
        return true;
    }

    if (attachment->type == RenderTargetAttachmentType::Colour) {
        attachment->texture = &ColoureTargetAttachmentTexture;
    } else if (attachment->type == RenderTargetAttachmentType::Depth) {
        attachment->texture = &DepthTargetAttachmentTexture;
    } else {
        MERROR("Неподдерживаемый тип прикрепления 0x%x.", attachment->type);
        return false;
    }

    if (PassIndex == 1) {
        // Не нужно повторно генерировать для обоих проходов, так как они оба используют одно и то же прикрепленное приложение.
        // Просто прикрепите его и продолжайте.
        return true;
    }

    // Уничтожьте текущее прикрепленное приложение, если оно существует.
    if (attachment->texture->data) {
        RenderingSystem::Unload(attachment->texture);
        MemorySystem::ZeroMem(attachment->texture, sizeof(Texture));
    }

    // Настройте новую текстуру.
    // Сгенерируйте UUID, который будет использоваться в качестве имени текстуры.
    uuid TextureNameUuid = uuid::Generate();

    u32 width = passes[PassIndex].RenderArea.z;
    u32 height = passes[PassIndex].RenderArea.w;
    bool HasTransparency = false;  // ЗАДАЧА: настраиваемый

    attachment->texture->id = INVALID::ID;
    attachment->texture->type = TextureType::_2D;
    MString::Copy(attachment->texture->name, TextureNameUuid.value, TEXTURE_NAME_MAX_LENGTH);
    attachment->texture->width = width;
    attachment->texture->height = height;
    attachment->texture->ChannelCount = 4;  // ЗАДАЧА: настраиваемый
    attachment->texture->generation = INVALID::ID;
    attachment->texture->flags |= HasTransparency ? Texture::Flag::HasTransparency : 0;
    attachment->texture->flags |= Texture::Flag::IsWriteable;
    if (attachment->type == RenderTargetAttachmentType::Depth) {
        attachment->texture->flags |= Texture::Flag::Depth;
    }
    attachment->texture->data = nullptr;

    RenderingSystem::LoadTextureWriteable(attachment->texture);

    return true;
}

void RenderViewPick::GetMatrices(Matrix4D &OutView, Matrix4D &OutProjection)
{

}

void *RenderViewPick::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Renderer);
}

void RenderViewPick::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Renderer);
}

bool RenderViewPick::OnMouseMoved(u16 code, void *sender, void *ListenerInst, EventContext EventData)
{
    if (code == EventSystem::MouseMoved) {
        auto self = reinterpret_cast<RenderViewPick*>(ListenerInst);

        // Обновите положение и регенерируйте матрицу проекции.
        const u16& y = EventData.data.u16[1];
        const u16& x = EventData.data.u16[0];

        self->MouseX = x;
        self->MouseY = y;

        return true;
    }
    return false;
}

void RenderViewPick::AcquireShaderInstances()
{
    // Не сохраняем идентификатор экземпляра, так как это не имеет значения.
    u32 instance;
    // Шейдер пользовательского интерфейса
    if (!RenderingSystem::ShaderAcquireInstanceResources(UiShaderInfo.s, 0, nullptr, instance)) {
        MFATAL("RenderViewPick не удалось получить ресурсы шейдера пользовательского интерфейса.");
        return;
    }
    // Шейдер мира
    if (!RenderingSystem::ShaderAcquireInstanceResources(WorldShaderInfo.s, 0, nullptr, instance)) {
        MFATAL("RenderViewPick не удалось получить ресурсы шейдера мира.");
        return;
    }
    // Шейдер ландшафта
    if (!RenderingSystem::ShaderAcquireInstanceResources(TerrainShaderInfo.s, 0, nullptr, instance)) {
        MFATAL("RenderViewPick не удалось получить ресурсы шейдера ландшафта.");
        return;
    }
    InstanceCount++;
    InstanceUpdate.PushBack(false);
}

void RenderViewPick::ReleaseShaderInstances()
{
    for (u32 i = 0; i < InstanceCount; ++i) {
        // Шейдер пользовательского интерфейса
        if (!RenderingSystem::ShaderReleaseInstanceResources(UiShaderInfo.s, i)) {
            MWARN("Не удалось освободить ресурсы шейдера пользовательского интерфейса.");
        }

        // Шейдер мира
        if (!RenderingSystem::ShaderReleaseInstanceResources(WorldShaderInfo.s, i)) {
            MWARN("Не удалось освободить ресурсы шейдера мира.");
        }

        // Terrain shader
        if (!RenderingSystem::ShaderReleaseInstanceResources(TerrainShaderInfo.s, i)) {
            MWARN("Не удалось освободить ресурсы шейдера ландшафта.");
        }
    }
    InstanceUpdate.Clear();
}
