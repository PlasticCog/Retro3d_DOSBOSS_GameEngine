#pragma once
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Unique ID generator
std::string generateUID();

// Primitive types
enum class PrimType {
    Box, Cylinder, Cone, Sphere, Wedge, Plane
};

const char* primTypeName(PrimType t);
PrimType primTypeFromName(const std::string& name);

// Scene object
struct SceneObject {
    std::string id;
    std::string name;
    PrimType type = PrimType::Box;
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};  // degrees
    glm::vec3 scale{1.0f};
    glm::vec3 color{0.5f, 0.5f, 0.5f};
    bool visible = true;
    std::vector<float> customVerts;  // empty = use default geometry

    static SceneObject create(PrimType type, const std::string& name = "");
};

// Condition parameter
struct ConditionParam {
    std::string name;
    std::string type;  // "key", "number", "string", "object", "comparison", "axis"
};

// Condition definition
struct ConditionDef {
    std::string id;
    std::string label;
    std::string category;
    std::vector<ConditionParam> params;
};

// Action parameter
struct ActionParam {
    std::string name;
    std::string type;
};

// Action definition
struct ActionDef {
    std::string id;
    std::string label;
    std::string category;
    std::vector<ActionParam> params;
};

// Event condition instance
struct EventCondition {
    std::string id;
    std::string type;
    std::string label;
    std::map<std::string, std::string> params;
    bool negate = false;
};

// Event action instance
struct EventAction {
    std::string id;
    std::string type;
    std::string label;
    std::map<std::string, std::string> params;
};

// Game event (condition -> action mapping)
struct GameEvent {
    std::string id;
    bool enabled = true;
    std::vector<EventCondition> conditions;
    std::vector<EventAction> actions;
};

// Keyframe
struct Keyframe {
    std::string id;
    std::string objectId;
    int frame = 0;
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

// Transform mode
enum class TransformMode {
    Move, Rotate, Scale, Vertex
};

// Editor tab
enum class EditorTab {
    Model, Events, Animation
};

// Global condition/action definitions
extern const std::vector<ConditionDef> CONDITIONS;
extern const std::vector<ActionDef> ACTIONS;
