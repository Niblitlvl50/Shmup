
#include "UIElements.h"
#include "Rendering/IRenderer.h"
#include "Rendering/Sprite/ISprite.h"
#include "Rendering/Sprite/SpriteFactory.h"

using namespace game;

UITextElement::UITextElement(int font_id, const std::string& text, bool centered, const mono::Color::RGBA& color)
    : m_font_id(font_id)
    , m_text(text)
    , m_centered(centered)
    , m_color(color)
{ }

void UITextElement::SetFontId(int new_font_id)
{
    m_font_id = new_font_id;
}

void UITextElement::SetText(const std::string& new_text)
{
    m_text = new_text;
}

void UITextElement::SetColor(const mono::Color::RGBA& new_color)
{
    m_color = new_color;
}

void UITextElement::Draw(mono::IRenderer& renderer) const
{
    renderer.DrawText(m_font_id, m_text.c_str(), math::ZeroVec, m_centered, m_color);
}

void UITextElement::Update(unsigned int delta)
{ }


UISpriteElement::UISpriteElement(const std::vector<std::string>& sprite_files)
    : m_active_sprite(0)
{
    for(const std::string& sprite_file : sprite_files)
        m_sprites.push_back(mono::CreateSprite(sprite_file.c_str()));
}

void UISpriteElement::SetActiveSprite(size_t index)
{
    m_active_sprite = index;
}

mono::ISpritePtr UISpriteElement::GetSprite(size_t index)
{
    return m_sprites[index];
}

void UISpriteElement::Draw(mono::IRenderer& renderer) const
{
    renderer.DrawSprite(*m_sprites[m_active_sprite]);
}

void UISpriteElement::Update(unsigned int delta)
{
    for(auto& sprite : m_sprites)
        sprite->doUpdate(delta);
}

UISquareElement::UISquareElement(const math::Quad& square, const mono::Color::RGBA& color)
    : UISquareElement(square, color, color, 0.0f)
{ }

UISquareElement::UISquareElement(
    const math::Quad& square, const mono::Color::RGBA& color, const mono::Color::RGBA& border_color, float border_width)
    : m_square(square)
    , m_color(color)
    , m_border_color(border_color)
    , m_border_width(border_width)
{ }

void UISquareElement::Draw(mono::IRenderer& renderer) const
{
    if(m_border_width > 0.0f)
    {
        const math::Vector border_shift(m_border_width, m_border_width);

        math::Quad border_square = m_square;
        border_square.mA -= border_shift;
        border_square.mB += border_shift;

        renderer.DrawFilledQuad(border_square, m_border_color);
    }

    renderer.DrawFilledQuad(m_square, m_color);
}

void UISquareElement::Update(unsigned int delta)
{ }