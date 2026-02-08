#include "editor/editor.h"
#include "editor/viewport.h"
#include "editor/properties.h"
#include "editor/event_editor.h"
#include "editor/timeline.h"
#include "engine/geometry.h"
#include <imgui.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdio>

static Viewport s_viewport;

static glm::mat4 buildModelMatrix(const SceneObject& obj) {
    glm::mat4 m(1.0f);
    m = glm::translate(m, obj.position);
    m = glm::rotate(m, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, obj.scale);
    return m;
}

bool Editor::init(SDL_Window* /*window*/) {
    if (!renderer.init()) return false;
    camera.angleX = -0.5f;
    camera.angleY = 0.6f;
    camera.distance = 12.0f;
    camera.target = glm::vec3(0, 0, 0);
    gizmo.init();
    s_viewport.init(1280, 720);
    project.createDefaultScene();
    if (!project.objects.empty()) selectedId = project.objects[0].id;
    showStatus("Welcome to DOSBOSS 3D Editor");
    return true;
}

void Editor::shutdown() {
    clearMeshCache();
    s_viewport.shutdown();
    gizmo.shutdown();
    renderer.shutdown();
}

Mesh& Editor::getOrCreateMesh(const SceneObject& obj) {
    auto it = meshCache.find(obj.id);
    if (it != meshCache.end()) return it->second;
    Mesh mesh = generatePrimitive(obj.type);
    if (!obj.customVerts.empty()) applyCustomVerts(mesh, obj.customVerts);
    mesh.upload();
    meshCache[obj.id] = std::move(mesh);
    return meshCache[obj.id];
}

void Editor::clearMeshCache() {
    for (auto& [id, mesh] : meshCache) mesh.destroy();
    meshCache.clear();
}

void Editor::addObject(PrimType type) {
    SceneObject obj = SceneObject::create(type);
    obj.position = glm::vec3(0, 0.5f, 0);
    selectedId = obj.id;
    project.objects.push_back(obj);
    showStatus("Added " + std::string(primTypeName(type)));
}

void Editor::deleteObject(const std::string& id) {
    auto& objs = project.objects;
    auto it = std::find_if(objs.begin(), objs.end(), [&](const SceneObject& o) { return o.id == id; });
    if (it == objs.end()) return;
    std::string name = it->name;
    auto mit = meshCache.find(id);
    if (mit != meshCache.end()) { mit->second.destroy(); meshCache.erase(mit); }
    objs.erase(it);
    auto& kfs = project.keyframes;
    kfs.erase(std::remove_if(kfs.begin(), kfs.end(), [&](const Keyframe& kf) { return kf.objectId == id; }), kfs.end());
    if (selectedId == id) selectedId = objs.empty() ? "" : objs[0].id;
    showStatus("Deleted " + name);
}

void Editor::duplicateObject() {
    SceneObject* src = getSelected();
    if (!src) return;
    SceneObject dup = *src;
    dup.id = generateUID();
    dup.name = src->name + "_copy";
    dup.position += glm::vec3(1.0f, 0.0f, 1.0f);
    selectedId = dup.id;
    project.objects.push_back(dup);
    showStatus("Duplicated " + src->name);
}

SceneObject* Editor::getSelected() {
    for (auto& obj : project.objects) if (obj.id == selectedId) return &obj;
    return nullptr;
}

void Editor::showStatus(const std::string& msg, float duration) {
    statusMessage = msg;
    statusTimer = duration;
}

