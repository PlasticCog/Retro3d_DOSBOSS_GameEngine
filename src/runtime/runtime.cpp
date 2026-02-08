#include "runtime/runtime.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cctype>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static const float PLAYER_HEIGHT    = 1.6f;
static const float MOVE_SPEED       = 5.0f;
static const float LOOK_SENSITIVITY = 0.004f;   // radians per pixel
static const float GRAVITY_ACCEL    = -15.0f;
static const float JUMP_VELOCITY    = 6.0f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string toUpper(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = (char)toupper((unsigned char)c);
    return r;
}

static float safeFloat(const std::map<std::string, std::string>& params,
                       const std::string& key, float def = 0.0f) {
    auto it = params.find(key);
    if (it != params.end()) {
        try { return std::stof(it->second); }
        catch (...) { return def; }
    }
    return def;
}

static std::string safeStr(const std::map<std::string, std::string>& params,
                           const std::string& key,
                           const std::string& def = "") {
    auto it = params.find(key);
    return (it != params.end()) ? it->second : def;
}

// ---------------------------------------------------------------------------
// SDL keycode -> readable key name used by the event system
// ---------------------------------------------------------------------------
static std::string keyName(SDL_Keycode k) {
    // Letters A-Z
    if (k >= SDLK_a && k <= SDLK_z) {
        char c = (char)('A' + (k - SDLK_a));
        return std::string(1, c);
    }
    // Digits 0-9
    if (k >= SDLK_0 && k <= SDLK_9) {
        char c = (char)('0' + (k - SDLK_0));
        return std::string(1, c);
    }
    switch (k) {
        case SDLK_UP:        return "Up";
        case SDLK_DOWN:      return "Down";
        case SDLK_LEFT:      return "Left";
        case SDLK_RIGHT:     return "Right";
        case SDLK_SPACE:     return "Space";
        case SDLK_RETURN:    return "Return";
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:    return "Shift";
        case SDLK_LCTRL:
        case SDLK_RCTRL:     return "Ctrl";
        case SDLK_LALT:
        case SDLK_RALT:      return "Alt";
        case SDLK_TAB:       return "Tab";
        case SDLK_ESCAPE:    return "Escape";
        case SDLK_BACKSPACE: return "Backspace";
        case SDLK_DELETE:    return "Delete";
        case SDLK_F1:  return "F1";
        case SDLK_F2:  return "F2";
        case SDLK_F3:  return "F3";
        case SDLK_F4:  return "F4";
        case SDLK_F5:  return "F5";
        case SDLK_F6:  return "F6";
        case SDLK_F7:  return "F7";
        case SDLK_F8:  return "F8";
        case SDLK_F9:  return "F9";
        case SDLK_F10: return "F10";
        case SDLK_F11: return "F11";
        case SDLK_F12: return "F12";
        default: {
            const char* n = SDL_GetKeyName(k);
            return n ? toUpper(std::string(n)) : "UNKNOWN";
        }
    }
}

// ===========================================================================
// init() -- build game objects from project data, reset all state
// ===========================================================================
bool GameRuntime::init(const Project& project, Renderer& rend) {
    renderer = &rend;
    events   = project.events;

    // Reset FPS camera
    camera.fpsPosition = glm::vec3(0.0f, PLAYER_HEIGHT, 5.0f);
    camera.yaw   = 0.0f;
    camera.pitch  = 0.0f;

    // Create a GPU mesh for every scene object
    objects.clear();
    for (const auto& obj : project.objects) {
        GameObject go;
        go.name         = obj.name;
        go.mesh         = generatePrimitive(obj.type);
        if (!obj.customVerts.empty()) {
            applyCustomVerts(go.mesh, obj.customVerts);
        }
        go.position     = obj.position;
        go.rotation     = obj.rotation;
        go.scale        = obj.scale;
        go.origPosition = obj.position;
        go.origRotation = obj.rotation;
        go.origScale    = obj.scale;
        go.color        = obj.color;
        go.visible      = obj.visible;
        objects[obj.name] = go;
    }

    // Reset all runtime state
    keysDown.clear();
    keysPressed.clear();
    keysReleased.clear();
    mouseClicked = false;

    gameTimer    = 0.0f;
    timerAccum.clear();
    variables.clear();
    destroyed.clear();
    isFirstFrame = true;
    gameOver     = false;

    velY      = 0.0f;
    grounded  = true;
    mouseLook = false;
    lastMX    = 0;
    lastMY    = 0;

    // Pre-initialise timer accumulators for all timer_every conditions
    for (const auto& evt : events) {
        for (const auto& cond : evt.conditions) {
            if (cond.type == "timer_every") {
                timerAccum[cond.id] = 0.0f;
            }
        }
    }

    return true;
}

