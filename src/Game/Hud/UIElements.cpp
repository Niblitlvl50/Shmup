
#include "UIElements.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/IRenderer.h"

#include "Rendering/RenderBuffer/BufferFactory.h"
#include "Rendering/Sprite/ISprite.h"
#include "Rendering/Sprite/ISpriteFactory.h"
#include "Rendering/Sprite/SpriteBufferFactory.h"

#include "Util/Algorithm.h"

using namespace game;

UIElement::UIElement()
    : m_scale(1.0f, 1.0f)
    , m_rotation(0.0f)
{ }

void UIElement::Update(const mono::UpdateContext& context)
{
    for(UIElement* child : m_ui_elements)
        child->Update(context);
}

void UIElement::Draw(mono::IRenderer& renderer) const
{
    const math::Matrix& model = Transform();

    for(const UIElement* ui : m_ui_elements)
    {
        const math::Matrix& transform = model * ui->Transform();
        const auto transform_scope = mono::MakeTransformScope(transform, &renderer);
        ui->Draw(renderer);
    }
}

math::Quad UIElement::BoundingBox() const
{
    return math::InfQuad;
}

void UIElement::SetPosition(const math::Vector& position)
{
    m_position = position;
}

void UIElement::SetScale(const math::Vector& scale)
{
    m_scale = scale;
}

void UIElement::SetRotation(float radians)
{
    m_rotation = radians;
}

math::Matrix UIElement::Transform() const
{
    return 
        math::CreateMatrixWithPosition(m_position) *
        math::CreateMatrixFromZRotation(m_rotation) *
        math::CreateMatrixWithScale(m_scale);
}

void UIElement::AddChild(UIElement* element)
{
    m_ui_elements.push_back(element);
}

void UIElement::RemoveChild(UIElement* element)
{
    mono::remove(m_ui_elements, element);
}


UIOverlay::UIOverlay(float width, float height)
{
    m_projection = math::Ortho(0, width, 0, height, -10, 10);
}

void UIOverlay::Draw(mono::IRenderer& renderer) const
{
    const auto projection_scope = mono::MakeProjectionScope(m_projection, &renderer);
    const auto view_scope = mono::MakeViewTransformScope(math::Matrix(), &renderer);
    UIElement::Draw(renderer);
}


UITextElement::UITextElement(int font_id, const std::string& text, mono::FontCentering centering, const mono::Color::RGBA& color)
    : m_font_id(font_id)
    , m_text(text)
    , m_centering(centering)
    , m_color(color)
{ }

void UITextElement::SetText(const std::string& new_text)
{
    m_text = new_text;
}

void UITextElement::Draw(mono::IRenderer& renderer) const
{
    renderer.RenderText(m_font_id, m_text.c_str(), m_color, m_centering);
}


UISpriteElement::UISpriteElement(const std::string& sprite_file)
    : UISpriteElement(std::vector<std::string>{ sprite_file })
{ }

UISpriteElement::UISpriteElement(const std::vector<std::string>& sprite_files)
    : m_active_sprite(0)
{
    for(const std::string& sprite_file : sprite_files)
    {
        m_sprites.push_back(mono::GetSpriteFactory()->CreateSprite(sprite_file.c_str()));
        m_sprite_buffers.push_back(mono::BuildSpriteDrawBuffers(m_sprites.back()->GetSpriteData()));
    }

    constexpr uint16_t indices[] = {
        0, 1, 2, 0, 2, 3
    };
    m_indices = mono::CreateElementBuffer(mono::BufferType::STATIC, 6, indices);
}

void UISpriteElement::SetActiveSprite(size_t index)
{
    m_active_sprite = index;
}

mono::ISprite* UISpriteElement::GetSprite(size_t index)
{
    return m_sprites[index].get();
}

void UISpriteElement::Update(const mono::UpdateContext& update_context)
{
    for(auto& sprite : m_sprites)
        sprite->Update(update_context);
}

void UISpriteElement::Draw(mono::IRenderer& renderer) const
{
    const mono::ISprite* sprite = m_sprites[m_active_sprite].get();
    const mono::SpriteDrawBuffers& buffers = m_sprite_buffers[m_active_sprite];
    renderer.DrawSprite(
        sprite,
        buffers.vertices.get(),
        buffers.offsets.get(),
        buffers.uv.get(),
        buffers.uv_flipped.get(),
        buffers.heights.get(),
        m_indices.get(),
        sprite->GetTexture(),
        sprite->GetCurrentFrameIndex() * buffers.vertices_per_sprite);
}

UISquareElement::UISquareElement(float width, float height, const mono::Color::RGBA& color)
    : UISquareElement(width, height, color, color, 0.0f)
{ }

UISquareElement::UISquareElement(
    float width, float height, const mono::Color::RGBA& color, const mono::Color::RGBA& border_color, float border_width)
    : m_border_width(border_width)
{
    const std::vector<math::Vector> vertex_data = {
        { 0.0f, 0.0f },
        { 0.0f, height },
        { width, height },
        { width, 0.0f }
    };
    const std::vector<mono::Color::RGBA> color_data(vertex_data.size(), color);
    const std::vector<mono::Color::RGBA> border_color_data(vertex_data.size(), border_color);

    m_vertices = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, 2, 4, vertex_data.data());
    m_colors = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, 4, 4, color_data.data());
    m_border_colors = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, 4, 4, border_color_data.data());

    constexpr uint16_t indices[] = {
        0, 1, 2, 0, 2, 3,   // Two triangles
        0, 1, 2, 3, 0       // Outline
    };
    m_indices = mono::CreateElementBuffer(mono::BufferType::STATIC, std::size(indices), indices);
}

void UISquareElement::Draw(mono::IRenderer& renderer) const
{
    renderer.DrawTrianges(m_vertices.get(), m_colors.get(), m_indices.get(), 0, 6);

    if(m_border_width > 0.0f)
        renderer.DrawPolyline(m_vertices.get(), m_border_colors.get(), m_indices.get(), 6, 5);
}
