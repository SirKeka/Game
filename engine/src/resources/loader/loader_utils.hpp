#pragma once
#include "defines.hpp"
#include "core/mmemory.hpp"
#include "systems/resource_system.hpp"

namespace LoaderUtils
{
    bool ResourceUnload(class ResourceLoader* self, Resource* resource, MemoryTag tag);
} // namespace LoaderUtils
