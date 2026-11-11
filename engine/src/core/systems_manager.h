#pragma once

#include "memory/linear_allocator.h"

struct ApplicationConfig;
struct FrameData;
class RenderingSystem;
class UiSystem;

/// @brief Определение типа для указателя на функцию инициализации системы.
using PFN_SystemInitialize = bool(*)(u64&, void*, void*);
/// @brief Определение типа для указателя на функцию завершения работы системы.
using PFN_SystemShutdown   = void(*)(void*); 
/// @brief Определение типа для указателя на функцию обновления.
using PFN_SystemUpdate     = bool(*)(void*, FrameData&);
/// @brief Определение типа для указателя на функцию подготовки кадра к отрисовке.
using PFN_SystemRenderPrepareFrame = void (*)(void*, const struct FrameData&);

constexpr u32 M_SYSTEM_TYPE_MAX_COUNT = 512;

/// @brief Представляет собой зарегистрированную систему. 
/// Здесь хранятся указатели на функции инициализации, 
/// завершения работы и (опционально) обновления, а также состояние системы.
struct MSystem {
    // u64 StateSize;                   // Размер состояния для системы.
    void* state{};                      // Состояние системы.
    // PFN_SystemInitialize Initialize{}; // Указатель на функцию для процедуры инициализации. Обязательный элемент.
    PFN_SystemShutdown Shutdown{};      // Указатель на функцию для процедуры завершения работы. Обязательный параметр.
    /// @brief Указатель функции для процедуры обновления системы, вызываемой каждый кадр. Необязательно.
    PFN_SystemUpdate Update{};
    /// @brief Указатель на функцию для системной процедуры «подготовки кадра» (prepare frame), которая вызывается в каждом кадре. Необязательный элемент.
    PFN_SystemRenderPrepareFrame RenderPrepareFrame{};
};

class SystemsManager
{
public:
    /// @brief Определяет известные типы систем в ядре движка — вплоть до значения Max. 
    /// Пользовательские перечисления могут начинаться со значения Max + 1 для регистрации собственных систем.
    enum Type : u16 {
        Memory = 0,
        Console,
        MVar,
        Event,
        Logging,
        Input,
        Platform,
        Resource,
        Shader,
        Job,
        Texture,
        Font,
        Camera,
        Renderer,
        Material,
        Geometry,
        Light,

        // ПРИМЕЧАНИЕ: Все значения от 127 до 254 являются пространством расширения.
        KnownMax = 127,
        UI,
    
        // ПРИМЕЧАНИЕ: Все, что находится за пределами этого, находится в пользовательском пространстве.
        ExtensionMax = 255,
    
        // Максимум пользовательского пространства
        UserMax = M_SYSTEM_TYPE_MAX_COUNT,
        // Максимум, включая все типы пользовательского пространства.
        Max = UserMax
    };

    NON_COPYABLE(SystemsManager);

    /// @brief Инициализирует системный менеджер для всех систем, которые должны быть настроены до начала процесса загрузки приложения (например, событий, системы рендеринга и т. д.).
    /// @param manager указатель на инициализируемое состояние системного менеджера.
    /// @param AppConfig указатель на конфигурацию приложения.
    /// @return true в случае успеха; в противном случае — false.
    static bool Initialize(SystemsManager* manager, ApplicationConfig& AppConfig);

    /// @brief Инициализирует системный менеджер для всех систем, которые должны быть настроены после завершения процесса загрузки приложения 
    ///        (то есть тех, для настройки которых требуется само приложение — например, шрифты и т. д.).
    /// @param AppConfig ссылка на конфигурацию приложения.
    /// @return true в случае успеха; в противном случае — false.
    bool PostBootInitialize(ApplicationConfig& AppConfig);

    /// @brief Вызывает процедуры обновления во всех системах, которые участвуют в процессе обновления. Выполняется в ходе основного цикла движка.
    /// @param rFrameData Указатель на данные для этого кадра.
    /// @return true в случае успеха; в противном случае — false.
    bool Update(FrameData& rFrameData);

    /// @brief Вызывает процедуры «подготовки кадра» (frame prepare) во всех системах, которые подключились к этому механизму. 
    /// Обычно это требуется системам, которым необходимо внедрить в процесс формирования кадра логику, выполняемую перед этапом отрисовки 
    /// (например, обновление вершинных данных). Выполняется в ходе основного цикла работы движка.
    /// @param rFrameData Константный указатель на данные для этого кадра.
    void RendererFramePrepare(const FrameData& rFrameData);

    MAPI bool Register(u16 type, void* system, PFN_SystemUpdate update = nullptr, PFN_SystemShutdown shutdown = nullptr, PFN_SystemRenderPrepareFrame RenderPrepareFrame = nullptr);

    /// @brief Выделяет память под систему
    /// @param size количество памяти, которое нужно выделить
    /// @return Указатель на память куда нужно записать систему
    MAPI void* GetMem(u64 size) { return SystemsAllocator.Allocate(size); }

    MAPI static void* GetState(u16 type);

    MAPI static auto GetRenderingSystem() { return (RenderingSystem*)GetState(Renderer); }
    MAPI static auto GetUiSystem() { return (class UiSystem*)GetState(UI); }

private:
    static bool RegisterKnownSystemsPreBoot(ApplicationConfig& AppConfig);
    bool RegisterKnownSystemsPostBoot(ApplicationConfig& AppConfig);
    void ShutdownKnownSystems();

    friend class Engine;
    LinearAllocator SystemsAllocator;
    MSystem systems[M_SYSTEM_TYPE_MAX_COUNT];

    static inline SystemsManager* state;

    /// @brief Создать линейный распределитель для всех систем (кроме памяти) для использования.
    constexpr SystemsManager() : SystemsAllocator(MEBIBYTES(64)), systems() {}
    ~SystemsManager();
};

