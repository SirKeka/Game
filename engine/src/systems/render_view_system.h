/// @file render_view_system.hpp
/// @author 
/// @brief Система управления представлениями.
/// @version 1.0
/// @date 2024-08-07
/// 
/// @copyright
#pragma once
#include "defines.hpp"
#include "renderer/render_view.h"

struct FrameData;
struct RenderView;

/// @brief Конфигурация для системы рендеринга представлений.
struct RenderViewSystemConfig
{
    /// @brief Максимальное количество представлений, которые могут быть зарегистрированы в системе.
    u16 MaxViewCount;
};

namespace RenderViewSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    /// @brief Создает новый вид с использованием предоставленной конфигурации. Затем новый вид может быть получен с помощью вызова RenderViewSystem::Get.
    /// @param config постоянный указатель на конфигурацию вида.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Register(RenderView* view);

    /// @brief Вызывается при изменении размера владельца этого представления (т. е. окна).
    /// @param width Новая ширина в пикселях.
    /// @param height Новая высота в пикселях.
    MAPI void OnWindowResize(u32 width, u32 height);

    /// @brief Получает указатель на представление с заданным именем.
    /// @param name имя представления.
    /// @return Указатель на представление, если оно найдено; в противном случае — nulptr.
    MAPI RenderView *Get(const char *name);

    /// @brief Создает пакет представления рендеринга, используя предоставленное представление и сетки.
    /// @param view указатель на представление для использования.
    /// @param  распределитель использовал этот кадр для создания пакета.
    /// @param data данные свободной формы, используемые для создания пакета.
    /// @param OutPacket указатель для хранения сгенерированного пакета.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool BuildPacket(RenderView* view, LinearAllocator& FrameAllocator, void* data, RenderView::Packet& OutPacket);

    /// @brief Использует заданное представление и пакет для визуализации содержимого.
    /// @param view указатель на представление для использования.
    /// @param packet указатель на пакет, данные которого должны быть визуализированы.
    /// @param FrameNumber текущий номер кадра визуализатора, обычно используемый для синхронизации данных.
    /// @param RenderTargetIndex текущий индекс цели визуализации для визуализаторов, которые используют несколько целей визуализации одновременно (например, Vulkan).
    /// @return true в случае успеха; в противном случае false.
    bool OnRender(RenderView* view, const RenderView::Packet& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFramedata);
    
    MAPI void RegenerateRenderTargets(RenderView* view);
}
