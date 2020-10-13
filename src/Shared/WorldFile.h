
#pragma once

#include "MonoFwd.h"
#include "Math/Vector.h"
#include <vector>
#include <cstdint>
#include <functional>

struct Component;

namespace shared
{
    struct LevelMetadata
    {
        math::Vector camera_position;
        math::Vector camera_size;
    };

    struct LevelData
    {
        LevelMetadata metadata;
        std::vector<uint32_t> loaded_entities;
    };

    using EntityCreationCallback
        = std::function<void (const mono::Entity& entity, const std::string& folder, const std::vector<Component>& components)>;
    LevelData ReadWorldComponentObjects(const char* file_name, mono::IEntityManager* entity_manager, EntityCreationCallback callback);
}
