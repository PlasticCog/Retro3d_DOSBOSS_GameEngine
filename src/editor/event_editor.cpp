#include "editor/event_editor.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <set>

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

// Find the ConditionDef for a given type id.
static const ConditionDef* findConditionDef(const std::string& type) {
    for (auto& cd : CONDITIONS) {
        if (cd.id == type) return &cd;
    }
    return nullptr;
}

// Find the ActionDef for a given type id.
static const ActionDef* findActionDef(const std::string& type) {
    for (auto& ad : ACTIONS) {
        if (ad.id == type) return &ad;
    }
    return nullptr;
}

// Collect unique category names from a list of defs.
static std::set<std::string> collectCategories(const std::vector<ConditionDef>& defs) {
    std::set<std::string> cats;
    for (auto& d : defs) cats.insert(d.category);
    return cats;
}

static std::set<std::string> collectCategories(const std::vector<ActionDef>& defs) {
    std::set<std::string> cats;
    for (auto& d : defs) cats.insert(d.category);
    return cats;
}

// Draw a colored tag pill.
static void drawTag(const char* text, const ImVec4& col) {
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float pad = 4.0f;
    ImVec2 rMin(cursor.x, cursor.y);
    ImVec2 rMax(cursor.x + textSize.x + pad * 2, cursor.y + textSize.y + pad * 2);

    ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(
        ImVec4(col.x * 0.3f, col.y * 0.3f, col.z * 0.3f, 0.85f));
    ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(col);

    dl->AddRectFilled(rMin, rMax, bgCol, 4.0f);
    dl->AddRect(rMin, rMax, borderCol, 4.0f);
    dl->AddText(ImVec2(cursor.x + pad, cursor.y + pad),
                ImGui::ColorConvertFloat4ToU32(col), text);

    ImGui::Dummy(ImVec2(textSize.x + pad * 2, textSize.y + pad * 2));
}

// Editable parameter field drawn inline.  Returns true if modified.
static bool paramField(const std::string& paramName, const std::string& paramType,
                       std::map<std::string, std::string>& params) {
    bool changed = false;
    std::string& val = params[paramName];

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);

    char uid[128];
    std::snprintf(uid, sizeof(uid), "##p_%s_%p", paramName.c_str(), (void*)&val);

    if (paramType == "number") {
        float fv = 0.0f;
        try { fv = std::stof(val); } catch (...) {}
        if (ImGui::DragFloat(uid, &fv, 0.1f)) {
            val = std::to_string(fv);
            changed = true;
        }
    } else if (paramType == "key") {
        char buf[16];
        std::strncpy(buf, val.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText(uid, buf, sizeof(buf))) {
            val = buf;
            changed = true;
        }
    } else if (paramType == "comparison") {
        const char* ops[] = {"==", "!=", "<", ">", "<=", ">="};
        int cur = 0;
        for (int i = 0; i < 6; i++) {
            if (val == ops[i]) { cur = i; break; }
        }
        if (ImGui::Combo(uid, &cur, ops, 6)) {
            val = ops[cur];
            changed = true;
        }
    } else if (paramType == "axis") {
        const char* axes[] = {"X", "Y", "Z"};
        int cur = 0;
        if (val == "Y") cur = 1;
        else if (val == "Z") cur = 2;
        if (ImGui::Combo(uid, &cur, axes, 3)) {
            val = axes[cur];
            changed = true;
        }
    } else {
        // string / object -- free text
        char buf[64];
        std::strncpy(buf, val.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText(uid, buf, sizeof(buf))) {
            val = buf;
            changed = true;
        }
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s (%s)", paramName.c_str(), paramType.c_str());
    }
    return changed;
}

