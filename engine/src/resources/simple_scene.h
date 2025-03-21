#pragma once

#include "defines.hpp"
#include "simple_scene_config.h"
#include "mesh.hpp"
#include "lighting_structures.h"
#include "terrain.h"

#include "math/transform.hpp"
#include "renderer/views/render_view_world.h"

struct FrameData;
class Camera;
struct RenderPacket;

struct PendingMesh {
    Mesh* m;

    const char* MeshResourceName;

    u32 GeometryConfigCount;
    struct GeometryConfig** GeometryConfigs;
};

struct MAPI SimpleScene {
    enum class State {
        /// @brief создано, но ничего больше.
        Uninitialized,
        /// @brief Конфигурация проанализирована, настройка иерархии еще не загружена.
        Initialized,
        /// @brief В процессе загрузки иерархии.
        Loading,
        /// @brief Все загружено, готово к воспроизведению.
        Loaded,
        /// @brief В процессе выгрузки, не готово к воспроизведению.
        Unloading,
        /// @brief Выгружено и готово к уничтожению.
        Unloaded
    };

    u32 id;
    State state;
    bool enabled;

    MString name;
    MString description;

    Transform SceneTransform;

    // Единственный указатель на направленный свет.
    struct DirectionalLight* DirLight;

    DArray<PointLight> PointLights;
    DArray<Mesh> meshes;
    DArray<Terrain> terrains;
    DArray<PendingMesh> PendingMeshes;

    struct Skybox* sb;

    // Указатель на конфигурацию сцены, если она указана.
    SimpleSceneConfig* config;
    RenderViewWorldData WorldData;

    SimpleScene() : id(GlobalSceneID++), state(State::Uninitialized), enabled(false), name(), description(), SceneTransform(), DirLight(nullptr), PointLights(), meshes(), terrains(), PendingMeshes(), sb(nullptr), config(nullptr), WorldData() {}

    /// @brief Создает новую сцену с заданной конфигурацией со значениями по умолчанию. Ресурсы не выделены. Конфигурация еще не обработана.
    /// @param config Указатель на конфигурацию. Необязательно.
    /// @return True в случае успеха; в противном случае false.
    bool Create(SimpleSceneConfig* config = nullptr);

    /// @brief Уничтожает заданную сцену. Сначала выполняет выгрузку, если сцена загружена.
    /// @return True в случае успеха; в противном случае false.
    bool Destroy();

    /// @brief Выполняет процедуры инициализации на сцене, включая конфигурацию обработки (если она указана) и иерархию каркаса.
    /// @return True в случае успеха; в противном случае false.
    bool Initialize();

    /// @brief Выполняет процедуры загрузки и выделения ресурсов на заданной сцене.
    /// @return True в случае успеха; в противном случае false.
    bool Load();

    /// @brief Выполняет процедуры выгрузки и освобождения ресурсов на заданной сцене.
    /// @return True в случае успеха; в противном случае false.
    /// @param immediate Выгрузить немедленно, а не в следующем кадре. ПРИМЕЧАНИЕ: может иметь непреднамеренные побочные эффекты при неправильном использовании.
    bool Unload(bool immediate);

    /// @brief Выполняет все необходимые обновления сцены для указанного кадра.
    /// @param scene ссылка на сцену для обновления.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return True в случае успеха; в противном случае false.
    bool Update(const FrameData& rFrameData);

    /// @brief Заполняет указанный пакет рендеринга данными из предоставленной сцены.
    /// @param scene ссылка на сцену для обновления.
    /// @param CurrentCamera текущая камера для использования при рендеринге сцены.
    /// @param aspect Соотношение сторон.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @param packet ссылка на пакет для заполнения.
    /// @return True в случае успеха; в противном случае false.
    bool PopulateRenderPacket(Camera* CurrentCamera, f32 aspect, FrameData& rFrameData, RenderPacket& packet);

    /// @brief Добавляет направленый источник освещения на сцену
    /// @param name название направленного источника света
    /// @param light направленный источник освещения
    /// @return True в случае успеха; в противном случае false.
    bool AddDirectionalLight(const char* name, DirectionalLight& light);

    /// @brief Добавляет точечный источник освещения на сцену
    /// @param name название точечного источника света
    /// @param light точечный источник освещения
    /// @return True в случае успеха; в противном случае false.
    bool AddPointLight(const char* name, PointLight& light);

    /// @brief Добавляет сетку на сцену
    /// @param name название сетки
    /// @param m сетка
    /// @return True в случае успеха; в противном случае false.
    bool AddMesh(const char* name, Mesh& m);

    /// @brief Добавляет скайбокс на сцену
    /// @param name название скайбокса
    /// @param sb скайбокс
    /// @return True в случае успеха; в противном случае false.
    bool AddSkybox(const char* name, Skybox& sb);

    /// @brief Добавляет ландшафт на сцену
    /// @param name назване ландшафта
    /// @param terrain ландшафт, который нужно добавить на сцену.
    /// @return True в случае успеха; в противном случае false.
    bool AddTerrain(const char* name, Terrain& terrain);

    /// @brief 
    /// @return Структуру направленного света
    DirectionalLight* GetDirectionalLight(const char* name);

    /// @brief 
    /// @return Структуру точечного освещения
    PointLight* GetPointLight(const char* name);

    /// @brief 
    /// @return Структуру сетки
    Mesh* GetMesh(const MString& name);

    /// @brief 
    /// @return Скайбокс
    Skybox* GetSkybox(const char* name);

    /// @brief 
    /// @param name название ландшафта, который нужно получить из сцены
    /// @return Ландшафт
    Terrain* GetTerrain(const char* name);

    /// @brief Удаляет направленный источник освещения из сцены
    /// @param light направленный источник освещения
    /// @return true если удаление прошло успешно, иначе false
    bool RemoveDirectionalLight();

    /// @brief Удаляет точечный источник освещения.
    /// @param light точечный источник освещения
    /// @return true если удаление прошло успешно, иначе false
    bool RemovePointLight(const MString& name, PointLight& light);

    /// @brief Удаляет сетку со сцены.
    /// @param m сетка
    /// @return true если удаление прошло успешно, иначе false
    bool RemoveMesh(const char* name);

    /// @brief Удвляет скайбокс со сцены.
    /// @param sb скайбокс
    /// @return true если удаление прошло успешно, иначе false
    bool RemoveSkybox(const char* name);

    /// @brief Удаляет ландшафт из сцены
    /// @param name название ландшафта, который нужно удалить
    /// @return true если удаление прошло успешно, иначе false
    bool RemoveTerrain(const char* name);

private:
    static inline u32 GlobalSceneID;

    void ActualUnload();
};
