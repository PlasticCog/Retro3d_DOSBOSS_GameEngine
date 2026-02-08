#pragma once
#include "core/types.h"
#include <string>
#include <vector>

namespace TimelinePanel {
    struct State {
        int currentFrame = 0;
        int totalFrames = 60;
        bool playing = false;
    };

    void draw(State& state, std::vector<SceneObject>& objects,
              const std::string& selectedId, std::vector<Keyframe>& keyframes);
}