void Editor::update(float dt) {
    if (statusTimer > 0) { statusTimer -= dt; if (statusTimer < 0) statusTimer = 0; }

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##MainWindow", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) { project.clear(); project.createDefaultScene(); clearMeshCache(); selectedId = project.objects.empty() ? "" : project.objects[0].id; }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { project.save("project.json") ? showStatus("Saved") : showStatus("Save failed"); }
            if (ImGui::MenuItem("Load", "Ctrl+O")) { if (project.load("project.json")) { clearMeshCache(); selectedId = project.objects.empty() ? "" : project.objects[0].id; showStatus("Loaded"); } }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Delete", "Del", false, getSelected() != nullptr)) deleteObject(selectedId);
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, getSelected() != nullptr)) duplicateObject();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Insert")) {
            if (ImGui::MenuItem("Box")) addObject(PrimType::Box);
            if (ImGui::MenuItem("Cylinder")) addObject(PrimType::Cylinder);
            if (ImGui::MenuItem("Cone")) addObject(PrimType::Cone);
            if (ImGui::MenuItem("Sphere")) addObject(PrimType::Sphere);
            if (ImGui::MenuItem("Wedge")) addObject(PrimType::Wedge);
            if (ImGui::MenuItem("Plane")) addObject(PrimType::Plane);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Run")) {
            if (ImGui::MenuItem(isRunning ? "Stop (F5)" : "Play (F5)")) isRunning = !isRunning;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTabBar("##Tabs")) {
        if (ImGui::BeginTabItem("Model Editor")) { currentTab = EditorTab::Model; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Event Editor")) { currentTab = EditorTab::Events; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Animation")) { currentTab = EditorTab::Animation; ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    switch (currentTab) {
    case EditorTab::Model: drawModelTab(dt); break;
    case EditorTab::Events: drawEventsTab(); break;
    case EditorTab::Animation: drawAnimationTab(dt); break;
    }

    ImGui::End();

    if (statusTimer > 0) {
        ImGuiViewport* mvp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(mvp->WorkPos.x + mvp->WorkSize.x * 0.5f - 100, mvp->WorkPos.y + mvp->WorkSize.y - 40));
        ImGui::SetNextWindowBgAlpha(0.85f);
        ImGui::Begin("##Status", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted(statusMessage.c_str());
        ImGui::End();
    }
}

void Editor::drawModelTab(float /*dt*/) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftW = 200.0f, rightW = 240.0f;
    float centerW = avail.x - leftW - rightW - 8.0f;
    if (centerW < 100) centerW = 100;

    ImGui::BeginChild("##Left", ImVec2(leftW, avail.y), true);
    ImGui::TextDisabled("Mode:");
    if (ImGui::RadioButton("Move", transformMode == TransformMode::Move)) transformMode = TransformMode::Move;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rot", transformMode == TransformMode::Rotate)) transformMode = TransformMode::Rotate;
    if (ImGui::RadioButton("Scale", transformMode == TransformMode::Scale)) transformMode = TransformMode::Scale;
    ImGui::SameLine();
    if (ImGui::RadioButton("Vtx", transformMode == TransformMode::Vertex)) transformMode = TransformMode::Vertex;
    ImGui::Separator();
    ImGui::TextDisabled("Objects (%d)", (int)project.objects.size());
    for (auto& obj : project.objects) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (obj.id == selectedId) flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx(obj.id.c_str(), flags, "%s [%s]", obj.name.c_str(), primTypeName(obj.type));
        if (ImGui::IsItemClicked()) selectedId = obj.id;
    }
    ImGui::Separator();
    float btnW = (ImGui::GetContentRegionAvail().x - 4) * 0.5f;
    if (ImGui::Button("Box", ImVec2(btnW, 0))) addObject(PrimType::Box);
    ImGui::SameLine();
    if (ImGui::Button("Cyl", ImVec2(btnW, 0))) addObject(PrimType::Cylinder);
    if (ImGui::Button("Cone", ImVec2(btnW, 0))) addObject(PrimType::Cone);
    ImGui::SameLine();
    if (ImGui::Button("Sph", ImVec2(btnW, 0))) addObject(PrimType::Sphere);
    if (ImGui::Button("Wedge", ImVec2(btnW, 0))) addObject(PrimType::Wedge);
    ImGui::SameLine();
    if (ImGui::Button("Plane", ImVec2(btnW, 0))) addObject(PrimType::Plane);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##Center", ImVec2(centerW, avail.y), false);
    {
        ImVec2 sz = ImGui::GetContentRegionAvail();
        int vw = std::max((int)sz.x, 1), vh = std::max((int)sz.y, 1);
        s_viewport.resize(vw, vh);
        s_viewport.begin();
        renderScene(vw, vh);
        s_viewport.end();
        ImGui::Image((ImTextureID)(intptr_t)s_viewport.getTexture(), ImVec2((float)vw, (float)vh), ImVec2(0, 1), ImVec2(1, 0));
        if (ImGui::IsItemHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseWheel != 0) camera.zoom(io.MouseWheel * -0.5f);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                ImVec2 delta = io.MouseDelta;
                if (io.KeyShift) camera.pan(delta.x, delta.y);
                else camera.orbit(delta.x, delta.y);
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##Right", ImVec2(rightW, avail.y), true);
    ImGui::TextColored(ImVec4(0.78f, 0.84f, 0.90f, 1.0f), "Properties");
    ImGui::Separator();
    PropertiesPanel::draw(getSelected(), transformMode);
    ImGui::EndChild();
}

