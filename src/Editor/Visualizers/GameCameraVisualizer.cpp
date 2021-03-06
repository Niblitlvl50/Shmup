
#include "GameCameraVisualizer.h"
#include "Math/Quad.h"
#include "Rendering/IRenderer.h"
#include "Rendering/Color.h"

using namespace editor;

GameCameraVisualizer::GameCameraVisualizer(
    const bool& enabled, const math::Vector& player_spawn, const math::Vector& position, const math::Vector& size)
    : m_enabled(enabled)
    , m_player_spawn(player_spawn)
    , m_position(position)
    , m_size(size)
{ }

void GameCameraVisualizer::Draw(mono::IRenderer& renderer) const
{
    if(!m_enabled)
        return;

    const math::Vector half_size = m_size / 2.0f;
    renderer.DrawQuad(math::Quad(m_position - half_size, m_position + half_size), mono::Color::RED, 2.0f);

    renderer.DrawPoints({ m_player_spawn }, mono::Color::CYAN, 10.0f);
}

math::Quad GameCameraVisualizer::BoundingBox() const
{
    const math::Vector half_size = m_size / 2.0f;
    return math::Quad(m_position - half_size, m_position + half_size);
}
