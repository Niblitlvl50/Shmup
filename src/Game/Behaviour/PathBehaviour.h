
#pragma once

#include "MonoPtrFwd.h"
#include "Physics/CMFwd.h"
#include "Math/Vector.h"

namespace game
{
    class PathBehaviour
    {
    public:

        PathBehaviour(class Enemy* enemy, const mono::IPathPtr& path, mono::EventHandler& event_handler);
        ~PathBehaviour();

        void Run(unsigned int delta);

    private:

        mono::IPathPtr m_path;
        mono::EventHandler& m_event_handler;
        float m_current_position;

        mono::IBodyPtr m_control_body;
        mono::IConstraintPtr m_spring;
        math::Vector m_point;
    };
}