void Editor::drawEventsTab() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##Events", avail, false);
    EventEditorPanel::draw(project.events);
    ImGui::EndChild();
}

void Editor::drawAnimationTab(float dt) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float topH = avail.y * 0.6f;

    ImGui::BeginChild("##AnimTop", ImVec2(avail.x, topH), false);
    {
        float listW = 160.0f;
        float vpW = ImGui::GetContentRegionAvail().x - listW - 4.0f;
        float h = ImGui::GetContentRegionAvail().y;

        ImGui::BeginChild("##AnimList", ImVec2(listW, h), true);
        for (auto& obj : project.objects) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (obj.id == selectedId) flags |= ImGuiTreeNodeFlags_Selected;
            ImGui::TreeNodeEx(obj.id.c_str(), flags, "%s", obj.name.c_str());
            if (ImGui::IsItemClicked()) selectedId = obj.id;
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##AnimVP", ImVec2(vpW, h), false);
        {
            ImVec2 sz = ImGui::GetContentRegionAvail();
            int vw = std::max((int)sz.x, 1), vh = std::max((int)sz.y, 1);
            s_viewport.resize(vw, vh);
            s_viewport.begin();
            renderScene(vw, vh);
            s_viewport.end();
            ImGui::Image((ImTextureID)(intptr_t)s_viewport.getTexture(), ImVec2((float)vw, (float)vh), ImVec2(0, 1), ImVec2(1, 0));
            if (ImGui::IsItemHovered()) {
                ImGuiIO& io = ImGui::GetIO();
                if (io.MouseWheel != 0) camera.zoom(io.MouseWheel * -0.5f);
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                    ImVec2 delta = io.MouseDelta;
                    if (io.KeyShift) camera.pan(delta.x, delta.y);
                    else camera.orbit(delta.x, delta.y);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    float botH = avail.y - topH - 4.0f;
    ImGui::BeginChild("##Timeline", ImVec2(avail.x, botH), true);
    {
        static TimelinePanel::State tlState;
        TimelinePanel::draw(tlState, project.objects, selectedId, project.keyframes);
    }
    ImGui::EndChild();
}

void Editor::render(int /*windowW*/, int /*windowH*/) {
    // Rendering is handled via ImGui + FBO in update()
}

void Editor::renderScene(int viewW, int viewH) {
    glViewport(0, 0, viewW, viewH);
    glClearColor(10.0f / 255.0f, 10.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    float aspect = (viewH > 0) ? (float)viewW / (float)viewH : 1.0f;
    glm::mat4 projection = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getOrbitViewMatrix();
    glm::mat4 vp = projection * view;

    renderer.drawGrid(vp, 40, 1.0f);
    renderer.drawAxes(vp, 3.0f);

    glUseProgram(renderer.sceneProgram);
    glUniform3f(renderer.sceneLightDir_loc, 0.4f, 0.8f, 0.3f);
    glUniform3f(renderer.sceneFogColor_loc, 10.0f / 255.0f, 10.0f / 255.0f, 26.0f / 255.0f);
    glUniform1f(renderer.sceneFogDensity_loc, 0.012f);

    for (auto& obj : project.objects) {
        if (!obj.visible) continue;
        Mesh& mesh = getOrCreateMesh(obj);
        glm::mat4 model = buildModelMatrix(obj);
        glm::mat4 mvp = vp * model;
        glUniformMatrix4fv(renderer.sceneMVP_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(renderer.sceneModel_loc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(renderer.sceneColor_loc, 1, glm::value_ptr(obj.color));
        glUniform3f(renderer.sceneEmissive_loc, obj.id == selectedId ? 0.08f : 0.0f, obj.id == selectedId ? 0.08f : 0.0f, obj.id == selectedId ? 0.15f : 0.0f);
        glBindVertexArray(mesh.vao);
        if (mesh.indexCount > 0) glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        else glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        glBindVertexArray(0);
    }

    SceneObject* sel = getSelected();
    if (sel) {
        gizmo.show(sel->position);
        gizmo.draw(view, projection, camera.getOrbitPosition());
    } else {
        gizmo.hide();
    }
}
