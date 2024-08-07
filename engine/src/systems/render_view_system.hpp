/// @file render_view_system.hpp
/// @author 
/// @brief Система управления представлениями.
/// @version 1.0
/// @date 2024-08-07
/// 
/// @copyright
#pragma once
#include "containers/hashtable.hpp"

class RenderView;

class RenderViewSystem
{
    friend class RenderView;
private:
    HashTable<u16> lookup;
    void* TableBlock;
    u32 MaxViewCount;
    RenderView* RegisteredViews;

    static RenderViewSystem* state;
    constexpr RenderViewSystem(u16 MaxViewCount, void* TableBlock, RenderView* RegisteredViews);
    ~RenderViewSystem();
public:
    RenderViewSystem(const RenderViewSystem&) = delete;
    RenderViewSystem& operator= (const RenderViewSystem&) = delete;

    static bool Initialize(u16 MaxViewCount);
    static void Shutdown();

    /// @brief Создает новый вид с использованием предоставленной конфигурации. Затем новый вид может быть получен с помощью вызова RenderViewSystem::Get.
    /// @param config постоянный указатель на конфигурацию вида.
    /// @return true в случае успеха; в противном случае false.
    bool Create(const RenderView::Config* config);
    /// @brief Вызывается при изменении размера владельца этого представления (т. е. окна).
    /// @param width Новая ширина в пикселях.
    /// @param height Новая высота в пикселях.
    void OnWindowResize(u32 width, u32 height);
    /// @brief Получает указатель на представление с указанным именем.
    /// @param name имя представления.
    /// @return Указатель на представление, если оно найдено; в противном случае 0.
    RenderView* Get(const char* name);
    /// @brief Создает пакет представления рендеринга, используя предоставленное представление и сетки.
    /// @param view указатель на представление для использования.
    /// @param data данные свободной формы, используемые для создания пакета.
    /// @param OutPacket указатель для хранения сгенерированного пакета.
    /// @return true в случае успеха; в противном случае false.
    bool BuildPacket(const RenderView* view, void* data, struct RenderViewPacket* OutPacket);
    /// @brief Использует заданное представление и пакет для визуализации содержимого.
    /// @param view указатель на представление для использования.
    /// @param packet указатель на пакет, данные которого должны быть визуализированы.
    /// @param FrameNumber текущий номер кадра визуализатора, обычно используемый для синхронизации данных.
    /// @param RenderTargetIndex текущий индекс цели визуализации для визуализаторов, которые используют несколько целей визуализации одновременно (например, Vulkan).
    /// @return true в случае успеха; в противном случае false.
    bool OnRender(const RenderView* view, const RenderViewPacket* packet, u64 FrameNumber, u64 RenderTargetIndex);
};
