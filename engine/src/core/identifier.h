/// @file identifier.h
/// @author 
/// @brief Содержит систему создания числовых идентификаторов.
/// @version 1.0
/// @date 2024-09-20
/// @copyright 
#pragma once

#include "defines.hpp"

namespace Identifier
{
    ///@brief Получает новый идентификатор для данного владельца.
    ///@param owner Владелец идентификатора.
    ///@return Новый идентификатор.
    MAPI u32 AquireNewID(void* owner);

    /// @brief Освобождает данный идентификатор, который затем может быть использован снова.
    /// @param id Идентификатор, который будет освобожден.
    MAPI void ReleaseID(u32& id);
} // namespace Identifier
