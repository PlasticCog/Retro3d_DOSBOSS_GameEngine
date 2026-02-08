#include "core/types.h"
#include <cstdlib>
#include <ctime>
#include <sstream>

static int _uid_counter = 0;

std::string generateUID() {
    return "o" + std::to_string(++_uid_counter);
}

const char* primTypeName(PrimType t) {
    switch (t) {
        case PrimType::Box: return "box";
        case PrimType::Cylinder: return "cylinder";
        case PrimType::Cone: return "cone";
        case PrimType::Sphere: return "sphere";
        case PrimType::Wedge: return "wedge";
        case PrimType::Plane: return "plane";
    }
    return "box";
}

PrimType primTypeFromName(const std::string& name) {
    if (name == "cylinder") return PrimType::Cylinder;
    if (name == "cone") return PrimType::Cone;
    if (name == "sphere") return PrimType::Sphere;
    if (name == "wedge") return PrimType::Wedge;
    if (name == "plane") return PrimType::Plane;
    return PrimType::Box;
}

SceneObject SceneObject::create(PrimType type, const std::string& name) {
    SceneObject obj;
    obj.id = generateUID();
    if (name.empty()) {
        std::string tn = primTypeName(type);
        tn[0] = (char)toupper(tn[0]);
        obj.name = tn + "_" + std::to_string(_uid_counter);
    } else {
        obj.name = name;
    }
    obj.type = type;
    // Random color
    obj.color.r = (float)(rand() % 170 + 85) / 255.0f;
    obj.color.g = (float)(rand() % 170 + 85) / 255.0f;
    obj.color.b = (float)(rand() % 170 + 85) / 255.0f;
    return obj;
}

const std::vector<ConditionDef> CONDITIONS = {
    {"always", "Always", "Special", {}},
    {"start_of_frame", "Start of Frame", "Special", {}},
    {"key_pressed", "Key Pressed", "Input", {{"key", "key"}}},
    {"key_down", "Key Down", "Input", {{"key", "key"}}},
    {"key_released", "Key Released", "Input", {{"key", "key"}}},
    {"mouse_clicked", "Mouse Clicked", "Input", {}},
    {"collision", "Collision With", "Collision", {{"object", "object"}}},
    {"timer_equals", "Timer Equals", "Timer", {{"seconds", "number"}}},
    {"timer_every", "Every X Seconds", "Timer", {{"seconds", "number"}}},
    {"compare_var", "Compare Variable", "Variables", {{"variable", "string"}, {"comparison", "comparison"}, {"value", "number"}}},
    {"object_visible", "Object Visible", "Object", {{"object", "object"}}},
    {"object_invisible", "Object Invisible", "Object", {{"object", "object"}}},
    {"position_compare", "Compare Position", "Position", {{"object", "object"}, {"axis", "axis"}, {"comparison", "comparison"}, {"value", "number"}}},
};

const std::vector<ActionDef> ACTIONS = {
    {"move_object", "Move Object", "Movement", {{"object", "object"}, {"x", "number"}, {"y", "number"}, {"z", "number"}}},
    {"set_position", "Set Position", "Movement", {{"object", "object"}, {"x", "number"}, {"y", "number"}, {"z", "number"}}},
    {"rotate_object", "Rotate Object", "Movement", {{"object", "object"}, {"x", "number"}, {"y", "number"}, {"z", "number"}}},
    {"set_rotation", "Set Rotation", "Movement", {{"object", "object"}, {"x", "number"}, {"y", "number"}, {"z", "number"}}},
    {"destroy_object", "Destroy Object", "Object", {{"object", "object"}}},
    {"show_object", "Show Object", "Object", {{"object", "object"}}},
    {"hide_object", "Hide Object", "Object", {{"object", "object"}}},
    {"set_variable", "Set Variable", "Variables", {{"variable", "string"}, {"value", "number"}}},
    {"add_to_variable", "Add to Variable", "Variables", {{"variable", "string"}, {"value", "number"}}},
    {"play_sound", "Play Sound", "Sound", {{"sound", "string"}}},
    {"stop_sound", "Stop Sound", "Sound", {}},
    {"move_camera", "Move Camera", "Camera", {{"x", "number"}, {"y", "number"}, {"z", "number"}}},
    {"set_camera_position", "Set Camera Position", "Camera", {{"x", "number"}, {"y", "number"}, {"z", "number"}}},
    {"restart_frame", "Restart Frame", "Game", {}},
    {"end_game", "End Game", "Game", {}},
};
