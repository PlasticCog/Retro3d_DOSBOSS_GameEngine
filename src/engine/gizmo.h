#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

class Gizmo {
public:
    bool init();
    void shutdown();

    void show(const glm::vec3& position);
    void hide();
    bool isVisible() const;

    void draw(const glm::mat4& view, const glm::mat4& proj,
              const glm::vec3& cameraPos);

    // Returns 'x', 'y', 'z' if an axis is hit, or 0 if none
    char hitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const;

    // Highlight an axis: 'x','y','z','a'(all), or 0 (none)
    void highlight(char axis);
    void clearHighlight();

    glm::vec3 position{0.0f};

private:
    bool visible = false;
    char highlightedAxis = 0;
    float scale = 1.0f;

    // Shader
    GLuint shaderProgram = 0;
    GLint mvpLoc = -1;
    GLint colorLoc = -1;

    // Line geometry (the shaft of each arrow)
    GLuint lineVAO = 0;
    GLuint lineVBO = 0;

    // Cone geometry (the arrowhead)
    GLuint coneVAO = 0;
    GLuint coneVBO = 0;
    GLuint coneEBO = 0;
    int coneIndexCount = 0;

    void buildLine();
    void buildCone();
    void drawArrow(const glm::mat4& vp, char axis) const;
};
