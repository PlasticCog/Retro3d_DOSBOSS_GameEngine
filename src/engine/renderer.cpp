#include "engine/renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

static const char* sceneVS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNormal;
out vec3 vWorldPos;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vWorldPos = vec3(uModel * vec4(aPos, 1.0));
}
)";

static const char* sceneFS = R"(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
uniform vec3 uColor;
uniform vec3 uEmissive;
uniform vec3 uLightDir;
uniform vec3 uFogColor;
uniform float uFogDensity;
out vec4 FragColor;
void main() {
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    vec3 ambient = vec3(0.2, 0.25, 0.35);
    vec3 lit = uColor * (ambient + diff * 0.8) + uEmissive;

    // Exponential fog
    float dist = length(vWorldPos);
    float fog = exp(-uFogDensity * dist);
    lit = mix(uFogColor, lit, clamp(fog, 0.0, 1.0));

    FragColor = vec4(lit, 1.0);
}
)";

static const char* lineVS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* lineFS = R"(
#version 330 core
in vec3 vColor;
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor * uColor, 1.0);
}
)";

GLuint Renderer::compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return s;
}

GLuint Renderer::linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "Program link error: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

bool Renderer::init() {
    // Scene shader
    {
        GLuint vs = compileShader(GL_VERTEX_SHADER, sceneVS);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, sceneFS);
        sceneProgram = linkProgram(vs, fs);
        sceneMVP_loc = glGetUniformLocation(sceneProgram, "uMVP");
        sceneModel_loc = glGetUniformLocation(sceneProgram, "uModel");
        sceneColor_loc = glGetUniformLocation(sceneProgram, "uColor");
        sceneEmissive_loc = glGetUniformLocation(sceneProgram, "uEmissive");
        sceneLightDir_loc = glGetUniformLocation(sceneProgram, "uLightDir");
        sceneFogColor_loc = glGetUniformLocation(sceneProgram, "uFogColor");
        sceneFogDensity_loc = glGetUniformLocation(sceneProgram, "uFogDensity");
    }

    // Line shader
    {
        GLuint vs = compileShader(GL_VERTEX_SHADER, lineVS);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, lineFS);
        lineProgram = linkProgram(vs, fs);
        lineMVP_loc = glGetUniformLocation(lineProgram, "uMVP");
        lineColor_loc = glGetUniformLocation(lineProgram, "uColor");
    }

    buildGrid(40, 1.0f);
    buildAxes(3.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    return true;
}

void Renderer::shutdown() {
    if (sceneProgram) glDeleteProgram(sceneProgram);
    if (lineProgram) glDeleteProgram(lineProgram);
    if (gridVAO) { glDeleteVertexArrays(1, &gridVAO); glDeleteBuffers(1, &gridVBO); }
    if (axesVAO) { glDeleteVertexArrays(1, &axesVAO); glDeleteBuffers(1, &axesVBO); }
}

void Renderer::buildGrid(int size, float spacing) {
    std::vector<float> verts;
    float half = size * spacing * 0.5f;
    for (int i = 0; i <= size; i++) {
        float pos = -half + i * spacing;
        bool major = (i % 5 == 0);
        float bright = major ? 0.15f : 0.06f;
        // Line along X
        verts.insert(verts.end(), {pos, 0, -half, bright*0.4f, bright*0.6f, bright});
        verts.insert(verts.end(), {pos, 0,  half, bright*0.4f, bright*0.6f, bright});
        // Line along Z
        verts.insert(verts.end(), {-half, 0, pos, bright*0.4f, bright*0.6f, bright});
        verts.insert(verts.end(), { half, 0, pos, bright*0.4f, bright*0.6f, bright});
    }
    gridVertCount = (int)verts.size() / 6;

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::buildAxes(float length) {
    float verts[] = {
        // X axis (red)
        0,0,0, 1,0,0,
        length,0,0, 1,0,0,
        // Y axis (green)
        0,0,0, 0,1,0,
        0,length,0, 0,1,0,
        // Z axis (blue)
        0,0,0, 0,0.5f,1,
        0,0,length, 0,0.5f,1,
    };
    axesVertCount = 6;

    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);
    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::drawGrid(const glm::mat4& vp, int /*size*/, float /*spacing*/) {
    glUseProgram(lineProgram);
    glUniformMatrix4fv(lineMVP_loc, 1, GL_FALSE, glm::value_ptr(vp));
    glUniform3f(lineColor_loc, 1, 1, 1);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertCount);
    glBindVertexArray(0);
}

void Renderer::drawAxes(const glm::mat4& vp, float /*length*/) {
    glUseProgram(lineProgram);
    glUniformMatrix4fv(lineMVP_loc, 1, GL_FALSE, glm::value_ptr(vp));
    glUniform3f(lineColor_loc, 1, 1, 1);
    glLineWidth(2.0f);
    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, axesVertCount);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}
