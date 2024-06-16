#include "loader_utils.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

bool LoaderUtils::ResourceUnload(ResourceLoader *self, Resource &resource, MemoryTag tag)
{
    if (!self) {
        MWARN("ResourceUnload вызывается с nullptr для себя или ресурса.");
        return false;
    }

    /*u32 PathLength = resource->FullPath.Length();
    if (PathLength) {
        resource->FullPath.Destroy();
    }*/

    if (resource.data) {
        //MMemory::Free(resource.data, resource.DataSize, tag);
        resource.data = nullptr;
        resource.DataSize = 0;
        resource.LoaderID = INVALID::ID;
    }

    return true;
}