// -----------------------------------------------------------------------
// Condition picker popup -- organized by category
// -----------------------------------------------------------------------
static bool conditionPickerPopup(const char* popupId, EventCondition& out) {
    bool picked = false;
    if (ImGui::BeginPopup(popupId)) {
        ImGui::TextDisabled("Pick a condition:");
        ImGui::Separator();

        auto cats = collectCategories(CONDITIONS);
        for (auto& cat : cats) {
            if (ImGui::BeginMenu(cat.c_str())) {
                for (auto& cd : CONDITIONS) {
                    if (cd.category != cat) continue;
                    if (ImGui::MenuItem(cd.label.c_str())) {
                        out.id    = generateUID();
                        out.type  = cd.id;
                        out.label = cd.label;
                        out.negate = false;
                        out.params.clear();
                        for (auto& p : cd.params) {
                            out.params[p.name] = "";
                        }
                        picked = true;
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }
    return picked;
}

// -----------------------------------------------------------------------
// Action picker popup -- organized by category
// -----------------------------------------------------------------------
static bool actionPickerPopup(const char* popupId, EventAction& out) {
    bool picked = false;
    if (ImGui::BeginPopup(popupId)) {
        ImGui::TextDisabled("Pick an action:");
        ImGui::Separator();

        auto cats = collectCategories(ACTIONS);
        for (auto& cat : cats) {
            if (ImGui::BeginMenu(cat.c_str())) {
                for (auto& ad : ACTIONS) {
                    if (ad.category != cat) continue;
                    if (ImGui::MenuItem(ad.label.c_str())) {
                        out.id    = generateUID();
                        out.type  = ad.id;
                        out.label = ad.label;
                        out.params.clear();
                        for (auto& p : ad.params) {
                            out.params[p.name] = "";
                        }
                        picked = true;
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }
    return picked;
}

// -----------------------------------------------------------------------
// Draw a single condition with its tag and parameter inputs
// -----------------------------------------------------------------------
static void drawCondition(EventCondition& cond, bool& requestDelete) {
    const ConditionDef* def = findConditionDef(cond.type);

    // Negate checkbox
    ImGui::Checkbox("NOT", &cond.negate);
    ImGui::SameLine();

    // Tag color: blue for conditions
    ImVec4 tagCol = cond.negate
        ? ImVec4(1.0f, 0.45f, 0.45f, 1.0f)  // red tint when negated
        : ImVec4(0.4f, 0.7f, 1.0f, 1.0f);    // blue
    drawTag(cond.label.c_str(), tagCol);

    // Parameter fields
    if (def) {
        for (auto& pd : def->params) {
            paramField(pd.name, pd.type, cond.params);
        }
    }

    // Delete button for this condition
    ImGui::SameLine();
    char duid[64];
    std::snprintf(duid, sizeof(duid), "x##cd_%s", cond.id.c_str());
    if (ImGui::SmallButton(duid)) {
        requestDelete = true;
    }
}

// -----------------------------------------------------------------------
// Draw a single action with its tag and parameter inputs
// -----------------------------------------------------------------------
static void drawAction(EventAction& action, bool& requestDelete) {
    const ActionDef* def = findActionDef(action.type);

    // Tag color: green for actions
    ImVec4 tagCol(0.4f, 1.0f, 0.5f, 1.0f);
    drawTag(action.label.c_str(), tagCol);

    // Parameter fields
    if (def) {
        for (auto& pd : def->params) {
            paramField(pd.name, pd.type, action.params);
        }
    }

    // Delete button for this action
    ImGui::SameLine();
    char duid[64];
    std::snprintf(duid, sizeof(duid), "x##ad_%s", action.id.c_str());
    if (ImGui::SmallButton(duid)) {
        requestDelete = true;
    }
}

// -----------------------------------------------------------------------
// EventEditorPanel::draw
// -----------------------------------------------------------------------
void EventEditorPanel::draw(std::vector<GameEvent>& events) {
    // "Add Event" button at top
    if (ImGui::Button("+ Add Event")) {
        GameEvent evt;
        evt.id = generateUID();
        evt.enabled = true;
        events.push_back(evt);
    }

    ImGui::Separator();

    // Event table
    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("##EventTable", 5, tableFlags)) return;

    ImGui::TableSetupColumn("#",         ImGuiTableColumnFlags_WidthFixed,  40.0f);
    ImGui::TableSetupColumn("Enabled",   ImGuiTableColumnFlags_WidthFixed,  55.0f);
    ImGui::TableSetupColumn("Conditions",ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Actions",   ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Delete",    ImGuiTableColumnFlags_WidthFixed,  50.0f);
    ImGui::TableHeadersRow();

    int deleteEventIdx = -1;

    for (int evtIdx = 0; evtIdx < (int)events.size(); evtIdx++) {
        GameEvent& evt = events[evtIdx];
        ImGui::PushID(evt.id.c_str());

        ImGui::TableNextRow();

        // Column 0: row number
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", evtIdx + 1);

        // Column 1: enabled checkbox
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##en", &evt.enabled);

        // Column 2: conditions
        ImGui::TableSetColumnIndex(2);
        {
            int delCondIdx = -1;
            for (int ci = 0; ci < (int)evt.conditions.size(); ci++) {
                ImGui::PushID(ci);
                bool del = false;
                drawCondition(evt.conditions[ci], del);
                if (del) delCondIdx = ci;
                ImGui::PopID();
            }
            if (delCondIdx >= 0) {
                evt.conditions.erase(evt.conditions.begin() + delCondIdx);
            }

            // Add condition button + popup picker
            char addCid[64];
            std::snprintf(addCid, sizeof(addCid), "AddCond_%s", evt.id.c_str());
            if (ImGui::SmallButton("+ Cond")) {
                ImGui::OpenPopup(addCid);
            }
            EventCondition newCond;
            if (conditionPickerPopup(addCid, newCond)) {
                evt.conditions.push_back(newCond);
            }
        }

        // Column 3: actions
        ImGui::TableSetColumnIndex(3);
        {
            int delActIdx = -1;
            for (int ai = 0; ai < (int)evt.actions.size(); ai++) {
                ImGui::PushID(ai);
                bool del = false;
                drawAction(evt.actions[ai], del);
                if (del) delActIdx = ai;
                ImGui::PopID();
            }
            if (delActIdx >= 0) {
                evt.actions.erase(evt.actions.begin() + delActIdx);
            }

            // Add action button + popup picker
            char addAid[64];
            std::snprintf(addAid, sizeof(addAid), "AddAct_%s", evt.id.c_str());
            if (ImGui::SmallButton("+ Action")) {
                ImGui::OpenPopup(addAid);
            }
            EventAction newAct;
            if (actionPickerPopup(addAid, newAct)) {
                evt.actions.push_back(newAct);
            }
        }

        // Column 4: delete event
        ImGui::TableSetColumnIndex(4);
        if (ImGui::SmallButton("Del")) {
            deleteEventIdx = evtIdx;
        }

        ImGui::PopID();
    }

    ImGui::EndTable();

    // Perform event deletion outside the loop
    if (deleteEventIdx >= 0 && deleteEventIdx < (int)events.size()) {
        events.erase(events.begin() + deleteEventIdx);
    }
}
