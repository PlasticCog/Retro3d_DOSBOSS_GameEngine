#pragma once
#include "core/project.h"
#include "engine/renderer.h"
#include "engine/geometry.h"
#include "engine/camera.h"
#include <SDL2/SDL.h>
#include <map>
#include <set>
#include <string>
#include <vector>

class GameRuntime {
public:
    bool init(const Project& project, Renderer& renderer);
    void shutdown();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render(int width, int height);
    bool isGameOver() const { return gameOver; }

private:
    Renderer* renderer = nullptr;
    Camera camera;

    struct GameObject {
        std::string name;
        Mesh mesh;
        glm::vec3 position{0}, rotation{0}, scale{1};
        glm::vec3 origPosition{0}, origRotation{0}, origScale{1};
        glm::vec3 color{0.5f};
        bool visible = true;
    };
    std::map<std::string, GameObject> objects;

    // Input state
    std::set<std::string> keysDown, keysPressed, keysReleased;
    bool mouseClicked = false;

    // Game state
    float gameTimer = 0;
    std::map<std::string, float> timerAccum;
    std::map<std::string, float> variables;
    std::set<std::string> destroyed;
    bool isFirstFrame = true;
    bool gameOver = false;

    // FPS camera
    float velY = 0;
    bool grounded = true;
    bool mouseLook = false;
    int lastMX = 0, lastMY = 0;

    // Events
    std::vector<GameEvent> events;

    GameObject* findObject(const std::string& name);
    bool evalCondition(const EventCondition& cond);
    void execAction(const EventAction& action, float dt);
    bool compare(float a, const std::string& op, float b);
};
