
#include "StaticTerrainBlock.h"

#include "Rendering/IRenderer.h"
#include "Rendering/BufferFactory.h"
#include "Rendering/Texture/TextureFactory.h"

#include "Math/Quad.h"
#include "Math/MathFunctions.h"

#include "Rendering/Color.h"

StaticTerrainBlock::StaticTerrainBlock(size_t vertex_count, size_t polygon_count)
    : m_index(0)
{
    m_draw_data.reserve(polygon_count);

    m_vertex_buffer = mono::CreateRenderBuffer(mono::BufferTarget::ARRAY_BUFFER, mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_count * 2);
    m_texture_buffer = mono::CreateRenderBuffer(mono::BufferTarget::ARRAY_BUFFER, mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_count * 2);
    m_color_buffer = mono::CreateRenderBuffer(mono::BufferTarget::ARRAY_BUFFER, mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_count * 4);

    const std::vector<float> temp_buffer(vertex_count * 4, 0.0f);
    m_color_buffer->UpdateData(temp_buffer.data(), 0, vertex_count * 4);

    const size_t vertex_elements = (vertex_count + 2) * 2 * 2;
    const size_t color_elements = (vertex_count + 2) * 4 * 2;
    const size_t index_elements = vertex_elements * 3;
    m_vertex_buffer_2 = mono::CreateRenderBuffer(mono::BufferTarget::ARRAY_BUFFER, mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_elements);
    m_color_buffer_2 = mono::CreateRenderBuffer(mono::BufferTarget::ARRAY_BUFFER, mono::BufferType::STATIC, mono::BufferData::FLOAT, color_elements);
    m_index_buffer = mono::CreateRenderBuffer(mono::BufferTarget::ELEMENT_BUFFER, mono::BufferType::STATIC, mono::BufferData::INT, index_elements);

    const std::vector<float> temp_buffer2((vertex_count + 2) * 4 * 2, 1.0f);
    m_color_buffer_2->UpdateData(temp_buffer2.data(), 0, temp_buffer2.size());
}

void StaticTerrainBlock::AddPolygon(const world::PolygonData& polygon)
{
    TerrainDrawData draw_data;
    draw_data.offset = m_index;
    draw_data.count = polygon.vertices.size();
    draw_data.texture = mono::CreateTexture(polygon.texture);

    m_draw_data.emplace_back(draw_data);

    std::vector<math::Vector> texture_coordinates;
    texture_coordinates.reserve(polygon.vertices.size());

    math::Quad bounding_box = math::Quad(math::INF, math::INF, -math::INF, -math::INF);
    for(const math::Vector& vertex : polygon.vertices)
        bounding_box |= vertex;

    const math::Vector& repeate = (bounding_box.mB - bounding_box.mA) / 2.0f;
        
    for(const math::Vector& vertex : polygon.vertices)
        texture_coordinates.push_back(math::MapVectorInQuad(vertex, bounding_box) * repeate);

    m_vertex_buffer->UpdateData(polygon.vertices.data(), draw_data.offset * 2, draw_data.count * 2);
    m_texture_buffer->UpdateData(texture_coordinates.data(), draw_data.offset * 2, draw_data.count * 2);

    m_index += draw_data.count;

    std::vector<math::Vector> new_vertices; // = polygon.vertices;

    for(const math::Vector& vertex : polygon.vertices)
    {
        new_vertices.push_back(vertex);
        new_vertices.push_back(vertex + math::Vector(0.0f, -10.0f));
    }

    new_vertices.push_back(new_vertices[0]);
    new_vertices.push_back(new_vertices[1]);

    m_vertices = new_vertices.size();
    m_vertex_buffer_2->UpdateData(new_vertices.data(), 0, new_vertices.size() * 2);
}

void StaticTerrainBlock::doDraw(mono::IRenderer& renderer) const
{
    renderer.DrawTrianges(m_vertex_buffer_2.get(), m_color_buffer_2.get(), m_index_buffer.get(), m_vertices);

    for(const TerrainDrawData& draw_data : m_draw_data)
    {
        renderer.DrawGeometry(
            m_vertex_buffer.get(), m_texture_buffer.get(), draw_data.offset, draw_data.count, draw_data.texture);
        renderer.DrawPolyline(
            m_vertex_buffer.get(), m_color_buffer.get(), draw_data.offset, draw_data.count);
    }
}

math::Quad StaticTerrainBlock::BoundingBox() const
{
    return math::InfQuad;
}