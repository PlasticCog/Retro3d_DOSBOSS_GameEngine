#pragma once
#include "core/types.h"
#include <string>

struct Project {
    std::string name = "My Retro Game";
    std::vector<SceneObject> objects;
    std::vector<GameEvent> events;
    std::vector<Keyframe> keyframes;

    bool save(const std::string& filepath) const;
    bool load(const std::string& filepath);
    void clear();
    void createDefaultScene();
};
