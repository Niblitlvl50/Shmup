
#include "PlayerDaemon.h"
#include "PlayerLogic.h"
#include "AIKnowledge.h"

#include "Factories.h"
#include "EntitySystem/Entity.h"
#include "EntitySystem/IEntityManager.h"

#include "SystemContext.h"
#include "DamageSystem.h"
#include "GameCamera/CameraSystem.h"
#include "TransformSystem/TransformSystem.h"
#include "Entity/EntityLogicSystem.h"

#include "EventHandler/EventHandler.h"
#include "Events/EventFuncFwd.h"
#include "Events/ControllerEvent.h"
#include "Events/PlayerConnectedEvent.h"
#include "Events/ScoreEvent.h"

#include "Network/NetworkMessage.h"
#include "Network/INetworkPipe.h"
#include "Network/NetworkSerialize.h"

#include "Component.h"

#include <functional>

using namespace game;

/*
#define IS_TRIGGERED(variable) (!m_last_state.variable && state.variable)
#define HAS_CHANGED(variable) (m_last_state.variable != state.variable)

namespace
{
    class PlayerDeathScreen : public mono::EntityBase
    {
    public:
        PlayerDeathScreen(PlayerDaemon* player_daemon, mono::EventHandler& event_handler, const math::Vector& position)
            : m_player_daemon(player_daemon), m_event_handler(event_handler)
        {
            m_position = position;

            const std::vector<UIDialog::Option> options = {
                { "Respawn",    "res/sprites/ps_cross.sprite",      0.35f },
                { "Quit",       "res/sprites/ps_triangle.sprite",   0.35f }
            };

            constexpr mono::Color::RGBA background_color(0, 0, 0);
            constexpr mono::Color::RGBA text_color(1, 0, 0);

            AddChild(new UIDialog("YOU DEAD! Respawn?", options, background_color, text_color));
        }

        void EntityUpdate(const mono::UpdateContext& update_context)
        {
            const System::ControllerState& state = System::GetController(System::ControllerId::Primary);

            const bool a_pressed = IS_TRIGGERED(a) && HAS_CHANGED(a);
            const bool y_pressed = IS_TRIGGERED(y) && HAS_CHANGED(y);

            if(a_pressed)
            {
                m_player_daemon->SpawnPlayer1();
                //m_event_handler.DispatchEvent(RemoveEntityEvent(Id()));
            }
            else if(y_pressed)
            {
                m_event_handler.DispatchEvent(event::QuitEvent());
            }

            m_last_state = state;
        }

        void EntityDraw(mono::IRenderer& renderer) const
        { }

        PlayerDaemon* m_player_daemon;
        mono::EventHandler& m_event_handler;
        System::ControllerState m_last_state;
    };
}
*/

namespace
{
    uint32_t SpawnPlayer(
        game::PlayerInfo* player_info,
        const math::Vector& spawn_position,
        const System::ControllerState& controller,
        mono::SystemContext* system_context,
        mono::EventHandler* event_handler,
        game::DamageCallback damage_callback)
    {
        mono::Entity player_entity = game::g_entity_manager->CreateEntity("res/entities/player_entity.entity");

        mono::TransformSystem* transform_system = system_context->GetSystem<mono::TransformSystem>();
        math::Matrix& transform = transform_system->GetTransform(player_entity.id);
        math::Position(transform, spawn_position);
        transform_system->SetTransformState(player_entity.id, mono::TransformState::CLIENT);

        // No need to store the callback id, when destroyed this callback will be cleared up.
        game::DamageSystem* damage_system = system_context->GetSystem<DamageSystem>();
        const uint32_t callback_id = damage_system->SetDamageCallback(player_entity.id, game::DamageType::DESTROYED, damage_callback);
        (void)callback_id;

        game::EntityLogicSystem* logic_system = system_context->GetSystem<EntityLogicSystem>();
        game::g_entity_manager->AddComponent(player_entity.id, BEHAVIOUR_COMPONENT);

        IEntityLogic* player_logic = new PlayerLogic(player_entity.id, player_info, *event_handler, controller, system_context);
        logic_system->AddLogic(player_entity.id, player_logic);

        player_info->entity_id = player_entity.id;
        player_info->is_active = true;

        return player_entity.id;
    }
}

