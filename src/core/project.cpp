#include "core/project.h"
#include <json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// JSON helpers for glm::vec3
static json vec3ToJson(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

static glm::vec3 jsonToVec3(const json& j) {
    return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

bool Project::save(const std::string& filepath) const {
    json j;
    j["version"] = 1;
    j["name"] = name;

    j["objects"] = json::array();
    for (auto& obj : objects) {
        json jo;
        jo["id"] = obj.id;
        jo["name"] = obj.name;
        jo["type"] = primTypeName(obj.type);
        jo["position"] = vec3ToJson(obj.position);
        jo["rotation"] = vec3ToJson(obj.rotation);
        jo["scale"] = vec3ToJson(obj.scale);
        jo["color"] = vec3ToJson(obj.color);
        jo["visible"] = obj.visible;
        if (!obj.customVerts.empty()) {
            jo["customVerts"] = obj.customVerts;
        }
        j["objects"].push_back(jo);
    }

    j["events"] = json::array();
    for (auto& evt : events) {
        json je;
        je["id"] = evt.id;
        je["enabled"] = evt.enabled;
        je["conditions"] = json::array();
        for (auto& c : evt.conditions) {
            json jc;
            jc["id"] = c.id;
            jc["type"] = c.type;
            jc["label"] = c.label;
            jc["params"] = c.params;
            jc["negate"] = c.negate;
            je["conditions"].push_back(jc);
        }
        je["actions"] = json::array();
        for (auto& a : evt.actions) {
            json ja;
            ja["id"] = a.id;
            ja["type"] = a.type;
            ja["label"] = a.label;
            ja["params"] = a.params;
            je["actions"].push_back(ja);
        }
        j["events"].push_back(je);
    }

    j["keyframes"] = json::array();
    for (auto& kf : keyframes) {
        json jk;
        jk["id"] = kf.id;
        jk["objectId"] = kf.objectId;
        jk["frame"] = kf.frame;
        jk["position"] = vec3ToJson(kf.position);
        jk["rotation"] = vec3ToJson(kf.rotation);
        jk["scale"] = vec3ToJson(kf.scale);
        j["keyframes"].push_back(jk);
    }

    std::ofstream ofs(filepath);
    if (!ofs.is_open()) return false;
    ofs << j.dump(2);
    return ofs.good();
}

bool Project::load(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) return false;

    json j;
    try {
        ifs >> j;
    } catch (...) {
        return false;
    }

    if (j.contains("name")) name = j["name"].get<std::string>();

    objects.clear();
    if (j.contains("objects")) {
        for (auto& jo : j["objects"]) {
            SceneObject obj;
            obj.id = jo.value("id", generateUID());
            obj.name = jo.value("name", "Object");
            obj.type = primTypeFromName(jo.value("type", "box"));
            if (jo.contains("position")) obj.position = jsonToVec3(jo["position"]);
            if (jo.contains("rotation")) obj.rotation = jsonToVec3(jo["rotation"]);
            if (jo.contains("scale")) obj.scale = jsonToVec3(jo["scale"]);
            if (jo.contains("color")) obj.color = jsonToVec3(jo["color"]);
            obj.visible = jo.value("visible", true);
            if (jo.contains("customVerts")) {
                obj.customVerts = jo["customVerts"].get<std::vector<float>>();
            }
            objects.push_back(obj);
        }
    }

    events.clear();
    if (j.contains("events")) {
        for (auto& je : j["events"]) {
            GameEvent evt;
            evt.id = je.value("id", generateUID());
            evt.enabled = je.value("enabled", true);
            if (je.contains("conditions")) {
                for (auto& jc : je["conditions"]) {
                    EventCondition c;
                    c.id = jc.value("id", generateUID());
                    c.type = jc.value("type", "always");
                    c.label = jc.value("label", "Always");
                    if (jc.contains("params")) {
                        for (auto& [key, val] : jc["params"].items()) {
                            c.params[key] = val.is_string() ? val.get<std::string>() : std::to_string(val.get<float>());
                        }
                    }
                    c.negate = jc.value("negate", false);
                    evt.conditions.push_back(c);
                }
            }
            if (je.contains("actions")) {
                for (auto& ja : je["actions"]) {
                    EventAction a;
                    a.id = ja.value("id", generateUID());
                    a.type = ja.value("type", "");
                    a.label = ja.value("label", "");
                    if (ja.contains("params")) {
                        for (auto& [key, val] : ja["params"].items()) {
                            a.params[key] = val.is_string() ? val.get<std::string>() : std::to_string(val.get<float>());
                        }
                    }
                    evt.actions.push_back(a);
                }
            }
            events.push_back(evt);
        }
    }

    keyframes.clear();
    if (j.contains("keyframes")) {
        for (auto& jk : j["keyframes"]) {
            Keyframe kf;
            kf.id = jk.value("id", generateUID());
            kf.objectId = jk.value("objectId", "");
            kf.frame = jk.value("frame", 0);
            if (jk.contains("position")) kf.position = jsonToVec3(jk["position"]);
            if (jk.contains("rotation")) kf.rotation = jsonToVec3(jk["rotation"]);
            if (jk.contains("scale")) kf.scale = jsonToVec3(jk["scale"]);
            keyframes.push_back(kf);
        }
    }

    return true;
}

void Project::clear() {
    objects.clear();
    events.clear();
    keyframes.clear();
    name = "New Project";
}

void Project::createDefaultScene() {
    clear();
    name = "My Retro Game";

    auto ground = SceneObject::create(PrimType::Box, "Ground");
    ground.position = {0, -0.5f, 0};
    ground.scale = {10, 0.2f, 10};
    ground.color = {0.176f, 0.314f, 0.086f};
    objects.push_back(ground);

    auto wall = SceneObject::create(PrimType::Box, "Wall_1");
    wall.position = {-4, 1, 0};
    wall.scale = {0.3f, 2, 6};
    wall.color = {0.545f, 0.451f, 0.333f};
    objects.push_back(wall);

    auto building = SceneObject::create(PrimType::Box, "Building");
    building.position = {2, 1.5f, -2};
    building.scale = {3, 3, 3};
    building.color = {0.333f, 0.4f, 0.467f};
    objects.push_back(building);

    auto tower = SceneObject::create(PrimType::Cylinder, "Tower");
    tower.position = {-2, 2, 3};
    tower.scale = {1, 4, 1};
    tower.color = {0.6f, 0.4f, 0.267f};
    objects.push_back(tower);

    auto roof = SceneObject::create(PrimType::Cone, "Roof");
    roof.position = {-2, 4.5f, 3};
    roof.scale = {1.5f, 1.5f, 1.5f};
    roof.color = {0.8f, 0.2f, 0.2f};
    objects.push_back(roof);

    auto sphere = SceneObject::create(PrimType::Sphere, "Sphere_1");
    sphere.position = {4, 0.5f, 3};
    sphere.scale = {1, 1, 1};
    sphere.color = {0.2f, 0.533f, 0.8f};
    objects.push_back(sphere);

    // Default events
    GameEvent evt1;
    evt1.id = generateUID();
    evt1.enabled = true;
    evt1.conditions.push_back({generateUID(), "always", "Always", {}, false});
    evt1.actions.push_back({generateUID(), "rotate_object", "Rotate Object", {{"object", "Sphere_1"}, {"x", "0"}, {"y", "1"}, {"z", "0"}}});
    events.push_back(evt1);

    GameEvent evt2;
    evt2.id = generateUID();
    evt2.enabled = true;
    evt2.conditions.push_back({generateUID(), "key_down", "Key Down", {{"key", "E"}}, false});
    evt2.actions.push_back({generateUID(), "move_object", "Move Object", {{"object", "Building"}, {"x", "0"}, {"y", "0.05"}, {"z", "0"}}});
    events.push_back(evt2);

    GameEvent evt3;
    evt3.id = generateUID();
    evt3.enabled = true;
    evt3.conditions.push_back({generateUID(), "key_pressed", "Key Pressed", {{"key", "R"}}, false});
    evt3.actions.push_back({generateUID(), "restart_frame", "Restart Frame", {}});
    events.push_back(evt3);
}
