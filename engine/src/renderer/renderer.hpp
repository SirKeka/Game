/// @file renderer.hpp
/// @author 
/// @brief Интерфейс рендеринга — единственное, что видит остальная часть движка. 
/// Он отвечает за передачу любых данных в/из бэкэнда рендеринга независимым способом.
/// @version 1.0
/// @date 
///
/// @copyright 
#pragma once

#include "defines.hpp"

#include "renderer_types.hpp"
#include "math/vertex.hpp"

struct StaticMeshData;
struct PlatformState;
class VulkanAPI;
class Shader;

class Renderer
{
private:
    static RendererType* ptrRenderer;
    Matrix4D projection;
    Matrix4D view;
    Matrix4D UI_Projection;
    Matrix4D UI_View;
    f32 NearClip;
    f32 FarClip;

public:
    Renderer() : projection(), view(), UI_Projection(), UI_View(), NearClip(0.f), FarClip(0.f) {}
    ~Renderer();

    /// @brief Инициализирует интерфейс/систему рендеринга.
    /// @param window указатель на класс окна/поверхности на которой орисовщик будет рисовать
    /// @param ApplicationName Имя приложения.
    /// @param type тип отрисовщика с которым первоначально будет инициализироваться интерфейс/система
    /// @return true в случае успеха; в противном случае false.
    bool Initialize(MWindow* window, const char *ApplicationName, ERendererType type);
    /// @brief Выключает систему рендеринга/интерфейс.
    void Shutdown();
    /// @brief Обрабатывает события изменения размера.
    /// @param width Новая ширина окна.
    /// @param height Новая высота окна.
    void OnResized(u16 width, u16 height);
    /// @brief Рисует следующий кадр, используя данные, предоставленные в пакете рендеринга.
    /// @param packet Указатель на пакет рендеринга, который содержит данные о том, что должно быть визуализировано.
    /// @return true в случае успеха; в противном случае false.
    bool DrawFrame(RenderPacket* packet);

    /// @brief Функция/метод предоставляющая доступ к самому отрисовщику.
    /// @return указатель на отрисовщик вулкан.
    static VulkanAPI* GetRenderer();
 
    //static bool CreateMaterial(class Material* material);
    //static void DestroyMaterial(class Material* material);

    /// @brief Загружает данные геометрии в графический процессор.------------------------------------------------------------------------------------
    /// @param gid указатель на геометрию, которую требуется загрузить.
    /// @param VertexSize размер каждой вершины.
    /// @param VertexCount количество вершин.
    /// @param vertices массив вершин.
    /// @param IndexSize размер каждого индекса.
    /// @param IndexCount количество индексов.
    /// @param indices индексный массив.
    /// @return true в случае успеха; в противном случае false.
    static bool Load(GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices);
    /// @brief Уничтожает заданную геометрию, освобождая ресурсы графического процессора.-------------------------------------------------------------
    /// @param gid указатель на геометрию, которую нужно уничтожить.
    static void Unload(GeometryID* gid);
    /// @brief Получает идентификатор средства рендеринга с заданным именем.---------------------------------------------------------------------------
    /// @param name имя средства рендеринга, идентификатор которого требуется получить.
    /// @param OutRenderpassID указатель для хранения идентификатора renderpass.
    /// @return true если найден, иначе false.
    static bool RenderpassID(const char* name, u8& OutRenderpassID);
    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.---------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param RenderpassID идентификатор прохода рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    static bool Load(Shader* shader, u8 RenderpassID, u8 StageCount, DArray<char*> StageFilenames, const ShaderStage* stages);
    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    static void Unload(Shader* shader);
    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    static bool ShaderInitialize(Shader* shader);
    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    static bool ShaderUse(Shader* shader);
    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    static bool ShaderApplyGlobals(Shader* shader);
    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @return true в случае успеха, иначе false.
    static bool ShaderApplyInstance(Shader* shader);
    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    static bool ShaderAcquireInstanceResources(Shader* shader, u32& OutInstanceID);
    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    static bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID);
    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    static bool SetUniform(Shader* shader, struct ShaderUniform* uniform, const void* value);

    /// @brief Устанавливает матрицу представления в средстве визуализации. ПРИМЕЧАНИЕ: Доступен общедоступному API.
    /// @deprecated ВЗЛОМ: это не должно быть выставлено за пределы движка.
    /// @param view Матрица представления, которую необходимо установить.
    MAPI void SetView(Matrix4D view);


    void* operator new(u64 size);
    // void operator delete(void* ptr);
private:
    //static void CreateTexture(class Texture* t);
};