PlayerDaemon::PlayerDaemon(
    INetworkPipe* remote_connection,
    mono::SystemContext* system_context,
    mono::EventHandler& event_handler,
    const math::Vector& player_one_spawn)
    : m_remote_connection(remote_connection)
    , m_system_context(system_context)
    , m_event_handler(event_handler)
    , m_player_state(PlayerMetaState::NONE)
    , m_player_one_spawn(player_one_spawn)
{
    m_camera_system = m_system_context->GetSystem<CameraSystem>();

    using namespace std::placeholders;
    const event::ControllerAddedFunc& added_func = std::bind(&PlayerDaemon::OnControllerAdded, this, _1);
    const event::ControllerRemovedFunc& removed_func = std::bind(&PlayerDaemon::OnControllerRemoved, this, _1);

    m_added_token = m_event_handler.AddListener(added_func);
    m_removed_token = m_event_handler.AddListener(removed_func);

    const PlayerConnectedFunc& connected_func = std::bind(&PlayerDaemon::PlayerConnected, this, _1);
    const PlayerDisconnectedFunc& disconnected_func = std::bind(&PlayerDaemon::PlayerDisconnected, this, _1);
    const ScoreFunc& score_func = std::bind(&PlayerDaemon::PLayerScore, this, _1);

    m_player_connected_token = m_event_handler.AddListener(connected_func);
    m_player_disconnected_token = m_event_handler.AddListener(disconnected_func);

    const std::function<mono::EventResult (const RemoteInputMessage&)>& remote_input_func = std::bind(&PlayerDaemon::RemoteInput, this, _1);
    m_remote_input_token = m_event_handler.AddListener(remote_input_func);

    m_score_token = m_event_handler.AddListener(score_func);

    if(System::IsControllerActive(System::ControllerId::Primary))
    {
        m_player_one_id = System::GetControllerId(System::ControllerId::Primary);
        SpawnPlayer1();
    }

    if(System::IsControllerActive(System::ControllerId::Secondary))
    {
        m_player_two_id = System::GetControllerId(System::ControllerId::Secondary);
        SpawnPlayer2();
    }
}

PlayerDaemon::~PlayerDaemon()
{
    m_event_handler.RemoveListener(m_added_token);
    m_event_handler.RemoveListener(m_removed_token);

    m_event_handler.RemoveListener(m_player_connected_token);
    m_event_handler.RemoveListener(m_player_disconnected_token);
    m_event_handler.RemoveListener(m_remote_input_token);
    m_event_handler.RemoveListener(m_score_token);

    if(g_player_one.is_active)
        game::g_entity_manager->ReleaseEntity(g_player_one.entity_id);

    if(g_player_two.is_active)
        game::g_entity_manager->ReleaseEntity(g_player_two.entity_id);
}

std::vector<uint32_t> PlayerDaemon::GetPlayerIds() const
{
    std::vector<uint32_t> ids;

    if(g_player_one.is_active)
        ids.push_back(g_player_one.entity_id);

    for(const auto& pair : m_remote_players)
        ids.push_back(pair.second.player_info.entity_id);

    return ids;
}

void PlayerDaemon::SpawnPlayer1()
{
    const auto destroyed_func = [this](uint32_t entity_id, int damage, uint32_t id_who_did_damage, DamageType type) {
        game::g_player_one.is_active = false;
        m_camera_system->Unfollow();
    };

    const uint32_t spawned_id = SpawnPlayer(
        &game::g_player_one,
        m_player_one_spawn,
        System::GetController(System::ControllerId::Primary),
        m_system_context,
        &m_event_handler,
        destroyed_func);
    
    m_camera_system->Follow(spawned_id, math::Vector(0.0f, 3.0f));
    m_player_state = PlayerMetaState::SPAWNED;
}

void PlayerDaemon::SpawnPlayer2()
{
    const auto destroyed_func = [](uint32_t entity_id, int damage, uint32_t id_who_did_damage, DamageType type) {
        game::g_player_two.is_active = false;
    };

    const uint32_t spawned_id = SpawnPlayer(
        &game::g_player_two,
        m_player_two_spawn,
        System::GetController(System::ControllerId::Secondary),
        m_system_context,
        &m_event_handler,
        destroyed_func);
    (void)spawned_id;
}

mono::EventResult PlayerDaemon::OnControllerAdded(const event::ControllerAddedEvent& event)
{
    if(!game::g_player_one.is_active)
    {
        SpawnPlayer1();
        m_player_one_id = event.id;
    }
    else
    {
        SpawnPlayer2();
        m_player_two_id = event.id;
    }

    return mono::EventResult::PASS_ON;
}

mono::EventResult PlayerDaemon::OnControllerRemoved(const event::ControllerRemovedEvent& event)
{
    if(event.id == m_player_one_id)
    {
        if(game::g_player_one.is_active)
            g_entity_manager->ReleaseEntity(game::g_player_one.entity_id);

        m_camera_system->Unfollow();
        game::g_player_one.entity_id = mono::INVALID_ID;
        game::g_player_one.is_active = false;
    }
    else if(event.id == m_player_two_id)
    {
        if(game::g_player_two.is_active)
            g_entity_manager->ReleaseEntity(game::g_player_two.entity_id);

        game::g_player_two.entity_id = mono::INVALID_ID;
        game::g_player_two.is_active = false;
    }

    return mono::EventResult::PASS_ON;
}

