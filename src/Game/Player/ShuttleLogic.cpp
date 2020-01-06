
#include "ShuttleLogic.h"
#include "AIKnowledge.h"

#include "SystemContext.h"
#include "TransformSystem/TransformSystem.h"
#include "Physics/PhysicsSystem.h"

#include "Factories.h"
#include "Weapons/IWeaponSystem.h"
#include "Weapons/IWeaponFactory.h"

using namespace game;

ShuttleLogic::ShuttleLogic(
    uint32_t entity_id,
    PlayerInfo* player_info,
    mono::EventHandler& event_handler,
    const System::ControllerState& controller,
    mono::SystemContext* system_context)
    : m_entity_id(entity_id)
    , m_player_info(player_info)
    , m_gamepad_controller(this, event_handler, controller)
    , m_interaction_controller(this, event_handler)
    , m_fire(false)
    , m_total_ammo_left(500)
{
    m_transform_system = system_context->GetSystem<mono::TransformSystem>();
    m_physics_system = system_context->GetSystem<mono::PhysicsSystem>();

    // Make sure we have a weapon
    SelectWeapon(WeaponType::STANDARD);
}

void ShuttleLogic::Update(uint32_t delta_ms)
{
    m_gamepad_controller.Update(delta_ms);
    //m_pool->doUpdate(delta_ms);

    const math::Matrix& transform = m_transform_system->GetTransform(m_entity_id);
    const math::Vector& position = math::GetPosition(transform);

    if(m_fire)
    {
        const float rotation = math::GetZRotation(transform);
        m_weapon->Fire(position, rotation);
    }

    m_player_info->position = position;
    m_player_info->magazine_left = m_weapon->AmmunitionLeft();
    m_player_info->magazine_capacity = m_weapon->MagazineSize();
    m_player_info->ammunition_left = m_total_ammo_left;
    m_player_info->weapon_type = m_weapon_type;
}

void ShuttleLogic::Fire()
{
    m_fire = true;
}

void ShuttleLogic::StopFire()
{
    m_fire = false;
}

void ShuttleLogic::Reload()
{
    m_total_ammo_left -= m_weapon->MagazineSize() + m_weapon->AmmunitionLeft();
    m_total_ammo_left = std::max(0, m_total_ammo_left);

    if(m_total_ammo_left != 0)
        m_weapon->Reload();
}

void ShuttleLogic::SelectWeapon(WeaponType weapon)
{
    m_weapon = g_weapon_factory->CreateWeapon(weapon, WeaponFaction::PLAYER); //, m_pool.get());
    m_weapon_type = weapon;
}

void ShuttleLogic::ApplyImpulse(const math::Vector& force)
{
    const math::Matrix& transform = m_transform_system->GetTransform(m_entity_id);
    mono::IBody* body = m_physics_system->GetBody(m_entity_id);

    body->ApplyImpulse(force, math::GetPosition(transform));
}

void ShuttleLogic::SetRotation(float rotation)
{
    mono::IBody* body = m_physics_system->GetBody(m_entity_id);
    body->SetAngle(rotation);
}

void ShuttleLogic::GiveAmmo(int value)
{
    m_total_ammo_left += value;
}

void ShuttleLogic::GiveHealth(int value)
{
    printf("Give Health!\n");
}

uint32_t ShuttleLogic::EntityId() const
{
    return m_entity_id;
}

