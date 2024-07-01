#pragma once

#include "defines.hpp"
#include "math/matrix4d.hpp"
#include "math/vector4d.hpp"
#include "math/vertex.hpp"
#include "resources/shader.hpp"

struct StaticMeshData;
// class Shader;

namespace RendererDebugViewMode {
    enum RendererDebugViewMode : u32 {
        Default = 0,
        Lighting = 1,
        Normals = 2
};
}

enum class ERendererType 
{
    VULKAN,
    OPENGL,
    DIRECTX
};

/*размер данной струтуры для карт Nvidia должен быть равен 256 байт*/
struct VulkanMaterialShaderGlobalUniformObject
{
    Matrix4D projection;        // 64 байта
    Matrix4D view;              // 64 байта
    Matrix4D mReserved0;        // 64 байта, зарезервированные для будущего использования
    Matrix4D mReserved1;        // 64 байта, зарезервированные для будущего использования
};

struct VulkanMaterialShaderInstanceUniformObject 
{
    Vector4D<f32> DiffuseColor; // 16 байт
    Vector4D<f32> vReserved0;   // 16 байт, зарезервировано для будущего использования.
    Vector4D<f32> vReserved1;   // 16 байт, зарезервировано для будущего использования.
    Vector4D<f32> vReserved2;   // 16 байт, зарезервировано для будущего использования.
};

/// @brief 
struct VulkanUI_ShaderGlobalUniformObject {
    Matrix4D projection;        // 64 bytes
    Matrix4D view;              // 64 bytes
    Matrix4D mReserved0;        // 64 bytes, зарезервировано для будущего использования.
    Matrix4D mReserved1;        // 64 bytes, зарезервировано для будущего использования.
};

/// @brief Объект универсального буфера экземпляра материала пользовательского интерфейса, специфичный для Vulkan, для шейдера пользовательского интерфейса.
struct VulkanUI_ShaderInstanceUniformObject {
    Vector4D<f32> DiffuseColor; // 16 bytes
    Vector4D<f32> vReserved0;   // 16 bytes, зарезервировано для будущего использования.
    Vector4D<f32> vReserved1;   // 16 bytes, зарезервировано для будущего использования.
    Vector4D<f32> vReserved2;   // 16 bytes, зарезервировано для будущего использования.
};

struct GeometryRenderData 
{
    Matrix4D model;
    struct GeometryID* gid;

    constexpr GeometryRenderData() : model(), gid(nullptr) {}
    constexpr GeometryRenderData(const Matrix4D& model, struct GeometryID* gid) : model(model), gid(gid) {}
    constexpr GeometryRenderData(GeometryRenderData&& grd) : model(grd.model), gid() {}
};

/* f32 DeltaTime
 * u32 GeometryCount
 * struct GeometryRenderData* geometries
 * u32 UI_GeometryCount
 * struct GeometryRenderData* UI_Geometries*/
struct RenderPacket
{
    f64 DeltaTime;
    u32 GeometryCount;
    DArray<GeometryRenderData> geometries;
    u32 UI_GeometryCount;
    DArray<GeometryRenderData> UI_Geometries;
    constexpr RenderPacket() : DeltaTime(), GeometryCount(), geometries(), UI_GeometryCount(), UI_Geometries() {}
    constexpr RenderPacket(f64 DeltaTime, u32 GeometryCount, GeometryRenderData geometries, u32 UI_GeometryCount, GeometryRenderData UI_Geometries)
    : DeltaTime(DeltaTime), 
    GeometryCount(GeometryCount), 
    geometries(), 
    UI_GeometryCount(UI_GeometryCount), 
    UI_Geometries() {
        this->geometries.PushBack(std::move(geometries));
        this->UI_Geometries.PushBack(std::move(UI_Geometries));
    }
};

class RendererType
{
public:
    u64 FrameNumber;

    // Указатель на текстуру по умолчанию.
    //class Texture* DefaultDiffuse;
public:
    virtual ~RendererType() = default;

    // virtual bool Initialize(class MWindow* window, const char* ApplicationName) = 0;
    virtual void ShutDown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;
    virtual bool BeginFrame(f32 Deltatime) = 0;
    virtual bool EndFrame(f32 DeltaTime) = 0;
    virtual bool BeginRenderpass(u8 RenderpassID) = 0;
    virtual bool EndRenderpass(u8 RenderpassID) = 0;
    virtual void DrawGeometry(const GeometryRenderData& data) = 0;

    virtual bool Load(struct GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices) = 0;
    virtual void Unload(struct GeometryID* gid) = 0;
    // Методы относящиеся к шейдерам---------------------------------------------------------------------------------------------------------------------------------------------

    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    /// @param shader указатель на шейдер.
    /// @param RenderpassID идентификатор прохода рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    virtual bool Load(Shader* shader, u8 RenderpassID, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage* stages) = 0;
    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    virtual void Unload(Shader* shader) = 0;
    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderInitialize(Shader* shader) = 0;
    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderUse(Shader* shader) = 0;
    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyGlobals(Shader* shader) = 0;
    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyInstance(Shader* shader) = 0;
    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderAcquireInstanceResources(Shader* shader, u32& OutInstanceID) = 0;
    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID) = 0;
    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    virtual bool SetUniform(Shader* shader, struct ShaderUniform* uniform, const void* value) = 0;
};
