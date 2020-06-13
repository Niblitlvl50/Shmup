
#pragma once

#include "Zone/EntityBase.h"
#include "Util/FpsCounter.h"
#include "Rendering/Color.h"

namespace game
{
    class FPSElement : public mono::EntityBase
    {
    public:

        FPSElement(const math::Vector& position);
        FPSElement(const math::Vector& position, const mono::Color::RGBA& color);
        void EntityDraw(mono::IRenderer& renderer) const override;
        void EntityUpdate(const mono::UpdateContext& update_context) override;

    private:
        mono::FpsCounter m_counter;
        mono::Color::RGBA m_color;
    };
}