
#pragma once

#include "MonoPtrFwd.h"
#include "Physics/IBody.h"

#include "Entity/IEntityLogic.h"
#include "WeaponConfiguration.h"

#include <vector>

namespace game
{
    class BulletLogic : public game::IEntityLogic, public mono::ICollisionHandler
    {
    public:

        BulletLogic(uint32_t entity_id, uint32_t owner_entity_id, const BulletConfiguration& config, mono::PhysicsSystem* physics_system);
        void Update(const mono::UpdateContext& update_context) override;
        mono::CollisionResolve OnCollideWith(mono::IBody* body, const math::Vector& collision_point, uint32_t categories) override;
        void OnSeparateFrom(mono::IBody* body) override;

        const uint32_t m_entity_id;
        const uint32_t m_owner_entity_id;

        BulletImpactCallback m_collision_callback;
        int m_life_span;
        mono::ISoundPtr m_sound;

        mono::PhysicsSystem* m_physics_system;
        BulletCollisionBehaviour m_bullet_behaviour;

        std::vector<uint32_t> m_jump_ids;
        int m_jumps_left;
    };
}
