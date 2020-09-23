
#include "GameDebug.h"
#include "Events/KeyEvent.h"
#include "Events/EventFuncFwd.h"
#include "EventHandler/EventHandler.h"

#include "Physics/PhysicsDebugDrawer.h"
#include "ImGuiImpl/ImGuiWidgets.h"

#include "imgui/imgui.h"

bool game::g_draw_client_viewport = false;
bool game::g_draw_navmesh = false;
bool game::g_draw_transformsystem = false;
bool game::g_draw_fps = false;
bool game::g_draw_physics = false;
bool game::g_draw_audio = false;
bool game::g_draw_triggers = false;
uint32_t game::g_draw_physics_subcomponents = mono::PhysicsDebugComponents::DRAW_SHAPES;
bool game::g_draw_physics_stats = false;
bool game::g_draw_particle_stats = false;
bool game::g_draw_network_stats = false;

void DrawDebugMenu(const mono::UpdateContext& update_context, bool& show_window)
{
    constexpr int flags =
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize;

    ImGui::Begin("Debug", &show_window, flags);

    ImGui::Checkbox("Draw Client Viewport", &game::g_draw_client_viewport);
    ImGui::Checkbox("Draw Navmesh",         &game::g_draw_navmesh);
    ImGui::Checkbox("Draw TransformSystem", &game::g_draw_transformsystem);
    ImGui::Checkbox("Draw Physics",         &game::g_draw_physics);
    ImGui::Indent();
    mono::DrawBitFieldType(
        game::g_draw_physics_subcomponents, mono::all_physics_debug_component, std::size(mono::all_physics_debug_component), mono::PhsicsDebugComponentToString);
    ImGui::Unindent();
    ImGui::Checkbox("Draw Audio",           &game::g_draw_audio);
    ImGui::Checkbox("Draw Triggers",        &game::g_draw_triggers);
    ImGui::Checkbox("Draw FPS",             &game::g_draw_fps);
    ImGui::Checkbox("Draw Physics Stats",   &game::g_draw_physics_stats);
    ImGui::Checkbox("Draw Particle Stats",  &game::g_draw_particle_stats);
    ImGui::Checkbox("Draw Network Stats",   &game::g_draw_network_stats);

    ImGui::End();
}

game::DebugUpdater::DebugUpdater(mono::EventHandler* event_handler)
    : m_event_handler(event_handler)
    , m_draw(false)
{
    const event::KeyUpEventFunc key_up_func = [this](const event::KeyUpEvent& event) {
        if(event.key == Keycode::R)
            m_draw = !m_draw;
        return mono::EventResult::PASS_ON;
    };

    m_keyup_token = m_event_handler->AddListener(key_up_func);
}

game::DebugUpdater::~DebugUpdater()
{
    m_event_handler->RemoveListener(m_keyup_token);
}

void game::DebugUpdater::Update(const mono::UpdateContext& update_context)
{
    ImGui::GetIO().DeltaTime = float(update_context.delta_ms) / 1000.0f;
    ImGui::NewFrame();

    if(m_draw)
        DrawDebugMenu(update_context, m_draw);

    //ImGui::ShowDemoWindow();
    ImGui::Render();
}
