#include "editor/timeline.h"
#include <imgui.h>

namespace TimelinePanel {

void draw(State& state, float dt, std::vector<SceneObject>& objects,
          const std::string& selectedId, std::vector<Keyframe>& keyframes) {

    if (state.playing) {
        state.accumulator += dt;
        float frameTime = 1.0f / 24.0f;
        while (state.accumulator >= frameTime) {
            state.accumulator -= frameTime;
            state.currentFrame = (state.currentFrame + 1) % state.totalFrames;
        }
    }

    if (ImGui::Button("Stop")) { state.currentFrame = 0; state.playing = false; }
    ImGui::SameLine();
    if (ImGui::Button(state.playing ? "Pause" : "Play")) state.playing = !state.playing;
    ImGui::SameLine();
    ImGui::Text("Frame: %d / %d", state.currentFrame, state.totalFrames);

    SceneObject* sel = nullptr;
    for (auto& o : objects) if (o.id == selectedId) { sel = &o; break; }

    ImGui::SameLine();
    if (sel) {
        if (ImGui::Button("+ Keyframe")) {
            Keyframe kf;
            kf.id = generateUID();
            kf.objectId = sel->id;
            kf.frame = state.currentFrame;
            kf.position = sel->position;
            kf.rotation = sel->rotation;
            kf.scale = sel->scale;
            keyframes.push_back(kf);
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("+ Keyframe");
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    ImGui::SliderInt("##frame", &state.currentFrame, 0, state.totalFrames - 1, "Frame %d");
    ImGui::Separator();

    // Simple keyframe list
    for (size_t oi = 0; oi < objects.size(); oi++) {
        auto& obj = objects[oi];
        bool isSel = (obj.id == selectedId);
        if (isSel) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.82f, 1.0f, 1.0f));
        ImGui::Text("%s:", obj.name.c_str());
        if (isSel) ImGui::PopStyleColor();
        ImGui::SameLine();

        for (size_t ki = 0; ki < keyframes.size(); ki++) {
            auto& kf = keyframes[ki];
            if (kf.objectId != obj.id) continue;
            ImGui::SameLine();
            char label[32];
            snprintf(label, sizeof(label), "[F%d]##kf%zu", kf.frame, ki);
            if (ImGui::SmallButton(label)) {
                state.currentFrame = kf.frame;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                keyframes.erase(keyframes.begin() + (int)ki);
                ki--;
            }
        }
    }

    if (objects.empty()) {
        ImGui::TextColored(ImVec4(0.42f, 0.5f, 0.63f, 1.0f), "No objects in scene.");
    }
}

}
