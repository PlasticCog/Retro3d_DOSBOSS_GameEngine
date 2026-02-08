#include "editor/timeline.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace TimelinePanel {

void draw(State& state, std::vector<SceneObject>& objects,
          const std::string& selectedId, std::vector<Keyframe>& keyframes) {

    // -------------------------------------------------------------------
    // Playback: advance currentFrame when playing using ImGui's delta time
    // -------------------------------------------------------------------
    if (state.playing) {
        float dt = ImGui::GetIO().DeltaTime;
        static float accumulator = 0.0f;
        float frameTime = 1.0f / 24.0f;
        accumulator += dt;
        while (accumulator >= frameTime) {
            accumulator -= frameTime;
            state.currentFrame++;
            if (state.currentFrame >= state.totalFrames) {
                state.currentFrame = 0;
            }
        }
    }

    // -------------------------------------------------------------------
    // Top control bar: Play / Stop / + Keyframe / Frame / Total
    // -------------------------------------------------------------------
    if (state.playing) {
        if (ImGui::Button("Pause")) {
            state.playing = false;
        }
    } else {
        if (ImGui::Button("Play")) {
            state.playing = true;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        state.currentFrame = 0;
        state.playing = false;
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // Add Keyframe button -- only active when an object is selected
    SceneObject* sel = nullptr;
    for (auto& o : objects) {
        if (o.id == selectedId) { sel = &o; break; }
    }

    if (sel) {
        if (ImGui::Button("+ Keyframe")) {
            // Update existing keyframe or create a new one
            bool exists = false;
            for (auto& kf : keyframes) {
                if (kf.objectId == sel->id && kf.frame == state.currentFrame) {
                    kf.position = sel->position;
                    kf.rotation = sel->rotation;
                    kf.scale = sel->scale;
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                Keyframe kf;
                kf.id = generateUID();
                kf.objectId = sel->id;
                kf.frame = state.currentFrame;
                kf.position = sel->position;
                kf.rotation = sel->rotation;
                kf.scale = sel->scale;
                keyframes.push_back(kf);
            }
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("+ Keyframe");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    ImGui::DragInt("Frame", &state.currentFrame, 1.0f, 0, state.totalFrames - 1);
    ImGui::SameLine();
    ImGui::Text("/");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::DragInt("Total", &state.totalFrames, 1.0f, 1, 600);

    // Clamp
    if (state.currentFrame < 0) state.currentFrame = 0;
    if (state.currentFrame >= state.totalFrames) state.currentFrame = state.totalFrames - 1;

    ImGui::Separator();

    // Frame scrubber
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##scrubber", &state.currentFrame, 0, state.totalFrames - 1, "Frame %d");

    // -------------------------------------------------------------------
    // Timeline grid: object names left, frames across top, keyframe
    // diamonds, playhead line at currentFrame
    // -------------------------------------------------------------------
    ImGui::BeginChild("TimelineGrid", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    const float nameColWidth = 130.0f;
    const float cellWidth    = 14.0f;
    const float rowHeight    = 22.0f;
    const float headerHeight = 20.0f;

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float totalWidth  = nameColWidth + state.totalFrames * cellWidth;
    float totalHeight = headerHeight + objects.size() * rowHeight;

    // Background
    dl->AddRectFilled(origin,
        ImVec2(origin.x + totalWidth, origin.y + totalHeight),
        IM_COL32(18, 20, 30, 255));

    // -------------------------------------------------------------------
    // Header row: frame numbers every 5 frames
    // -------------------------------------------------------------------
    float timelineX = origin.x + nameColWidth;

    for (int f = 0; f < state.totalFrames; f++) {
        float x = timelineX + f * cellWidth;

        // Vertical grid lines with varying brightness
        ImU32 gridCol = (f % 10 == 0) ? IM_COL32(55, 60, 80, 255)
                      : (f %  5 == 0) ? IM_COL32(40, 45, 60, 255)
                                       : IM_COL32(30, 33, 45, 255);
        dl->AddLine(ImVec2(x, origin.y + headerHeight),
                    ImVec2(x, origin.y + totalHeight), gridCol);

        // Frame number label every 5 frames
        if (f % 5 == 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", f);
            dl->AddText(ImVec2(x + 2, origin.y + 2),
                        IM_COL32(140, 155, 180, 255), buf);
        }

        // Playhead highlight column
        if (f == state.currentFrame) {
            dl->AddRectFilled(
                ImVec2(x, origin.y + headerHeight),
                ImVec2(x + cellWidth, origin.y + totalHeight),
                IM_COL32(0, 180, 255, 25));
        }
    }

    // Name column separator
    dl->AddLine(ImVec2(origin.x + nameColWidth, origin.y),
                ImVec2(origin.x + nameColWidth, origin.y + totalHeight),
                IM_COL32(60, 65, 85, 255));

    // Header/body separator
    dl->AddLine(ImVec2(origin.x, origin.y + headerHeight),
                ImVec2(origin.x + totalWidth, origin.y + headerHeight),
                IM_COL32(60, 65, 85, 255));

    // -------------------------------------------------------------------
    // Object rows
    // -------------------------------------------------------------------
    for (size_t oi = 0; oi < objects.size(); oi++) {
        auto& obj = objects[oi];
        float rowY = origin.y + headerHeight + oi * rowHeight;

        // Highlight selected object row
        bool isSel = (obj.id == selectedId);
        if (isSel) {
            dl->AddRectFilled(
                ImVec2(origin.x, rowY),
                ImVec2(origin.x + totalWidth, rowY + rowHeight),
                IM_COL32(30, 55, 100, 180));
        }

        // Row separator line
        dl->AddLine(ImVec2(origin.x, rowY + rowHeight),
                    ImVec2(origin.x + totalWidth, rowY + rowHeight),
                    IM_COL32(35, 38, 55, 255));

        // Object name text
        ImU32 nameCol = isSel ? IM_COL32(230, 240, 255, 255)
                              : IM_COL32(190, 200, 220, 255);
        dl->AddText(ImVec2(origin.x + 6, rowY + 3), nameCol, obj.name.c_str());

        // Draw keyframe diamonds for this object
        for (auto& kf : keyframes) {
            if (kf.objectId != obj.id) continue;
            if (kf.frame < 0 || kf.frame >= state.totalFrames) continue;

            float kx = timelineX + kf.frame * cellWidth + cellWidth * 0.5f;
            float ky = rowY + rowHeight * 0.5f;
            float sz = 4.5f;

            ImU32 diamondFill = isSel ? IM_COL32(255, 200, 50, 255)
                                      : IM_COL32(220, 160, 40, 255);
            ImU32 diamondEdge = isSel ? IM_COL32(255, 230, 100, 255)
                                      : IM_COL32(255, 200, 80, 200);

            // Filled diamond shape
            dl->AddQuadFilled(
                ImVec2(kx, ky - sz),
                ImVec2(kx + sz, ky),
                ImVec2(kx, ky + sz),
                ImVec2(kx - sz, ky),
                diamondFill);
            // Diamond outline
            dl->AddQuad(
                ImVec2(kx, ky - sz - 0.5f),
                ImVec2(kx + sz + 0.5f, ky),
                ImVec2(kx, ky + sz + 0.5f),
                ImVec2(kx - sz - 0.5f, ky),
                diamondEdge);
        }
    }

    // -------------------------------------------------------------------
    // Playhead line with triangle indicator
    // -------------------------------------------------------------------
    {
        float px = timelineX + state.currentFrame * cellWidth + cellWidth * 0.5f;

        // Red triangle at top of header
        dl->AddTriangleFilled(
            ImVec2(px - 6, origin.y),
            ImVec2(px + 6, origin.y),
            ImVec2(px, origin.y + 10),
            IM_COL32(255, 70, 70, 255));

        // Red vertical line through all rows
        dl->AddLine(
            ImVec2(px, origin.y + 10),
            ImVec2(px, origin.y + totalHeight),
            IM_COL32(255, 70, 70, 220), 2.0f);
    }

    // -------------------------------------------------------------------
    // Mouse interaction
    // -------------------------------------------------------------------
    ImGui::SetCursorScreenPos(origin);
    ImGui::InvisibleButton("##timeline_grid",
        ImVec2(totalWidth, totalHeight));

    if (ImGui::IsItemHovered()) {
        ImVec2 mp = ImGui::GetMousePos();
        float mx = mp.x - timelineX;
        float my = mp.y - origin.y - headerHeight;

        int clickedFrame = (int)(mx / cellWidth);
        int clickedRow   = (int)(my / rowHeight);

        // Single click: seek to the clicked frame
        if (ImGui::IsMouseClicked(0) && mx >= 0.0f &&
            clickedFrame >= 0 && clickedFrame < state.totalFrames) {
            state.currentFrame = clickedFrame;
        }

        // Double-click on a keyframe diamond: delete it
        if (ImGui::IsMouseDoubleClicked(0) && mx >= 0.0f &&
            clickedRow >= 0 && clickedRow < (int)objects.size() &&
            clickedFrame >= 0 && clickedFrame < state.totalFrames) {

            const std::string& objId = objects[clickedRow].id;
            for (auto it = keyframes.begin(); it != keyframes.end(); ++it) {
                if (it->objectId == objId && it->frame == clickedFrame) {
                    keyframes.erase(it);
                    break;
                }
            }
        }
    }

    if (objects.empty()) {
        ImGui::SetCursorScreenPos(ImVec2(origin.x + 10, origin.y + headerHeight + 10));
        ImGui::TextColored(ImVec4(0.45f, 0.52f, 0.65f, 1.0f),
                           "No objects in scene.");
    }

    ImGui::EndChild();
}

} // namespace TimelinePanel
