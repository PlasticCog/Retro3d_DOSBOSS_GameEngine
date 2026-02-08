#pragma once
#include "core/project.h"
#include "engine/renderer.h"
#include "engine/camera.h"
#include "engine/gizmo.h"
#include "engine/geometry.h"
#include <string>
#include <map>

struct SDL_Window;

class Editor {
public:
    bool init(SDL_Window* window);
    void shutdown();
    void update(float dt);
    void render(int windowW, int windowH);

    Project project;
    std::string selectedId;
    TransformMode transformMode = TransformMode::Move;
    EditorTab currentTab = EditorTab::Model;
    bool isRunning = false;  // game runtime active

    Camera camera;
    Renderer renderer;
    Gizmo gizmo;

    // Mesh cache
    std::map<std::string, Mesh> meshCache;
    Mesh& getOrCreateMesh(const SceneObject& obj);
    void clearMeshCache();

    // Actions
    void addObject(PrimType type);
    void deleteObject(const std::string& id);
    void duplicateObject();
    SceneObject* getSelected();

    // Status message
    std::string statusMessage;
    float statusTimer = 0;
    void showStatus(const std::string& msg, float duration = 2.5f);

private:
    // Tab drawing helpers
    void drawModelTab(float dt);
    void drawEventsTab();
    void drawAnimationTab(float dt);
    void renderScene(int viewW, int viewH);
};
