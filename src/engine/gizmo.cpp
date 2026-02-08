#include "engine/gizmo.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Gizmo shader: simple unlit with uniform colour
// ---------------------------------------------------------------------------
static const char* gizmoVS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* gizmoFS = R"(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Shader compilation helpers (local to this translation unit)
// ---------------------------------------------------------------------------
static GLuint compileShaderLocal(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Gizmo shader compile error: " << log << std::endl;
    }
    return s;
}

static GLuint linkProgramLocal(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "Gizmo program link error: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static const float SHAFT_LENGTH = 1.0f;   // length of the line shaft
static const float CONE_HEIGHT  = 0.25f;  // arrowhead height
static const float CONE_RADIUS  = 0.06f;  // arrowhead base radius
static const int   CONE_SEGMENTS = 8;
static const float HIT_RADIUS   = 0.12f;  // cylinder radius for hit testing

// ---------------------------------------------------------------------------
// init()
// ---------------------------------------------------------------------------
bool Gizmo::init() {
    // Compile gizmo shader
    GLuint vs = compileShaderLocal(GL_VERTEX_SHADER, gizmoVS);
    GLuint fs = compileShaderLocal(GL_FRAGMENT_SHADER, gizmoFS);
    shaderProgram = linkProgramLocal(vs, fs);
    mvpLoc  = glGetUniformLocation(shaderProgram, "uMVP");
    colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    buildLine();
    buildCone();

    return true;
}

// ---------------------------------------------------------------------------
// shutdown()
// ---------------------------------------------------------------------------
void Gizmo::shutdown() {
    if (shaderProgram) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
    if (lineVAO) { glDeleteVertexArrays(1, &lineVAO); lineVAO = 0; }
    if (lineVBO) { glDeleteBuffers(1, &lineVBO); lineVBO = 0; }
    if (coneVAO) { glDeleteVertexArrays(1, &coneVAO); coneVAO = 0; }
    if (coneVBO) { glDeleteBuffers(1, &coneVBO); coneVBO = 0; }
    if (coneEBO) { glDeleteBuffers(1, &coneEBO); coneEBO = 0; }
}

// ---------------------------------------------------------------------------
// buildLine(): a single line segment from (0,0,0) to (0, SHAFT_LENGTH, 0)
// ---------------------------------------------------------------------------
void Gizmo::buildLine() {
    float verts[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, SHAFT_LENGTH, 0.0f
    };

    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// buildCone(): small cone pointing in +Y, base at y=0, tip at y=CONE_HEIGHT
// ---------------------------------------------------------------------------
void Gizmo::buildCone() {
    std::vector<float> verts;
    std::vector<unsigned int> inds;

    // Tip vertex at index 0
    verts.push_back(0.0f);
    verts.push_back(CONE_HEIGHT);
    verts.push_back(0.0f);

    // Base ring vertices
    for (int i = 0; i <= CONE_SEGMENTS; i++) {
        float a = (float)(2.0 * M_PI * i / CONE_SEGMENTS);
        verts.push_back(CONE_RADIUS * cosf(a));
        verts.push_back(0.0f);
        verts.push_back(CONE_RADIUS * sinf(a));
    }

    // Center of base for bottom cap
    int baseCenterIdx = (int)(verts.size() / 3);
    verts.push_back(0.0f);
    verts.push_back(0.0f);
    verts.push_back(0.0f);

    // Side triangles: tip (0) to each base edge
    for (int i = 0; i < CONE_SEGMENTS; i++) {
        inds.push_back(0);
        inds.push_back(i + 1);
        inds.push_back(i + 2);
    }

    // Bottom cap triangles
    for (int i = 0; i < CONE_SEGMENTS; i++) {
        inds.push_back(baseCenterIdx);
        inds.push_back(i + 2);
        inds.push_back(i + 1);
    }

    coneIndexCount = (int)inds.size();

    glGenVertexArrays(1, &coneVAO);
    glGenBuffers(1, &coneVBO);
    glGenBuffers(1, &coneEBO);

    glBindVertexArray(coneVAO);

    glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coneEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int),
                 inds.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// show / hide / isVisible
// ---------------------------------------------------------------------------
void Gizmo::show(const glm::vec3& pos) {
    position = pos;
    visible = true;
}

void Gizmo::hide() {
    visible = false;
}

bool Gizmo::isVisible() const {
    return visible;
}

// ---------------------------------------------------------------------------
// highlight / clearHighlight
// ---------------------------------------------------------------------------
void Gizmo::highlight(char axis) {
    highlightedAxis = axis;
}

void Gizmo::clearHighlight() {
    highlightedAxis = 0;
}

// ---------------------------------------------------------------------------
// drawArrow(): draw a single arrow along an axis
//   The line and cone are built along +Y. We rotate to the desired axis.
// ---------------------------------------------------------------------------
void Gizmo::drawArrow(const glm::mat4& vp, char axis) const {
    // Determine rotation to orient +Y toward the desired axis
    glm::mat4 rot(1.0f);
    glm::vec3 color;

    bool isHighlighted = (highlightedAxis == axis || highlightedAxis == 'a');

    switch (axis) {
        case 'x':
            rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                               glm::vec3(0, 0, 1));
            color = isHighlighted ? glm::vec3(1.0f, 0.6f, 0.6f)
                                  : glm::vec3(0.9f, 0.1f, 0.1f);
            break;
        case 'y':
            // Already along +Y, no rotation needed
            color = isHighlighted ? glm::vec3(0.6f, 1.0f, 0.6f)
                                  : glm::vec3(0.1f, 0.9f, 0.1f);
            break;
        case 'z':
            rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                               glm::vec3(1, 0, 0));
            color = isHighlighted ? glm::vec3(0.6f, 0.6f, 1.0f)
                                  : glm::vec3(0.1f, 0.3f, 0.9f);
            break;
        default:
            return;
    }

    glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    glm::mat4 model = translate * scaleM * rot;
    glm::mat4 mvp = vp * model;

    glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    // Draw shaft (line)
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw cone at end of shaft
    glm::mat4 coneOffset = glm::translate(glm::mat4(1.0f),
                                           glm::vec3(0, SHAFT_LENGTH, 0));
    glm::mat4 coneMVP = vp * model * coneOffset;
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(coneMVP));
    glBindVertexArray(coneVAO);
    glDrawElements(GL_TRIANGLES, coneIndexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// draw(): render all three arrows
// ---------------------------------------------------------------------------
void Gizmo::draw(const glm::mat4& view, const glm::mat4& proj,
                 const glm::vec3& cameraPos) {
    if (!visible) return;

    // Auto-scale based on camera distance so the gizmo appears constant size
    float dist = glm::length(cameraPos - position);
    scale = dist * 0.1f;
    if (scale < 0.2f) scale = 0.2f;
    if (scale > 20.0f) scale = 20.0f;

    glm::mat4 vp = proj * view;

    glUseProgram(shaderProgram);

    // Disable depth test so gizmo is always visible on top
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glLineWidth(2.5f);

    drawArrow(vp, 'x');
    drawArrow(vp, 'y');
    drawArrow(vp, 'z');

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

// ---------------------------------------------------------------------------
// hitTest(): ray-cylinder intersection test for each axis arrow
//   Returns 'x', 'y', 'z' for the closest hit, or 0 for no hit.
// ---------------------------------------------------------------------------
char Gizmo::hitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const {
    if (!visible) return 0;

    float totalLength = (SHAFT_LENGTH + CONE_HEIGHT) * scale;
    float hitRadius = HIT_RADIUS * scale;

    // Test each axis as a finite cylinder from position along the axis direction
    struct AxisTest {
        char axis;
        glm::vec3 dir;
    };
    AxisTest axes[3] = {
        {'x', glm::vec3(1, 0, 0)},
        {'y', glm::vec3(0, 1, 0)},
        {'z', glm::vec3(0, 0, 1)},
    };

    char bestAxis = 0;
    float bestDist = 1e30f;

    for (auto& at : axes) {
        // Ray-cylinder intersection test
        // The cylinder goes from 'position' along 'at.dir' for 'totalLength'
        // with radius 'hitRadius'.
        glm::vec3 cylDir = at.dir;
        glm::vec3 oc = rayOrigin - position;

        // Project out the cylinder axis component
        float dDotA = glm::dot(rayDir, cylDir);
        float ocDotA = glm::dot(oc, cylDir);

        glm::vec3 dPerp = rayDir - dDotA * cylDir;
        glm::vec3 ocPerp = oc - ocDotA * cylDir;

        float a = glm::dot(dPerp, dPerp);
        float b = 2.0f * glm::dot(dPerp, ocPerp);
        float c = glm::dot(ocPerp, ocPerp) - hitRadius * hitRadius;

        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f) continue;

        float sqrtDisc = sqrtf(disc);
        float t = (-b - sqrtDisc) / (2.0f * a);
        if (t < 0.0f) {
            t = (-b + sqrtDisc) / (2.0f * a);
        }
        if (t < 0.0f) continue;

        // Check that the hit point is within the cylinder's length
        glm::vec3 hitPt = rayOrigin + t * rayDir;
        float along = glm::dot(hitPt - position, cylDir);
        if (along < -hitRadius || along > totalLength + hitRadius) continue;

        if (t < bestDist) {
            bestDist = t;
            bestAxis = at.axis;
        }
    }

    return bestAxis;
}
