
#include "TitleScreen.h"
#include "UI/TextEntity.h"
#include "UI/Background.h"
#include "RenderLayers.h"
#include "Actions/MoveAction.h"

#include "Math/Quad.h"
#include "Rendering/ICamera.h"
#include "Rendering/Color.h"

#include "Events/KeyEvent.h"
#include "Events/QuitEvent.h"
#include "Events/EventFuncFwd.h"
#include "EventHandler/EventHandler.h"

using namespace game;

namespace
{
    class MoveContextUpdater : public mono::IUpdatable
    {
    public:

        MoveContextUpdater(std::vector<MoveActionContext>& contexts)
            : m_contexts(contexts)
        { }

        void doUpdate(unsigned int delta)
        {
            game::UpdateMoveContexts(delta, m_contexts);
        }

        std::vector<MoveActionContext>& m_contexts;
    };
}

TitleScreen::TitleScreen(mono::EventHandler& event_handler)
    : m_event_handler(event_handler)
{
    using namespace std::placeholders;
    const event::KeyUpEventFunc& key_callback = std::bind(&TitleScreen::OnKeyUp, this, _1);
    m_key_token = m_event_handler.AddListener(key_callback);
}

TitleScreen::~TitleScreen()
{
    m_event_handler.RemoveListener(m_key_token);
}

bool TitleScreen::OnKeyUp(const event::KeyUpEvent& event)
{
    if(event.key == Keycode::ENTER)
        m_event_handler.DispatchEvent(event::QuitEvent());

    return false;
}

void TitleScreen::OnLoad(mono::ICameraPtr& camera)
{
    const math::Quad& viewport = camera->GetViewport();
    const math::Vector new_position(viewport.mB.x + (viewport.mB.x / 2.0f), viewport.mB.y / 3.0f);

    AddUpdatable(std::make_shared<MoveContextUpdater>(m_move_contexts));

    auto background1 = std::make_shared<Background>(viewport, mono::Color::HSL(0.1f, 0.3f, 0.5f));
    auto background2 = std::make_shared<Background>(viewport, mono::Color::HSL(0.6f, 0.3f, 0.5f));

    auto title_text = std::make_shared<TextEntity>("Shoot, Survive!", FontId::PIXELETTE_LARGE, false);
    title_text->m_shadow_color = mono::Color::RGBA(0.8, 0.0, 0.8, 1.0);
    title_text->m_text_color = mono::Color::RGBA(0.9, 0.8, 0.8, 1.0f);
    title_text->SetPosition(new_position);

    const math::Vector dont_die_position(viewport.mB.x * 2.0f, viewport.mB.y / 4.0f);

    auto dont_die_text = std::make_shared<TextEntity>("...dont die.", FontId::PIXELETTE_SMALL, false);
    dont_die_text->m_shadow_color = mono::Color::RGBA(0.8, 0.0, 0.8, 1.0);
    dont_die_text->m_text_color = mono::Color::RGBA(0.9, 0.9, 0.0, 1.0f);
    dont_die_text->SetPosition(dont_die_position);

    auto hit_enter_text = std::make_shared<TextEntity>("Hit enter", FontId::PIXELETTE_SMALL, true);
    hit_enter_text->m_shadow_color = mono::Color::RGBA(0.3, 0.3, 0.3, 1);
    hit_enter_text->SetPosition(math::Vector(viewport.mB.x / 2.0f, 2.0f));

    game::MoveActionContext title_text_animation;
    title_text_animation.entity = title_text;
    title_text_animation.duration = 500;
    title_text_animation.start_position = new_position;
    title_text_animation.end_position = math::Vector(1.0f, new_position.y);
    title_text_animation.ease_func = EaseOutCubic;

    game::MoveActionContext dont_die_animation;
    dont_die_animation.entity = dont_die_text;
    dont_die_animation.duration = 1000;
    dont_die_animation.start_position = dont_die_position;
    dont_die_animation.end_position = math::Vector(5.0f, dont_die_position.y);
    dont_die_animation.ease_func = EaseOutCubic;

    game::MoveActionContext background_animation;
    background_animation.entity = background1;
    background_animation.duration = 1000;
    background_animation.start_position = math::Vector(new_position.x, 0.0f);
    background_animation.end_position = math::Vector(0.0f, 0.0f);
    background_animation.ease_func = EaseInOutCubic;

    game::MoveActionContext hit_enter_animation;
    hit_enter_animation.entity = hit_enter_text;
    hit_enter_animation.duration = 800;
    hit_enter_animation.start_position = hit_enter_text->Position();
    hit_enter_animation.end_position = hit_enter_text->Position() + math::Vector(0.0f, 0.4f);
    hit_enter_animation.ease_func = EaseInOutCubic;
    hit_enter_animation.ping_pong = true;

    m_move_contexts.push_back(title_text_animation);
    m_move_contexts.push_back(dont_die_animation);
    m_move_contexts.push_back(background_animation);
    m_move_contexts.push_back(hit_enter_animation);

    AddEntity(background2, LayerId::BACKGROUND);
    AddEntity(background1, LayerId::BACKGROUND);
    AddEntity(title_text, LayerId::FOREGROUND);
    AddEntity(dont_die_text, LayerId::FOREGROUND);
    AddEntity(hit_enter_text, LayerId::FOREGROUND);
}

void TitleScreen::OnUnload()
{

}