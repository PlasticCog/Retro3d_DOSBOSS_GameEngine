#include "runtime/runtime.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>

static const float PLAYER_HEIGHT = 1.6f;
static const float MOVE_SPEED = 5.0f;
static const float LOOK_SENS = 0.004f;
static const float GRAVITY = -15.0f;
static const float JUMP_VEL = 6.0f;

static std::string toUpper(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = (char)toupper((unsigned char)c);
    return r;
}

static float parseFloat(const std::string& s) {
    try { return std::stof(s); } catch (...) { return 0.0f; }
}

bool GameRuntime::init(const Project& project, Renderer& rend) {
    renderer = &rend;
    events = project.events;

    camera.yaw = 0;
    camera.pitch = 0;
    camera.fpsPosition = glm::vec3(0, PLAYER_HEIGHT, 5);

    for (auto& obj : project.objects) {
        GameObject go;
        go.name = obj.name;
        go.mesh = generatePrimitive(obj.type);
        go.position = obj.position;
        go.rotation = obj.rotation;
        go.scale = obj.scale;
        go.origPosition = obj.position;
        go.origRotation = obj.rotation;
        go.origScale = obj.scale;
        go.color = obj.color;
        go.visible = obj.visible;
        objects[obj.name] = go;
    }

    gameTimer = 0;
    isFirstFrame = true;
    gameOver = false;
    velY = 0;
    grounded = true;
    return true;
}

void GameRuntime::shutdown() {
    for (auto& [name, go] : objects) {
        go.mesh.destroy();
    }
    objects.clear();
    keysDown.clear();
    keysPressed.clear();
    keysReleased.clear();
    variables.clear();
    destroyed.clear();
    timerAccum.clear();
}

void GameRuntime::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        std::string key = toUpper(SDL_GetKeyName(event.key.keysym.sym));
        keysDown.insert(key);
        keysPressed.insert(key);
    }
    if (event.type == SDL_KEYUP) {
        std::string key = toUpper(SDL_GetKeyName(event.key.keysym.sym));
        keysDown.erase(key);
        keysReleased.insert(key);
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        mouseClicked = true;
        mouseLook = true;
        lastMX = event.button.x;
        lastMY = event.button.y;
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        mouseLook = false;
    }
    if (event.type == SDL_MOUSEMOTION && mouseLook) {
        float dx = (float)(event.motion.x - lastMX);
        float dy = (float)(event.motion.y - lastMY);
        camera.yaw -= dx * LOOK_SENS;
        camera.pitch = glm::clamp(camera.pitch - dy * LOOK_SENS, -1.4f, 1.4f);
        lastMX = event.motion.x;
        lastMY = event.motion.y;
    }
}

void GameRuntime::update(float dt) {
    if (gameOver) return;
    dt = std::min(dt, 0.05f);
    gameTimer += dt;

    // Update timer accumulators
    for (auto& [key, val] : timerAccum) {
        val += dt;
    }

    // FPS movement
    glm::vec3 fwd(-sinf(camera.yaw), 0, -cosf(camera.yaw));
    glm::vec3 right(cosf(camera.yaw), 0, -sinf(camera.yaw));
    glm::vec3 moveDir(0);

    if (keysDown.count("W") || keysDown.count("UP")) moveDir += fwd;
    if (keysDown.count("S") || keysDown.count("DOWN")) moveDir -= fwd;
    if (keysDown.count("A") || keysDown.count("LEFT")) moveDir -= right;
    if (keysDown.count("D") || keysDown.count("RIGHT")) moveDir += right;

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir) * MOVE_SPEED * dt;
    }

    velY += GRAVITY * dt;
    if ((keysDown.count("SPACE") || keysDown.count(" ")) && grounded) {
        velY = JUMP_VEL;
        grounded = false;
    }

    camera.fpsPosition.x += moveDir.x;
    camera.fpsPosition.z += moveDir.z;
    camera.fpsPosition.y += velY * dt;

    if (camera.fpsPosition.y <= PLAYER_HEIGHT) {
        camera.fpsPosition.y = PLAYER_HEIGHT;
        velY = 0;
        grounded = true;
    }

    // Evaluate events
    for (auto& evt : events) {
        if (!evt.enabled || evt.conditions.empty()) continue;
        bool allTrue = true;
        for (auto& cond : evt.conditions) {
            if (!evalCondition(cond)) { allTrue = false; break; }
        }
        if (allTrue) {
            for (auto& act : evt.actions) {
                execAction(act, dt);
            }
        }
    }

    // Clear per-frame state
    keysPressed.clear();
    keysReleased.clear();
    mouseClicked = false;
    isFirstFrame = false;
}