// ===========================================================================
// shutdown() -- release GPU meshes and reset state
// ===========================================================================
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

    // Release mouse capture if still held
    if (mouseLook) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouseLook = false;
    }
}

// ===========================================================================
// handleEvent() -- translate SDL input into runtime input state
// ===========================================================================
void GameRuntime::handleEvent(const SDL_Event& event) {
    switch (event.type) {

    case SDL_KEYDOWN:
        if (!event.key.repeat) {
            std::string name = keyName(event.key.keysym.sym);
            keysDown.insert(name);
            keysPressed.insert(name);
        }
        break;

    case SDL_KEYUP: {
        std::string name = keyName(event.key.keysym.sym);
        keysDown.erase(name);
        keysReleased.insert(name);
        break;
    }

    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            mouseClicked = true;
        }
        if (event.button.button == SDL_BUTTON_RIGHT) {
            mouseLook = true;
            lastMX = event.button.x;
            lastMY = event.button.y;
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_RIGHT) {
            mouseLook = false;
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        break;

    case SDL_MOUSEMOTION:
        if (mouseLook) {
            // Camera yaw/pitch are in radians
            camera.yaw   += event.motion.xrel * LOOK_SENSITIVITY;
            camera.pitch -= event.motion.yrel * LOOK_SENSITIVITY;

            // Clamp pitch to prevent gimbal lock
            float maxPitch = (float)(M_PI * 0.5 - 0.01);
            camera.pitch = glm::clamp(camera.pitch, -maxPitch, maxPitch);
        }
        break;

    default:
        break;
    }
}

// ===========================================================================
// findObject() -- lookup by name, with case-insensitive fallback
// ===========================================================================
GameRuntime::GameObject* GameRuntime::findObject(const std::string& name) {
    if (name.empty()) return nullptr;

    // Check destroyed set first
    if (destroyed.count(name) > 0) return nullptr;

    // Exact match (fast path)
    auto it = objects.find(name);
    if (it != objects.end()) return &it->second;

    // Case-insensitive fallback
    std::string upper = toUpper(name);
    for (auto& [k, v] : objects) {
        if (destroyed.count(k) > 0) continue;
        if (toUpper(k) == upper) return &v;
    }

    return nullptr;
}

// ===========================================================================
// compare() -- generic numeric comparison with epsilon for equality
// ===========================================================================
bool GameRuntime::compare(float a, const std::string& op, float b) {
    if (op == "==" || op == "=" || op == "Equal" || op == "equal" || op == "equals")
        return std::abs(a - b) < 0.001f;
    if (op == "!=" || op == "Not Equal" || op == "not_equals")
        return std::abs(a - b) >= 0.001f;
    if (op == "<"  || op == "Less Than"      || op == "less")       return a < b;
    if (op == ">"  || op == "Greater Than"   || op == "greater")    return a > b;
    if (op == "<=" || op == "Less or Equal"  || op == "less_equal") return a <= b;
    if (op == ">=" || op == "Greater or Equal" || op == "greater_equal") return a >= b;
    // Default: equality
    return std::abs(a - b) < 0.001f;
}

