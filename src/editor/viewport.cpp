#include "editor/viewport.h"
#include <iostream>

// -----------------------------------------------------------------------
// Public interface
// -----------------------------------------------------------------------

bool Viewport::init(int w, int h) {
    width  = w;
    height = h;
    createBuffers();
    return fbo != 0;
}

void Viewport::resize(int w, int h) {
    if (w <= 0 || h <= 0) return;
    if (w == width && h == height) return;

    width  = w;
    height = h;

    destroyBuffers();
    createBuffers();
}

void Viewport::shutdown() {
    destroyBuffers();
}

void Viewport::begin() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void Viewport::end() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Viewport::getTexture() const {
    return colorTex;
}

// -----------------------------------------------------------------------
// Internal
// -----------------------------------------------------------------------

void Viewport::createBuffers() {
    // Color texture
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth renderbuffer
    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Viewport FBO incomplete: 0x" << std::hex << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Viewport::destroyBuffers() {
    if (fbo)      { glDeleteFramebuffers(1, &fbo);      fbo      = 0; }
    if (colorTex) { glDeleteTextures(1, &colorTex);     colorTex = 0; }
    if (depthRbo) { glDeleteRenderbuffers(1, &depthRbo); depthRbo = 0; }
}
