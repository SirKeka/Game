/// @file identifier.h
/// @author 
/// @brief Содержит систему создания числовых идентификаторов.
/// @version 1.0
/// @date 15.10.2025
/// @copyright 
#pragma once

#include "defines.h"

/// @brief Глобально/универсально уникальный идентификатор в формате 64-битного беззнакового целого числа. Используется в первую очередь как идентификатор ресурсов. Удобен для (де)сериализации.
struct MAPI Identifier
{
    u64 UniqueID;    // Фактический внутренний идентификатор.

    constexpr Identifier() = default;
    ///@brief Генерирует новый уникальный идентификатор.
    Identifier& Generate();

    /// @brief Создаёт идентификатор на основе известного значения. Полезно для десериализации.
    constexpr Identifier(u64 UniqueID) : UniqueID(UniqueID) {}

    MINLINE bool operator==(Identifier id) { return UniqueID == id.UniqueID; }
};

MINLINE bool operator==(Identifier lid, u64 rid) { return lid.UniqueID == rid; }
MINLINE bool operator>(Identifier lid, u64 rid) { return lid.UniqueID > rid; }