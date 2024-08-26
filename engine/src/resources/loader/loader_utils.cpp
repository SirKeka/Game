#include "loader_utils.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

bool LoaderUtils::ResourceUnload(ResourceLoader *self, Resource &resource, Memory::Tag tag)
{
    if (!self) {
        MWARN("ResourceUnload вызывается с nullptr для себя или ресурса.");
        return false;
    }

    if (resource.FullPath) {
        //u16 PathLength = resource.FullPath.Length();
        if (resource.FullPath) {
            resource.FullPath.Clear();
        }
    }

    if (resource.data) {
        //MMemory::Free(resource.data, resource.DataSize, tag);
        resource.data = nullptr;
        resource.DataSize = 0;
        resource.LoaderID = INVALID::ID;
    }

    return true;
}