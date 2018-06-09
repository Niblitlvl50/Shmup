
#include "Bullet.h"
#include "CollisionConfiguration.h"

#include "Physics/IBody.h"
#include "Physics/IShape.h"
#include "Physics/CMFactory.h"

#include "Particle/ParticleEmitter.h"

#include "Rendering/IRenderer.h"
#include "Rendering/Sprite/ISprite.h"
#include "Rendering/Sprite/SpriteFactory.h"
#include "Audio/ISound.h"
#include "Audio/AudioFactory.h"
#include "Random.h"

using namespace game;

Bullet::Bullet(const BulletConfiguration& config)
    : m_collisionCallback(config.collision_callback)
{
    m_scale = config.scale;

    m_physics.body = mono::PhysicsFactory::CreateBody(1.0f, 1.0f);
    m_physics.body->SetCollisionHandler(this);
    m_physics.body->SetNoDamping();

    mono::IShapePtr shape = mono::PhysicsFactory::CreateShape(
        m_physics.body,
        config.collision_radius, //* config.scale,
        math::Vector(0.0f, 0.0f)
    );

    shape->SetCollisionFilter(config.collision_category, config.collision_mask);

    m_physics.body->SetMoment(shape->GetInertiaValue());
    m_physics.shapes.push_back(shape);

    m_sprite = mono::CreateSprite(config.sprite_file);
    m_sprite->SetShade(config.sprite_shade);

    m_sound = mono::AudioFactory::CreateNullSound();

    if(config.sound_file)
    {
        m_sound = mono::AudioFactory::CreateSound(config.sound_file, true, false);
        m_sound->Play();
    }

    const float life_span = config.life_span + (mono::Random() * config.fuzzy_life_span);
    m_lifeSpan = life_span * 1000.0f;

    if(config.pool)
        m_emitter = std::make_unique<mono::ParticleEmitter>(config.emitter_config, config.pool);
}

Bullet::~Bullet()
{ }

void Bullet::Draw(mono::IRenderer& renderer) const
{
    renderer.DrawSprite(*m_sprite);
}

void Bullet::Update(unsigned int delta)
{
    m_sprite->doUpdate(delta);
    m_sound->Position(m_position.x, m_position.y);

    if(m_emitter)
    {
        m_emitter->SetPosition(m_position);
        m_emitter->doUpdate(delta);
    }

    m_lifeSpan -= delta;
    if(m_lifeSpan < 0)
        OnCollideWith(m_physics.body, 0);
}

void Bullet::OnCollideWith(const mono::IBodyPtr& body, unsigned int category)
{
    m_collisionCallback(this, body);
}
