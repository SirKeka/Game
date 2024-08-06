#include "camera_system.hpp"


CameraSystem* CameraSystem::state = nullptr;

struct CameraLookup {
    u16 id;
    u16 ReferenceCount;
    Camera c;
};

constexpr CameraSystem::CameraSystem()
{
}