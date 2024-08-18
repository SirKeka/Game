#pragma once
#include "systems/resource_system.hpp"
#include "core/mmemory.hpp"

namespace LoaderUtils
{
    bool ResourceUnload(class ResourceLoader* self, Resource& resource, Memory::Tag tag);
} // namespace LoaderUtils
