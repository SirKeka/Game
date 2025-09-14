#pragma once

namespace Testbed
{
    enum PacketViews {
        Skybox,
        World,
        EditorWorld,
        UI,
        Pick,
        Max = Pick + 1
    };
} // namespace Testbed