void GameRuntime::render(int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.04f, 0.04f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float aspect = (float)width / std::max((float)height, 1.0f);
    glm::mat4 proj = glm::perspective(glm::radians(75.0f), aspect, 0.1f, 500.0f);
    glm::mat4 view = camera.getFPSViewMatrix();
    glm::mat4 vp = proj * view;

    glUseProgram(renderer->sceneProgram);
    glUniform3f(renderer->sceneLightDir_loc, 0.5f, 0.8f, 0.3f);
    glUniform3f(renderer->sceneFogColor_loc, 0.04f, 0.04f, 0.1f);
    glUniform1f(renderer->sceneFogDensity_loc, 0.015f);

    for (auto& [name, go] : objects) {
        if (!go.visible) continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, go.position);
        model = glm::rotate(model, glm::radians(go.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(go.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(go.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, go.scale);

        glm::mat4 mvp = vp * model;
        glUniformMatrix4fv(renderer->sceneMVP_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(renderer->sceneModel_loc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(renderer->sceneColor_loc, 1, glm::value_ptr(go.color));
        glUniform3f(renderer->sceneEmissive_loc, 0, 0, 0);

        glBindVertexArray(go.mesh.vao);
        if (go.mesh.indexCount > 0)
            glDrawElements(GL_TRIANGLES, go.mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(GL_TRIANGLES, 0, go.mesh.vertexCount);
        glBindVertexArray(0);
    }
}

GameRuntime::GameObject* GameRuntime::findObject(const std::string& name) {
    // Exact match
    auto it = objects.find(name);
    if (it != objects.end()) return &it->second;

    // Case-insensitive match
    std::string lower = toUpper(name);
    for (auto& [k, v] : objects) {
        if (toUpper(k) == lower) return &v;
    }
    // Partial match
    for (auto& [k, v] : objects) {
        if (toUpper(k).find(lower) != std::string::npos) return &v;
    }
    return nullptr;
}

bool GameRuntime::compare(float a, const std::string& op, float b) {
    if (op == "==" || op == "Equal" || op == "equal") return a == b;
    if (op == "!=" || op == "Not Equal") return a != b;
    if (op == "<" || op == "Less Than") return a < b;
    if (op == "<=" || op == "Less or Equal") return a <= b;
    if (op == ">" || op == "Greater Than") return a > b;
    if (op == ">=" || op == "Greater or Equal") return a >= b;
    return a == b;
}

bool GameRuntime::evalCondition(const EventCondition& cond) {
    bool result = false;

    if (cond.type == "always") {
        result = true;
    }
    else if (cond.type == "start_of_frame") {
        result = isFirstFrame;
    }
    else if (cond.type == "key_pressed") {
        auto it = cond.params.find("key");
        if (it != cond.params.end()) {
            result = keysPressed.count(toUpper(it->second)) > 0;
        }
    }
    else if (cond.type == "key_down") {
        auto it = cond.params.find("key");
        if (it != cond.params.end()) {
            result = keysDown.count(toUpper(it->second)) > 0;
        }
    }
    else if (cond.type == "key_released") {
        auto it = cond.params.find("key");
        if (it != cond.params.end()) {
            result = keysReleased.count(toUpper(it->second)) > 0;
        }
    }
    else if (cond.type == "mouse_clicked") {
        result = mouseClicked;
    }
    else if (cond.type == "timer_equals") {
        auto it = cond.params.find("seconds");
        if (it != cond.params.end()) {
            float target = parseFloat(it->second);
            result = fabsf(gameTimer - target) < 0.05f;
        }
    }
    else if (cond.type == "timer_every") {
        auto it = cond.params.find("seconds");
        if (it != cond.params.end()) {
            float interval = parseFloat(it->second);
            if (interval > 0) {
                std::string tk = "ev_" + it->second;
                if (timerAccum.find(tk) == timerAccum.end()) timerAccum[tk] = 0;
                if (timerAccum[tk] >= interval) {
                    timerAccum[tk] -= interval;
                    result = true;
                }
            }
        }
    }
    else if (cond.type == "compare_var") {
        auto vit = cond.params.find("variable");
        auto cit = cond.params.find("comparison");
        auto vlit = cond.params.find("value");
        if (vit != cond.params.end() && cit != cond.params.end() && vlit != cond.params.end()) {
            float varVal = 0;
            auto vf = variables.find(vit->second);
            if (vf != variables.end()) varVal = vf->second;
            result = compare(varVal, cit->second, parseFloat(vlit->second));
        }
    }
    else if (cond.type == "object_visible") {
        auto it = cond.params.find("object");
        if (it != cond.params.end()) {
            auto* obj = findObject(it->second);
            result = obj ? obj->visible : false;
        }
    }
    else if (cond.type == "object_invisible") {
        auto it = cond.params.find("object");
        if (it != cond.params.end()) {
            auto* obj = findObject(it->second);
            result = obj ? !obj->visible : false;
        }
    }
    else if (cond.type == "position_compare") {
        auto oit = cond.params.find("object");
        auto ait = cond.params.find("axis");
        auto cit = cond.params.find("comparison");
        auto vit = cond.params.find("value");
        if (oit != cond.params.end() && ait != cond.params.end() &&
            cit != cond.params.end() && vit != cond.params.end()) {
            auto* obj = findObject(oit->second);
            if (obj) {
                float axisVal = 0;
                std::string axis = ait->second;
                if (axis == "x") axisVal = obj->position.x;
                else if (axis == "y") axisVal = obj->position.y;
                else if (axis == "z") axisVal = obj->position.z;
                result = compare(axisVal, cit->second, parseFloat(vit->second));
            }
        }
    }

    return cond.negate ? !result : result;
}

void GameRuntime::execAction(const EventAction& action, float dt) {
    if (action.type == "move_object") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj && destroyed.count(obj->name) == 0) {
                auto xit = action.params.find("x");
                auto yit = action.params.find("y");
                auto zit = action.params.find("z");
                if (xit != action.params.end()) obj->position.x += parseFloat(xit->second) * dt * 60.0f;
                if (yit != action.params.end()) obj->position.y += parseFloat(yit->second) * dt * 60.0f;
                if (zit != action.params.end()) obj->position.z += parseFloat(zit->second) * dt * 60.0f;
            }
        }
    }
    else if (action.type == "set_position") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj) {
                auto xit = action.params.find("x");
                auto yit = action.params.find("y");
                auto zit = action.params.find("z");
                obj->position.x = xit != action.params.end() ? parseFloat(xit->second) : 0;
                obj->position.y = yit != action.params.end() ? parseFloat(yit->second) : 0;
                obj->position.z = zit != action.params.end() ? parseFloat(zit->second) : 0;
            }
        }
    }
    else if (action.type == "rotate_object") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj && destroyed.count(obj->name) == 0) {
                auto xit = action.params.find("x");
                auto yit = action.params.find("y");
                auto zit = action.params.find("z");
                if (xit != action.params.end()) obj->rotation.x += parseFloat(xit->second) * dt * 60.0f;
                if (yit != action.params.end()) obj->rotation.y += parseFloat(yit->second) * dt * 60.0f;
                if (zit != action.params.end()) obj->rotation.z += parseFloat(zit->second) * dt * 60.0f;
            }
        }
    }
    else if (action.type == "set_rotation") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj) {
                auto xit = action.params.find("x");
                auto yit = action.params.find("y");
                auto zit = action.params.find("z");
                obj->rotation.x = xit != action.params.end() ? parseFloat(xit->second) : 0;
                obj->rotation.y = yit != action.params.end() ? parseFloat(yit->second) : 0;
                obj->rotation.z = zit != action.params.end() ? parseFloat(zit->second) : 0;
            }
        }
    }
    else if (action.type == "destroy_object") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj) {
                obj->visible = false;
                destroyed.insert(obj->name);
            }
        }
    }
    else if (action.type == "show_object") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj) {
                obj->visible = true;
                destroyed.erase(obj->name);
            }
        }
    }
    else if (action.type == "hide_object") {
        auto it = action.params.find("object");
        if (it != action.params.end()) {
            auto* obj = findObject(it->second);
            if (obj) obj->visible = false;
        }
    }
    else if (action.type == "set_variable") {
        auto vit = action.params.find("variable");
        auto val = action.params.find("value");
        if (vit != action.params.end() && val != action.params.end()) {
            variables[vit->second] = parseFloat(val->second);
        }
    }
    else if (action.type == "add_to_variable") {
        auto vit = action.params.find("variable");
        auto val = action.params.find("value");
        if (vit != action.params.end() && val != action.params.end()) {
            variables[vit->second] += parseFloat(val->second);
        }
    }
    else if (action.type == "move_camera") {
        auto xit = action.params.find("x");
        auto yit = action.params.find("y");
        auto zit = action.params.find("z");
        if (xit != action.params.end()) camera.fpsPosition.x += parseFloat(xit->second) * dt * 60.0f;
        if (yit != action.params.end()) camera.fpsPosition.y += parseFloat(yit->second) * dt * 60.0f;
        if (zit != action.params.end()) camera.fpsPosition.z += parseFloat(zit->second) * dt * 60.0f;
    }
    else if (action.type == "set_camera_position") {
        auto xit = action.params.find("x");
        auto yit = action.params.find("y");
        auto zit = action.params.find("z");
        camera.fpsPosition.x = xit != action.params.end() ? parseFloat(xit->second) : 0;
        camera.fpsPosition.y = yit != action.params.end() ? parseFloat(yit->second) : 0;
        camera.fpsPosition.z = zit != action.params.end() ? parseFloat(zit->second) : 0;
    }
    else if (action.type == "restart_frame") {
        for (auto& [name, go] : objects) {
            go.position = go.origPosition;
            go.rotation = go.origRotation;
            go.scale = go.origScale;
            go.visible = true;
        }
        destroyed.clear();
        variables.clear();
        gameTimer = 0;
        isFirstFrame = true;
        camera.fpsPosition = glm::vec3(0, PLAYER_HEIGHT, 5);
        camera.yaw = 0;
        camera.pitch = 0;
    }
    else if (action.type == "end_game") {
        gameOver = true;
    }
}