// ===========================================================================
// evalCondition() -- evaluate a single EventCondition, return true/false
// ===========================================================================
bool GameRuntime::evalCondition(const EventCondition& cond) {
    bool result = false;

    // ----- Special conditions -----
    if (cond.type == "always") {
        result = true;
    }
    else if (cond.type == "start_of_frame") {
        result = isFirstFrame;
    }
    // ----- Input conditions -----
    else if (cond.type == "key_pressed") {
        std::string key = toUpper(safeStr(cond.params, "key"));
        result = keysPressed.count(key) > 0;
    }
    else if (cond.type == "key_down") {
        std::string key = toUpper(safeStr(cond.params, "key"));
        result = keysDown.count(key) > 0;
    }
    else if (cond.type == "key_released") {
        std::string key = toUpper(safeStr(cond.params, "key"));
        result = keysReleased.count(key) > 0;
    }
    else if (cond.type == "mouse_clicked") {
        result = mouseClicked;
    }
    // ----- Timer conditions -----
    else if (cond.type == "timer_equals") {
        float seconds = safeFloat(cond.params, "seconds");
        result = (std::abs(gameTimer - seconds) < 0.05f);
    }
    else if (cond.type == "timer_every") {
        float interval = safeFloat(cond.params, "seconds", 1.0f);
        if (interval > 0.0f) {
            // Use condition's unique ID as accumulator key
            float& accum = timerAccum[cond.id];
            if (accum >= interval) {
                accum -= interval;
                result = true;
            }
        }
    }
    // ----- Variable conditions -----
    else if (cond.type == "compare_var") {
        std::string varName = safeStr(cond.params, "variable");
        std::string op      = safeStr(cond.params, "comparison", "==");
        float value         = safeFloat(cond.params, "value");
        float varVal        = variables.count(varName) ? variables[varName] : 0.0f;
        result = compare(varVal, op, value);
    }
    // ----- Object visibility conditions -----
    else if (cond.type == "object_visible") {
        std::string objName = safeStr(cond.params, "object");
        auto* go = findObject(objName);
        result = (go != nullptr && go->visible);
    }
    else if (cond.type == "object_invisible") {
        std::string objName = safeStr(cond.params, "object");
        auto* go = findObject(objName);
        result = (go == nullptr || !go->visible);
    }
    // ----- Position conditions -----
    else if (cond.type == "position_compare") {
        std::string objName = safeStr(cond.params, "object");
        std::string axis    = safeStr(cond.params, "axis", "y");
        std::string op      = safeStr(cond.params, "comparison", "==");
        float value         = safeFloat(cond.params, "value");
        auto* go = findObject(objName);
        if (go) {
            float posVal = 0.0f;
            if (axis == "x" || axis == "X") posVal = go->position.x;
            else if (axis == "y" || axis == "Y") posVal = go->position.y;
            else if (axis == "z" || axis == "Z") posVal = go->position.z;
            result = compare(posVal, op, value);
        }
    }

    // Apply negation flag
    return cond.negate ? !result : result;
}

