/**
 * @file handle.h
 * @author 
 * @brief Глобальная система дескрипторов для движка. Дескрипторы используются для получения доступа к различным ресурсам посредством уникального идентификатора.
 * @version 1.0
 * @date 18.11.2026
 *
 * @copyright
 *
 */

#pragma once

#include "identifier.h"

// #define INVALID_K_HANDLE INVALID_ID_U64

/// @brief Дескриптор (handle) — это уникальный идентификатор, используемый в системе движка, чтобы по возможности избежать применения «сырых» указателей.
struct MAPI Handle 
{
    /// @brief Обращение к таблице ресурсов по индексу. Считается нулевым, если равно INVALID::ID.
    u32 HandleIndex;
    /// @brief Глобально уникальный идентификатор
    Identifier UniqueID;

    /// @brief Создает и возвращает недействительный дескриптор.
    constexpr Handle() : HandleIndex(INVALID::ID), UniqueID(INVALID::U64ID) {}
    /// @brief Создает и возвращает дескриптор с заданным индексом. Также создает новый уникальный идентификатор.
    constexpr Handle(u32 HandleIndex) : HandleIndex(HandleIndex), UniqueID(UniqueID.Generate()) {}
    /// @brief Создает и возвращает дескриптор на основе предоставленных индекса и идентификатора дескриптора.
    constexpr Handle(u32 HandleIndex, Identifier id) : HandleIndex(HandleIndex), UniqueID(id) {}

    /// @brief Указывает, является ли предоставленный дескриптор недействительным.
    bool IsInvalid() { return HandleIndex == INVALID::ID || UniqueID == INVALID::U64ID; }
    /// @brief Делает предоставленный дескриптор недействительным.
    bool Invalidate() { HandleIndex = INVALID::ID; UniqueID = INVALID::U64ID; }
};
