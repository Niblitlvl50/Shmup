
#include "Prefab.h"
#include "SnapPoint.h"
#include "Rendering/Sprite/ISprite.h"
#include "Rendering/Sprite/SpriteFactory.h"
#include "Rendering/IRenderer.h"
#include "Rendering/Color.h"
#include "Math/Quad.h"

using namespace editor;

Prefab::Prefab(const std::string& name, const std::string& sprite_file, const std::vector<SnapPoint>& snap_points)
    : m_name(name),
      m_snap_points(snap_points),
      m_selected(false)
{
    m_sprite = mono::CreateSprite(sprite_file.c_str());
    for(SnapPoint& snapper : m_snap_points)
        snapper.id = Id();
}

void Prefab::Draw(mono::IRenderer& renderer) const
{
    renderer.DrawSprite(*m_sprite);

    if(m_selected)
    {
        math::Quad bb(-0.5f, -0.5f, 0.5f, 0.5f);
        renderer.DrawQuad(bb, mono::Color::RGBA(0.0f, 1.0f, 0.0f), 2.0f);
    }
}

void Prefab::Update(unsigned int delta)
{
    m_sprite->doUpdate(delta);
}

const std::string& Prefab::Name() const
{
    return m_name;
}

void Prefab::SetSelected(bool selected)
{
    m_selected = selected;
}

const std::vector<SnapPoint>& Prefab::SnapPoints() const
{
    return m_snap_points;
}