// ===========================================================================
// execAction() -- execute a single EventAction
// ===========================================================================
void GameRuntime::execAction(const EventAction& action, float dt) {

    // ----- Movement actions -----
    if (action.type == "move_object") {
        std::string name = safeStr(action.params, "object");
        auto* go = findObject(name);
        if (go && destroyed.count(go->name) == 0) {
            float x = safeFloat(action.params, "x");
            float y = safeFloat(action.params, "y");
            float z = safeFloat(action.params, "z");
            // Values are per-frame at 60fps; scale by dt for frame-rate independence
            go->position += glm::vec3(x, y, z) * dt * 60.0f;
        }
    }
    else if (action.type == "set_position") {
        std::string name = safeStr(action.params, "object");
        auto* go = findObject(name);
        if (go) {
            go->position.x = safeFloat(action.params, "x");
            go->position.y = safeFloat(action.params, "y");
            go->position.z = safeFloat(action.params, "z");
        }
    }
    else if (action.type == "rotate_object") {
        std::string name = safeStr(action.params, "object");
        auto* go = findObject(name);
        if (go && destroyed.count(go->name) == 0) {
            float x = safeFloat(action.params, "x");
            float y = safeFloat(action.params, "y");
            float z = safeFloat(action.params, "z");
            go->rotation += glm::vec3(x, y, z) * dt * 60.0f;
        }
    }
    else if (action.type == "set_rotation") {
        std::string name = safeStr(action.params, "object");
        auto* go = findObject(name);
        if (go) {
            go->rotation.x = safeFloat(action.params, "x");
            go->rotation.y = safeFloat(action.params, "y");
            go->rotation.z = safeFloat(action.params, "z");
        }
    }
    // ----- Object lifecycle actions -----
    else if (action.type == "destroy_object") {
        std::string name = safeStr(action.params, "object");
        auto* go = findObject(name);
        if (go) {
            go->visible = false;
            destroyed.insert(go->name);
        }
    }
    else if (action.type == "show_object") {
        std::string name = safeStr(action.params, "object");
        // For show, we need to find even if destroyed, so search directly
        auto it = objects.find(name);
        if (it == objects.end()) {
            // Case-insensitive fallback
            std::string upper = toUpper(name);
            for (auto& [k, v] : objects) {
                if (toUpper(k) == upper) { it = objects.find(k); break; }
            }
        }
        if (it != objects.end()) {
            it->second.visible = true;
            destroyed.erase(it->first);
        }
    }
    else if (action.type == "hide_object") {
        std::string name = safeStr(action.params, "object");
        auto* go = findObject(name);
        if (go) {
            go->visible = false;
        }
    }
    // ----- Variable actions -----
    else if (action.type == "set_variable") {
        std::string var = safeStr(action.params, "variable");
        float val       = safeFloat(action.params, "value");
        if (!var.empty()) {
            variables[var] = val;
        }
    }
    else if (action.type == "add_to_variable") {
        std::string var = safeStr(action.params, "variable");
        float val       = safeFloat(action.params, "value");
        if (!var.empty()) {
            variables[var] += val;
        }
    }
    // ----- Sound actions (stubs -- needs audio backend) -----
    else if (action.type == "play_sound") {
        // Would play the named sound asset
        // std::string sound = safeStr(action.params, "sound");
    }
    else if (action.type == "stop_sound") {
        // Would stop all currently playing sounds
    }
    // ----- Camera actions -----
    else if (action.type == "move_camera") {
        float x = safeFloat(action.params, "x");
        float y = safeFloat(action.params, "y");
        float z = safeFloat(action.params, "z");
        camera.fpsPosition += glm::vec3(x, y, z) * dt * 60.0f;
    }
    else if (action.type == "set_camera_position") {
        camera.fpsPosition.x = safeFloat(action.params, "x");
        camera.fpsPosition.y = safeFloat(action.params, "y");
        camera.fpsPosition.z = safeFloat(action.params, "z");
    }
    // ----- Game flow actions -----
    else if (action.type == "restart_frame") {
        // Reset all objects to their original transforms
        for (auto& [name, go] : objects) {
            go.position = go.origPosition;
            go.rotation = go.origRotation;
            go.scale    = go.origScale;
            go.visible  = true;
        }
        destroyed.clear();
        variables.clear();
        gameTimer = 0.0f;
        for (auto& [id, acc] : timerAccum) {
            acc = 0.0f;
        }
        isFirstFrame = true;

        // Reset camera
        camera.fpsPosition = glm::vec3(0.0f, PLAYER_HEIGHT, 5.0f);
        camera.yaw   = 0.0f;
        camera.pitch  = 0.0f;
        velY     = 0.0f;
        grounded = true;
    }
    else if (action.type == "end_game") {
        gameOver = true;
    }
}

