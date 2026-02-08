#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

// Simple shader-based renderer for the editor and game
class Renderer {
public:
    bool init();
    void shutdown();

    // Scene shader (lit meshes)
    GLuint sceneProgram = 0;
    GLint sceneMVP_loc = -1;
    GLint sceneModel_loc = -1;
    GLint sceneColor_loc = -1;
    GLint sceneEmissive_loc = -1;
    GLint sceneLightDir_loc = -1;
    GLint sceneFogColor_loc = -1;
    GLint sceneFogDensity_loc = -1;

    // Grid/line shader (unlit)
    GLuint lineProgram = 0;
    GLint lineMVP_loc = -1;
    GLint lineColor_loc = -1;

    // Draw helpers
    void drawGrid(const glm::mat4& vp, int size, float spacing);
    void drawAxes(const glm::mat4& vp, float length);

private:
    GLuint compileShader(GLenum type, const char* src);
    GLuint linkProgram(GLuint vs, GLuint fs);

    // Grid VAO/VBO
    GLuint gridVAO = 0, gridVBO = 0;
    int gridVertCount = 0;

    // Axes VAO/VBO
    GLuint axesVAO = 0, axesVBO = 0;
    int axesVertCount = 0;

    void buildGrid(int size, float spacing);
    void buildAxes(float length);
};