mono::EventResult PlayerDaemon::PlayerConnected(const PlayerConnectedEvent& event)
{
    auto it = m_remote_players.find(event.address);
    if(it != m_remote_players.end())
        return mono::EventResult::HANDLED;

    System::Log("PlayerDaemon|Player connected, %s\n", network::AddressToString(event.address).c_str());

    PlayerDaemon::RemotePlayerData& remote_player_data = m_remote_players[event.address];

    const auto remote_player_destroyed = [](uint32_t entity_id, int damage, uint32_t id_who_did_damage, DamageType type) {
        //game::entity_manager->ReleaseEntity(it->second.player_info.entity_id);
        //m_remote_players.erase(it);
    };

    const uint32_t spawned_id = SpawnPlayer(
        &remote_player_data.player_info,
        math::ZeroVec,
        remote_player_data.controller_state,
        m_system_context,
        &m_event_handler,
        remote_player_destroyed);

    ClientPlayerSpawned client_spawned_message;
    client_spawned_message.client_entity_id = spawned_id;

    NetworkMessage reply_message;
    reply_message.payload = SerializeMessage(client_spawned_message);

    m_remote_connection->SendMessageTo(reply_message, event.address);

    return mono::EventResult::HANDLED;
}

mono::EventResult PlayerDaemon::PlayerDisconnected(const PlayerDisconnectedEvent& event)
{
    auto it = m_remote_players.find(event.address);
    if(it != m_remote_players.end())
    {
        game::g_entity_manager->ReleaseEntity(it->second.player_info.entity_id);
        m_remote_players.erase(it);
    }

    return mono::EventResult::HANDLED;
}

mono::EventResult PlayerDaemon::RemoteInput(const RemoteInputMessage& event)
{
    auto it = m_remote_players.find(event.sender);
    if(it != m_remote_players.end())
        it->second.controller_state = event.controller_state;

    return mono::EventResult::HANDLED;
}

mono::EventResult PlayerDaemon::PLayerScore(const ScoreEvent& event)
{
    if(g_player_one.entity_id == event.entity_id)
        g_player_one.score += event.score;
    else if(g_player_two.entity_id == event.entity_id)
        g_player_two.score += event.score;

    return mono::EventResult::PASS_ON;
}


ClientPlayerDaemon::ClientPlayerDaemon(CameraSystem* camera_system, mono::EventHandler& event_handler)
    : m_camera_system(camera_system)
    , m_event_handler(event_handler)
{
    using namespace std::placeholders;
    const event::ControllerAddedFunc& added_func = std::bind(&ClientPlayerDaemon::OnControllerAdded, this, _1);
    const event::ControllerRemovedFunc& removed_func = std::bind(&ClientPlayerDaemon::OnControllerRemoved, this, _1);
    const std::function<mono::EventResult (const ClientPlayerSpawned&)>& client_spawned = std::bind(&ClientPlayerDaemon::ClientSpawned, this, _1);

    m_added_token = m_event_handler.AddListener(added_func);
    m_removed_token = m_event_handler.AddListener(removed_func);
    m_client_spawned_token = m_event_handler.AddListener(client_spawned);

    if(System::IsControllerActive(System::ControllerId::Primary))
    {
        m_player_one_controller_id = System::GetControllerId(System::ControllerId::Primary);
        SpawnPlayer1();
    }
}

ClientPlayerDaemon::~ClientPlayerDaemon()
{
    m_event_handler.RemoveListener(m_added_token);
    m_event_handler.RemoveListener(m_removed_token);
    m_event_handler.RemoveListener(m_client_spawned_token);

    m_camera_system->Unfollow();
}

void ClientPlayerDaemon::SpawnPlayer1()
{
    System::Log("PlayerDaemon|Spawn player 1\n");
    //game::g_player_one.entity_id = player_entity.id;
    //game::g_player_one.is_active = true;
}

mono::EventResult ClientPlayerDaemon::OnControllerAdded(const event::ControllerAddedEvent& event)
{
    if(!game::g_player_one.is_active)
    {
        SpawnPlayer1();
        m_player_one_controller_id = event.id;
    }

    return mono::EventResult::PASS_ON;
}

mono::EventResult ClientPlayerDaemon::OnControllerRemoved(const event::ControllerRemovedEvent& event)
{
    if(event.id == m_player_one_controller_id)
    {
        game::g_player_one.entity_id = mono::INVALID_ID;
        game::g_player_one.is_active = false;
        m_camera_system->Unfollow();
    }

    return mono::EventResult::PASS_ON;
}

mono::EventResult ClientPlayerDaemon::ClientSpawned(const ClientPlayerSpawned& message)
{
    game::g_player_one.entity_id = message.client_entity_id;
    game::g_player_one.is_active = true;

    m_camera_system->Follow(g_player_one.entity_id, math::ZeroVec);

    return mono::EventResult::PASS_ON;
}