// ===========================================================================
// update() -- per-frame tick: FPS movement, gravity, event evaluation
// ===========================================================================
void GameRuntime::update(float dt) {
    if (gameOver) return;

    // Clamp delta time to prevent physics explosion after long pauses
    if (dt > 0.1f) dt = 0.1f;

    // -----------------------------------------------------------------
    // FPS camera movement (WASD + right-click drag look + jump + gravity)
    // -----------------------------------------------------------------
    // Flat forward and right vectors derived from camera yaw (radians)
    // Camera convention: yaw=0 looks along +Z, yaw increases turning right
    glm::vec3 flatForward(sinf(camera.yaw), 0.0f, cosf(camera.yaw));
    glm::vec3 flatRight(cosf(camera.yaw), 0.0f, -sinf(camera.yaw));

    glm::vec3 moveDir(0.0f);
    if (keysDown.count("W") || keysDown.count("Up"))    moveDir += flatForward;
    if (keysDown.count("S") || keysDown.count("Down"))  moveDir -= flatForward;
    if (keysDown.count("D") || keysDown.count("Right")) moveDir += flatRight;
    if (keysDown.count("A") || keysDown.count("Left"))  moveDir -= flatRight;

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
        camera.fpsPosition += moveDir * MOVE_SPEED * dt;
    }

    // Jump
    if ((keysDown.count("Space")) && grounded) {
        velY = JUMP_VELOCITY;
        grounded = false;
    }

    // Gravity
    velY += GRAVITY_ACCEL * dt;
    camera.fpsPosition.y += velY * dt;

    // Ground collision (floor at y=0, player eyes at PLAYER_HEIGHT)
    if (camera.fpsPosition.y <= PLAYER_HEIGHT) {
        camera.fpsPosition.y = PLAYER_HEIGHT;
        velY     = 0.0f;
        grounded = true;
    }

    // -----------------------------------------------------------------
    // Update timers
    // -----------------------------------------------------------------
    gameTimer += dt;
    for (auto& [id, accum] : timerAccum) {
        accum += dt;
    }

    // -----------------------------------------------------------------
    // Evaluate all events: conditions use AND logic
    // -----------------------------------------------------------------
    for (auto& evt : events) {
        if (!evt.enabled || evt.conditions.empty()) continue;

        bool allTrue = true;
        for (const auto& cond : evt.conditions) {
            if (!evalCondition(cond)) {
                allTrue = false;
                break;
            }
        }

        if (allTrue) {
            for (const auto& act : evt.actions) {
                execAction(act, dt);
                if (gameOver) return;
            }
        }
    }

    // -----------------------------------------------------------------
    // Clear per-frame input flags
    // -----------------------------------------------------------------
    keysPressed.clear();
    keysReleased.clear();
    mouseClicked = false;
    isFirstFrame = false;
}

// ===========================================================================
// render() -- draw the game world from the FPS camera viewpoint
// ===========================================================================
void GameRuntime::render(int width, int height) {
    if (!renderer || width <= 0 || height <= 0) return;

    glViewport(0, 0, width, height);
    glClearColor(0.04f, 0.04f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    float aspect = (float)width / std::max((float)height, 1.0f);
    glm::mat4 view = camera.getFPSViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 vp   = proj * view;

    // Draw ground grid
    renderer->drawGrid(vp, 40, 1.0f);

    // Set up the lit scene shader
    glUseProgram(renderer->sceneProgram);
    glUniform3f(renderer->sceneLightDir_loc, 0.4f, 0.8f, 0.3f);
    glUniform3f(renderer->sceneFogColor_loc, 0.04f, 0.04f, 0.10f);
    glUniform1f(renderer->sceneFogDensity_loc, 0.015f);

    // Draw every visible, non-destroyed game object
    for (auto& [name, go] : objects) {
        if (!go.visible || destroyed.count(name) > 0) continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, go.position);
        model = glm::rotate(model, glm::radians(go.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(go.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(go.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, go.scale);

        glm::mat4 mvp = vp * model;

        glUniformMatrix4fv(renderer->sceneMVP_loc,   1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(renderer->sceneModel_loc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(renderer->sceneColor_loc, 1, glm::value_ptr(go.color));
        glUniform3f(renderer->sceneEmissive_loc, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(go.mesh.vao);
        if (go.mesh.indexCount > 0) {
            glDrawElements(GL_TRIANGLES, go.mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, go.mesh.vertexCount);
        }
        glBindVertexArray(0);
    }
}
