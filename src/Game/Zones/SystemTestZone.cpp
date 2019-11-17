
#include "SystemTestZone.h"

#include "EventHandler/EventHandler.h"
#include "Events/GameEventFuncFwd.h"

#include "Rendering/Sprite/SpriteSystem.h"
#include "Rendering/Sprite/SpriteBatchDrawer.h"

#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsDebugDrawer.h"

#include "SystemContext.h"
#include "EntitySystem.h"
#include "TransformSystem.h"
#include "Util/Random.h"

#include "AIKnowledge.h"

#include "Hud/WeaponStatusElement.h"
#include "Hud/Overlay.h"
#include "Hud/FPSElement.h"
#include "Hud/PhysicsStatsElement.h"
#include "Hud/PickupDrawer.h"
#include "Hud/WaveDrawer.h"
#include "Hud/ConsoleDrawer.h"
#include "Hud/Healthbar.h"
#include "Hud/NetworkStatusDrawer.h"
#include "Hud/ClientViewportVisualizer.h"

#include "Navigation/NavmeshFactory.h"
#include "Navigation/NavMeshVisualizer.h"

#include "Network/ServerManager.h"
#include "Network/ServerReplicator.h"
#include "Network/NetworkMessage.h"

#include "UpdateTasks/ListenerPositionUpdater.h"
#include "UpdateTasks/CameraViewportReporter.h"
#include "UpdateTasks/PickupUpdater.h"

#include "Pickups/Ammo.h"

#include "Entity/IEntityManager.h"
#include "Factories.h"
#include "DamageSystem.h"

#include "RenderLayers.h"
#include "Player/PlayerDaemon.h"

#include "WorldFile.h"

#include "Camera/ICamera.h"

using namespace game;

namespace
{
    class SyncPoint : public mono::IUpdatable
    {
        void doUpdate(const mono::UpdateContext& update_context)
        {
            entity_manager->Sync();
        }
    };
}

SystemTestZone::SystemTestZone(const ZoneCreationContext& context)
    : m_system_context(context.system_context)
    , m_event_handler(context.event_handler)
    , m_game_config(*context.game_config)
{
    using namespace std::placeholders;

    const std::function<bool (const TextMessage&)> text_func = std::bind(&SystemTestZone::HandleText, this, _1);
    const std::function<bool (const RemoteCameraMessage&)> camera_func = std::bind(&SystemTestZone::HandleRemoteCamera, this, _1);

    m_text_func_token = m_event_handler->AddListener(text_func);
    m_camera_func_token = m_event_handler->AddListener(camera_func);
}

SystemTestZone::~SystemTestZone()
{
    m_event_handler->RemoveListener(m_text_func_token);
    m_event_handler->RemoveListener(m_camera_func_token);
}

void SystemTestZone::OnLoad(mono::ICameraPtr& camera)
{
    m_camera = camera;

    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::TransformSystem* transform_system = m_system_context->GetSystem<mono::TransformSystem>();
    mono::PhysicsSystem* physics_system = m_system_context->GetSystem<mono::PhysicsSystem>();
    mono::SpriteSystem* sprite_system = m_system_context->GetSystem<mono::SpriteSystem>();
    DamageSystem* damage_system = m_system_context->GetSystem<DamageSystem>();

    // Network and syncing should be done first in the frame.
    m_server_manager = std::make_shared<ServerManager>(m_event_handler, &m_game_config);
    AddUpdatable(m_server_manager);
    AddUpdatable(
        std::make_shared<ServerReplicator>(entity_system, transform_system, sprite_system, entity_manager, m_server_manager.get(), m_game_config.server_replication_interval));
    AddUpdatable(std::make_shared<SyncPoint>());

    AddUpdatable(std::make_shared<ListenerPositionUpdater>());
    AddUpdatable(std::make_shared<CameraViewportReporter>(camera));
    AddUpdatable(std::make_shared<PickupUpdater>(m_pickups, *m_event_handler));

    AddDrawable(std::make_shared<ClientViewportVisualizer>(m_server_manager->GetConnectedClients()), LayerId::UI);
    AddDrawable(std::make_shared<PickupDrawer>(m_pickups), LayerId::GAMEOBJECTS);
    AddDrawable(std::make_shared<WaveDrawer>(*m_event_handler), LayerId::UI);
    m_console_drawer = std::make_shared<ConsoleDrawer>();
    AddDrawable(m_console_drawer, LayerId::UI);

    for(int index = 0; index < 250; ++index)
    {
        mono::Entity new_entity = entity_manager->CreateEntity("res/entities/pink_blolb.entity");
        const math::Vector position = math::Vector(mono::Random(0.0f, 22.0f), mono::Random(0.0f, 24.0f));
        mono::IBody* body = physics_system->GetBody(new_entity.id);
        body->SetPosition(position);

        m_loaded_entities.push_back(new_entity.id);
    }

    const std::vector<uint32_t>& loaded_entities = world::ReadWorldComponentObjects("res/world.components", entity_manager);
    m_loaded_entities.insert(m_loaded_entities.end(), loaded_entities.begin(), loaded_entities.end());

    {
        // Nav mesh
        std::vector<ExcludeZone> exclude_zones;
        m_navmesh.points = game::GenerateMeshPoints(math::Vector(-100, -50), 150, 100, 3, exclude_zones);
        m_navmesh.nodes = game::GenerateMeshNodes(m_navmesh.points, 5, exclude_zones);
        game::g_navmesh = &m_navmesh;
        
        //AddDrawable(std::make_shared<NavmeshVisualizer>(m_navmesh, *m_event_handler), UI);
    }

    //std::vector<math::Vector> player_points;
    //m_player_daemon = std::make_unique<PlayerDaemon>(camera, player_points, m_system_context, *m_event_handler);

    AddDrawable(std::make_shared<mono::SpriteBatchDrawer>(m_system_context), LayerId::GAMEOBJECTS);
    //AddDrawable(std::make_shared<mono::PhysicsDebugDrawer>(physics_system), UI);
    AddDrawable(std::make_shared<HealthbarDrawer>(damage_system, transform_system), LayerId::UI);

    auto hud_overlay = std::make_shared<UIOverlayDrawer>();
    hud_overlay->AddChild(std::make_shared<WeaponStatusElement>(g_player_one, math::Vector(10.0f, 10.0f), math::Vector(-50.0f, 10.0f)));
    hud_overlay->AddChild(std::make_shared<WeaponStatusElement>(g_player_two, math::Vector(277.0f, 10.0f), math::Vector(320.0f, 10.0f)));
    hud_overlay->AddChild(std::make_shared<FPSElement>(math::Vector(2.0f, 2.0f), mono::Color::BLACK));
    hud_overlay->AddChild(std::make_shared<PhysicsStatsElement>(physics_system, math::Vector(2.0f, 190.0f), mono::Color::BLACK));
    hud_overlay->AddChild(std::make_shared<NetworkStatusDrawer>(math::Vector(2.0f, 190.0f), m_server_manager.get()));
    AddEntity(hud_overlay, LayerId::UI);
}

int SystemTestZone::OnUnload()
{
    for(uint32_t entity_id : m_loaded_entities)
        entity_manager->ReleaseEntity(entity_id);

    entity_manager->Sync();
    return 0;
}

bool SystemTestZone::HandleText(const TextMessage& text_message)
{
    m_console_drawer->AddText(text_message.text, 1500);
    return true;
}

bool SystemTestZone::HandleRemoteCamera(const RemoteCameraMessage& message)
{
    //m_camera->SetPosition(message.position);
    //m_camera->SetViewport(message.viewport);
    return true;
}
