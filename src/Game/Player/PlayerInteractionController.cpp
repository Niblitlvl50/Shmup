
#include "PlayerInteractionController.h"
#include "Events/PickupEvent.h"
#include "Player/Shuttle.h"

#include "Audio/AudioFactory.h"
#include "EventHandler/EventHandler.h"


namespace
{
    using PickupFunc = void (game::Shuttle::*)(int amount);

    constexpr PickupFunc pickup_func_table[] = {
        &game::Shuttle::GiveAmmo,
        &game::Shuttle::GiveHealth
    };
}

using namespace game;

PlayerInteractionController::PlayerInteractionController(Shuttle* player, mono::EventHandler& event_handler)
    : m_player(player)
    , m_event_handler(event_handler)
{
    using namespace std::placeholders;
    const std::function<bool (const game::PickupEvent&)>& pickup_func = std::bind(&PlayerInteractionController::OnPickup, this, _1);
    m_pickup_token = m_event_handler.AddListener(pickup_func);

    m_pickup_sound = mono::AudioFactory::CreateSound("res/sound/item_pickup.wav", false, true);
}

PlayerInteractionController::~PlayerInteractionController()
{
    m_event_handler.RemoveListener(m_pickup_token);
}

bool PlayerInteractionController::OnPickup(const game::PickupEvent& event)
{
    if(event.entity_id != m_player->Id())
        return false;

    const PickupFunc pickup_func = pickup_func_table[event.type];
    (m_player->*pickup_func)(event.value);
    m_pickup_sound->Play();

    return true;
}