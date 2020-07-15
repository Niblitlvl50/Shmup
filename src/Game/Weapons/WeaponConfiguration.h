
#pragma once

#include "Physics/PhysicsFwd.h"
#include "Particle/ParticleFwd.h"
#include "CollisionConfiguration.h"

#include <functional>

namespace game
{
    enum BulletCollisionFlag
    {
        APPLY_DAMAGE = 1,
        DESTROY_THIS = 2,
    };

    // entity_id is the bullet that collides with something
    // owner_entity_id is the owner of this bullet
    // imact_flags is what to do on impact
    // collide_with is the body that the bullet collides with
    using BulletImpactCallback =
        std::function<void (uint32_t entity_id, uint32_t owner_entity_id, BulletCollisionFlag impact_flags, mono::IBody* collide_with)>;

    enum class BulletCollisionBehaviour
    {
        NORMAL,
        BOUNCE,
        JUMPER,
        PASS_THROUGH
    };

    struct BulletConfiguration
    {
        float life_span = 1.0f;
        float fuzzy_life_span = 0.0f;

        const char* entity_file = nullptr;
        const char* sound_file = nullptr;

        BulletCollisionBehaviour bullet_behaviour = BulletCollisionBehaviour::NORMAL;
        shared::CollisionCategory collision_category = shared::CollisionCategory::STATIC;
        uint32_t collision_mask = 0;
        BulletImpactCallback collision_callback;
    };

    struct WeaponConfiguration
    {
        uint32_t owner_id = 0;
        int magazine_size = 10;
        int projectiles_per_fire = 1;
        float rounds_per_second = 1.0f;
        float fire_rate_multiplier = 1.0f;
        float max_fire_rate = 1.0f;
        float bullet_force = 1.0f;
        float bullet_spread_degrees = 0.0f;

        float reload_time = 1.0f;
        
        const char* fire_sound = nullptr;
        const char* out_of_ammo_sound = nullptr;
        const char* reload_sound = nullptr;

        BulletConfiguration bullet_config;
    };
}
