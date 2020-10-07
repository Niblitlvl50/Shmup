
#include "SystemTestZone.h"

#include "Audio/AudioDebugDrawer.h"

#include "EventHandler/EventHandler.h"
#include "Events/GameEventFuncFwd.h"

#include "Rendering/Sprite/SpriteSystem.h"
#include "Rendering/Sprite/SpriteBatchDrawer.h"
#include "Rendering/Text/TextSystem.h"
#include "Rendering/Text/TextBatchDrawer.h"

#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsDebugDrawer.h"

#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemDrawer.h"

#include "SystemContext.h"
#include "EntitySystem/EntitySystem.h"
#include "TransformSystem/TransformSystem.h"
#include "TransformSystem/TransformSystemDrawer.h"

#include "Util/Random.h"

#include "AIKnowledge.h"

#include "Hud/PlayerUIElement.h"
#include "Hud/Overlay.h"
#include "Hud/Healthbar.h"
#include "Hud/Debug/FPSElement.h"
#include "Hud/Debug/PhysicsStatsElement.h"
#include "Hud/Debug/ConsoleDrawer.h"
#include "Hud/Debug/NetworkStatusDrawer.h"
#include "Hud/Debug/ClientViewportVisualizer.h"
#include "Hud/Debug/ParticleStatusDrawer.h"

#include "Navigation/NavmeshFactory.h"
#include "Navigation/NavMeshVisualizer.h"

#include "Network/ServerManager.h"
#include "Network/ServerReplicator.h"
#include "Network/NetworkMessage.h"

#include "UpdateTasks/ListenerPositionUpdater.h"
#include "UpdateTasks/GameCamera.h"

#include "EntitySystem/IEntityManager.h"
#include "Factories.h"

#include "RenderLayers.h"
#include "Player/PlayerDaemon.h"
#include "DamageSystem.h"
#include "WorldFile.h"
#include "World/StaticBackground.h"
#include "Camera/ICamera.h"
#include "GameDebug.h"
#include "GameDebugDrawer.h"

#include "TriggerSystem.h"
#include "TriggerDebugDrawer.h"

#include "ImGuiImpl/ImGuiInputHandler.h"

using namespace game;

namespace
{
    class SyncPoint : public mono::IUpdatable
    {
        void Update(const mono::UpdateContext& update_context)
        {
            g_entity_manager->Sync();
        }
    };
}

SystemTestZone::SystemTestZone(const ZoneCreationContext& context)
    : m_system_context(context.system_context)
    , m_event_handler(context.event_handler)
    , m_game_config(*context.game_config)
{
    using namespace std::placeholders;
    const std::function<mono::EventResult (const RemoteCameraMessage&)> camera_func = std::bind(&SystemTestZone::HandleRemoteCamera, this, _1);
    m_camera_func_token = m_event_handler->AddListener(camera_func);
}

SystemTestZone::~SystemTestZone()
{
    m_event_handler->RemoveListener(m_camera_func_token);
}

