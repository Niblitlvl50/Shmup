
#include "SpawnSystemDrawer.h"
#include "SpawnSystem.h"
#include "GameDebug.h"
#include "FontIds.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/IRenderer.h"
#include "Rendering/Color.h"
#include "Rendering/RenderBuffer/IRenderBuffer.h"
#include "TransformSystem/TransformSystem.h"
#include "Math/Quad.h"
#include "Math/MathFunctions.h"

#include "Effects/SmokeEffect.h"
#include "Effects/Lightning.h"
#include "Paths/PathFactory.h"

using namespace game;

SpawnSystemDrawer::SpawnSystemDrawer(SpawnSystem* spawn_system, mono::TransformSystem* transform_system, mono::ParticleSystem* particle_system, mono::IEntityManager* entity_manager)
    : m_spawn_system(spawn_system)
    , m_transform_system(transform_system)
    , m_smoke_effect(particle_system, entity_manager)
{
    m_texture = mono::GetTextureFactory()->CreateTexture("res/textures/particles/beam_white_vertical6.png");
}

void SpawnSystemDrawer::Draw(mono::IRenderer& renderer) const
{
    for(const SpawnSystem::SpawnEvent& event : m_spawn_system->GetSpawnEvents())
    {
        const int time_to_spawn = ((int)event.timestamp_to_spawn - (int)renderer.GetTimestamp());

        const auto it = m_path_draw_buffers.find(event.spawner_id);
        if(it == m_path_draw_buffers.end())
        {
            const math::Vector& to = math::GetPosition(event.transform);
            const math::Vector from = to + math::Vector(0.0f, 10.0f);
            const std::vector<math::Vector>& lightning_points = game::GenerateLightning(from, to, 5);

            mono::PathOptions options;
            options.closed = false;
            options.width = 0.25f;
            options.color = mono::Color::WHITE;
            options.uv_mode = mono::UVMode::NORMALIZED_DISTANCE;

            SpawnEffect spawn_effect;
            spawn_effect.buffers = mono::BuildPathDrawBuffers(mono::PathType::REGULAR, lightning_points, options);
            spawn_effect.emit_smoke = false;

            m_path_draw_buffers[event.spawner_id] = std::move(spawn_effect);
        }
        else if(time_to_spawn <= 300)
        {
            mono::Color::RGBA color = mono::Color::WHITE;
            color.alpha = math::Scale01(time_to_spawn, 0, 300);

            SpawnEffect& spawn_effect = it->second;
            auto& buffers = spawn_effect.buffers;

            const float scale_value = math::Scale01Clamped(time_to_spawn, 300, 225);
            const uint32_t n_indices = scale_value * buffers.indices->Size();

            renderer.DrawAnnotatedTrianges(
                buffers.vertices.get(), buffers.anotations.get(), buffers.indices.get(), m_texture.get(), color, 0, n_indices);

            if(!spawn_effect.emit_smoke)
            {
                const math::Vector& world_position = math::GetPosition(event.transform);
                m_smoke_effect.EmitSmokeAt(world_position);

                spawn_effect.emit_smoke = true;
            }
        }
        
        if(time_to_spawn <= 0)
            m_path_draw_buffers.erase(event.spawner_id);
    }

    if(!game::g_draw_spawn_points)
        return;

    const auto draw_spawn_points = [&renderer, this](
        uint32_t entity_id, const SpawnSystem::SpawnPointComponent& spawn_point, const SpawnSystem::SpawnPointInternalData& internal_data) {

        const char* active_string = internal_data.active ? "Active" : "Inactive";

        char text_buffer[128] = { };
        std::snprintf(text_buffer, std::size(text_buffer), "%s|%d/%d", active_string, internal_data.counter, spawn_point.interval);

        const math::Matrix& world_transform = m_transform_system->GetWorld(entity_id);
        const auto scope = mono::MakeTransformScope(world_transform, &renderer);
        renderer.DrawCircle(math::ZeroVec, spawn_point.radius, 16, 1.0f, mono::Color::RED);
        renderer.RenderText(shared::PIXELETTE_TINY, text_buffer, mono::Color::CYAN, mono::FontCentering::HORIZONTAL);
    };

    m_spawn_system->ForEeachWithInternalData(draw_spawn_points);
}

math::Quad SpawnSystemDrawer::BoundingBox() const
{
    return math::InfQuad;
}
