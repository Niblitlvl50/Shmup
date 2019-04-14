
#include "ComponentDrawFuncs.h"

#include "Rendering/IRenderer.h"
#include "Rendering/Color.h"
#include "Math/Quad.h"

#include "DefinedAttributes.h"


void editor::DrawCircleShapeDetails(
    mono::IRenderer& renderer, const math::Vector& position, const std::vector<Attribute>& component_properties)
{
    float radius_value;
    math::Vector offset;

    world::FindAttribute(world::RADIUS_ATTRIBUTE, component_properties, radius_value);
    world::FindAttribute(world::POSITION_ATTRIBUTE, component_properties, offset);

    constexpr mono::Color::RGBA color(1.0f, 0.0f, 1.0f);
    renderer.DrawCircle(position + offset, std::max(radius_value, 0.1f), 20, 1.0f, color);
}

void editor::DrawBoxShapeDetails(
    mono::IRenderer& renderer, const math::Vector& position, const std::vector<Attribute>& component_properties)
{
    float width;
    float height;
    math::Vector offset;

    world::FindAttribute(world::WIDTH_ATTRIBUTE, component_properties, width);
    world::FindAttribute(world::HEIGHT_ATTRIBUTE, component_properties, height);
    world::FindAttribute(world::POSITION_ATTRIBUTE, component_properties, offset);

    const math::Vector half_size = math::Vector(width, height) / 2.0f;

    math::Quad box;
    box.mA = position + offset - half_size;
    box.mB = position + offset + half_size;

    constexpr mono::Color::RGBA color(1.0f, 0.0f, 1.0f);
    renderer.DrawQuad(box, color, 1.0f);
}

void editor::DrawSegmentShapeDetails(
    mono::IRenderer& renderer, const math::Vector& position, const std::vector<Attribute>& component_properties)
{
    math::Vector start;
    math::Vector end;
    float radius;

    world::FindAttribute(world::START_ATTRIBUTE, component_properties, start);
    world::FindAttribute(world::END_ATTRIBUTE, component_properties, end);
    world::FindAttribute(world::RADIUS_ATTRIBUTE, component_properties, radius);

    const std::vector<math::Vector> line = {
        start + position,
        end + position
    };

    constexpr mono::Color::RGBA color(1.0f, 0.0f, 1.0f);
    renderer.DrawLines(line, color, std::max(radius, 1.0f));
}