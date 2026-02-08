#pragma once
#include "core/types.h"

namespace PropertiesPanel {
    // Draw the properties panel for the currently selected object.
    // Pass nullptr when nothing is selected.
    void draw(SceneObject* obj, TransformMode mode);
}
