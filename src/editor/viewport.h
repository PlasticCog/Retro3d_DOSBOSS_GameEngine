#pragma once
#include <GL/glew.h>

// Offscreen framebuffer that renders a 3D scene to a texture
// for display inside an ImGui window.
class Viewport {
public:
    bool init(int width, int height);
    void resize(int width, int height);
    void shutdown();

    void begin();  // bind FBO and set viewport
    void end();    // unbind FBO (restore default framebuffer)

    GLuint getTexture() const;

    int getWidth()  const { return width; }
    int getHeight() const { return height; }

private:
    GLuint fbo      = 0;
    GLuint colorTex = 0;
    GLuint depthRbo = 0;
    int width  = 800;
    int height = 600;

    void createBuffers();
    void destroyBuffers();
};