void SystemTestZone::OnLoad(mono::ICamera* camera)
{
    const shared::LevelData leveldata = shared::ReadWorldComponentObjects("res/world.components", g_entity_manager, nullptr);
    m_loaded_entities = leveldata.loaded_entities;

    camera->SetViewport(math::Quad(leveldata.metadata.camera_position, leveldata.metadata.camera_size));
    m_camera = camera;

    m_debug_input = std::make_unique<ImGuiInputHandler>(*m_event_handler);

    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::TransformSystem* transform_system = m_system_context->GetSystem<mono::TransformSystem>();
    mono::PhysicsSystem* physics_system = m_system_context->GetSystem<mono::PhysicsSystem>();
    mono::SpriteSystem* sprite_system = m_system_context->GetSystem<mono::SpriteSystem>();
    mono::TextSystem* text_system = m_system_context->GetSystem<mono::TextSystem>();
    mono::ParticleSystem* particle_system = m_system_context->GetSystem<mono::ParticleSystem>();
    DamageSystem* damage_system = m_system_context->GetSystem<DamageSystem>();
    TriggerSystem* trigger_system = m_system_context->GetSystem<TriggerSystem>();

    m_game_camera = std::make_unique<GameCamera>(camera, transform_system, *m_event_handler);

    // Network and syncing should be done first in the frame.
    m_server_manager = std::make_unique<ServerManager>(m_event_handler, &m_game_config);
    AddUpdatable(m_server_manager.get());
    AddUpdatable(
        new ServerReplicator(
            entity_system,
            transform_system,
            sprite_system,
            g_entity_manager,
            m_server_manager.get(),
            m_game_config.server_replication_interval)
        );
    AddUpdatable(new SyncPoint()); // this should probably go last or something...

    AddUpdatable(new ListenerPositionUpdater());
    AddUpdatable(m_game_camera.get());

    m_player_daemon =
        std::make_unique<PlayerDaemon>(m_game_camera.get(), m_server_manager.get(), m_system_context, *m_event_handler);

    // Nav mesh
    std::vector<ExcludeZone> exclude_zones;
    m_navmesh.points = game::GenerateMeshPoints(math::Vector(-100, -50), 150, 100, 3, exclude_zones);
    m_navmesh.nodes = game::GenerateMeshNodes(m_navmesh.points, 5, exclude_zones);
    game::g_navmesh = &m_navmesh;

    AddDrawable(new StaticBackground(), LayerId::BACKGROUND);
    AddDrawable(new mono::SpriteBatchDrawer(transform_system, sprite_system), LayerId::GAMEOBJECTS);
    AddDrawable(new mono::TextBatchDrawer(text_system, transform_system), LayerId::GAMEOBJECTS);
    AddDrawable(new mono::ParticleSystemDrawer(particle_system), LayerId::GAMEOBJECTS);
    AddDrawable(new HealthbarDrawer(damage_system, transform_system, entity_system), LayerId::UI);

    UIOverlayDrawer* hud_overlay = new UIOverlayDrawer();
    hud_overlay->AddChild(new PlayerUIElement(g_player_one, math::Vector(0.0f, 0.0f), math::Vector(-100.0f, 0.0f)));
    hud_overlay->AddChild(new PlayerUIElement(g_player_two, math::Vector(277.0f, 0.0f), math::Vector(320.0f, 0.0f)));

    // Debug ui
    hud_overlay->AddChild(new FPSElement(math::Vector(2.0f, 2.0f), mono::Color::BLACK));
    hud_overlay->AddChild(new PhysicsStatsElement(physics_system, math::Vector(2.0f, 190.0f), mono::Color::BLACK));
    hud_overlay->AddChild(new NetworkStatusDrawer(math::Vector(2.0f, 190.0f), m_server_manager.get()));
    hud_overlay->AddChild(new ParticleStatusDrawer(particle_system, math::Vector(2, 190)));

    AddEntity(hud_overlay, LayerId::UI);

    // Debug
    AddUpdatable(new DebugUpdater(m_event_handler));
    AddDrawable(new GameDebugDrawer(), LayerId::GAMEOBJECTS_DEBUG);
    AddDrawable(new ClientViewportVisualizer(m_server_manager->GetConnectedClients()), LayerId::UI);
    AddDrawable(new NavmeshVisualizer(m_navmesh, *m_event_handler), LayerId::UI);
    AddDrawable(new mono::TransformSystemDrawer(g_draw_transformsystem, transform_system), LayerId::UI);
    AddDrawable(new mono::PhysicsDebugDrawer(g_draw_physics, g_draw_physics_subcomponents, physics_system, m_event_handler), LayerId::UI);
    AddDrawable(new mono::AudioDebugDrawer(g_draw_audio), LayerId::UI);
    AddDrawable(new TriggerDebugDrawer(g_draw_triggers, trigger_system, transform_system), LayerId::UI);
}

int SystemTestZone::OnUnload()
{
    for(uint32_t entity_id : m_loaded_entities)
        g_entity_manager->ReleaseEntity(entity_id);

    g_entity_manager->Sync();

    RemoveUpdatable(m_game_camera.get());
    RemoveUpdatable(m_server_manager.get());

    return 0;
}

mono::EventResult SystemTestZone::HandleRemoteCamera(const RemoteCameraMessage& message)
{
    //m_camera->SetPosition(message.position);
    //m_camera->SetViewport(message.viewport);
    return mono::EventResult::HANDLED;
}
