#pragma once

namespace Testbed
{
    enum PacketViews {
        Skybox = 0,
        World = 1,
        UI = 2,
        Pick = 3,
        Max = Pick + 1
    };
} // namespace Testbed