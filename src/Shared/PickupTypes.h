
#pragma once

#include <cstdint>

namespace shared
{
    enum class PickupType : uint32_t
    {
        AMMO,
        HEALTH
    };

    constexpr const char* pickup_items[] = {
        "Ammo",
        "Health"
    };

    inline const char* PickupTypeToString(PickupType pickup_type)
    {
        return pickup_items[static_cast<uint32_t>(pickup_type)];
    }
}
