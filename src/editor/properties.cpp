#include "editor/properties.h"
#include <imgui.h>
#include <cstring>

// -----------------------------------------------------------------------
// Helpers -- colored axis drag-floats
// -----------------------------------------------------------------------

// Draw a single colored DragFloat for one axis.
// label: display label ("X", "Y", "Z")
// value: pointer to the float to edit
// col:   the highlight color for the label
// speed: drag speed
static bool coloredDragFloat(const char* label, float* value,
                             const ImVec4& col, float speed = 0.1f) {
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // Build a unique hidden label from the visible label and pointer address
    char uid[64];
    std::snprintf(uid, sizeof(uid), "##%s_%p", label, (void*)value);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    return ImGui::DragFloat(uid, value, speed);
}

// Draw a vec3 editor with colored X / Y / Z fields.
// Returns true if any component was changed.
static bool coloredVec3(const char* sectionLabel, float* v, float speed = 0.1f) {
    ImGui::TextDisabled("%s", sectionLabel);

    bool changed = false;
    ImGui::Indent(8.0f);
    changed |= coloredDragFloat("X", &v[0], ImVec4(1.0f, 0.3f, 0.3f, 1.0f), speed);
    changed |= coloredDragFloat("Y", &v[1], ImVec4(0.3f, 1.0f, 0.3f, 1.0f), speed);
    changed |= coloredDragFloat("Z", &v[2], ImVec4(0.4f, 0.5f, 1.0f, 1.0f), speed);
    ImGui::Unindent(8.0f);
    return changed;
}

// -----------------------------------------------------------------------
// PropertiesPanel::draw
// -----------------------------------------------------------------------
void PropertiesPanel::draw(SceneObject* obj, TransformMode mode) {
    if (!obj) {
        ImGui::TextDisabled("No object selected.");
        return;
    }

    // ------- Name -------
    ImGui::TextDisabled("Name");
    char nameBuf[128];
    std::strncpy(nameBuf, obj->name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
        obj->name = nameBuf;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ------- Type (read-only) -------
    ImGui::TextDisabled("Type");
    ImGui::SameLine();
    ImGui::Text("%s", primTypeName(obj->type));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ------- Position -------
    coloredVec3("Position", &obj->position.x, 0.1f);

    ImGui::Spacing();

    // ------- Rotation (degrees) -------
    coloredVec3("Rotation", &obj->rotation.x, 1.0f);

    ImGui::Spacing();

    // ------- Scale -------
    coloredVec3("Scale", &obj->scale.x, 0.05f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ------- Color picker -------
    ImGui::TextDisabled("Color");
    ImGui::ColorEdit3("##Color", &obj->color.x,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ------- Visibility -------
    ImGui::Checkbox("Visible", &obj->visible);

    // ------- Vertex info (shown in Vertex mode) -------
    if (mode == TransformMode::Vertex) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Vertex Mode");

        if (obj->customVerts.empty()) {
            ImGui::TextWrapped(
                "This object uses default geometry. "
                "Vertex editing will create a custom copy of the mesh.");
        } else {
            int vertCount = (int)obj->customVerts.size() / 3;
            ImGui::Text("Custom vertices: %d", vertCount);

            ImGui::Spacing();
            if (ImGui::TreeNode("Vertex Data")) {
                // Show a scrollable list of vertices
                ImGui::BeginChild("##VertScroll", ImVec2(0, 200), true);
                for (int i = 0; i < vertCount; i++) {
                    ImGui::PushID(i);
                    float* vp = &obj->customVerts[i * 3];
                    char lbl[32];
                    std::snprintf(lbl, sizeof(lbl), "V%d", i);
                    ImGui::SetNextItemWidth(-1);
                    ImGui::DragFloat3(lbl, vp, 0.01f);
                    ImGui::PopID();
                }
                ImGui::EndChild();
                ImGui::TreePop();
            }
        }

        if (ImGui::Button("Reset to Default Geometry")) {
            obj->customVerts.clear();
        }
    }
}
