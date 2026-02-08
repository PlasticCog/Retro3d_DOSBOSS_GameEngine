#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "core/types.h"

// A mesh stored on the GPU
struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
    int vertexCount = 0;

    // CPU-side data for vertex editing
    std::vector<float> positions;  // x,y,z per vertex
    std::vector<float> normals;    // x,y,z per vertex
    std::vector<unsigned int> indices;

    void upload();
    void destroy();
    void recomputeNormals();
};

// Generate primitive meshes
Mesh generateBox();
Mesh generateCylinder(int segments = 12);
Mesh generateCone(int segments = 12);
Mesh generateSphere(int segH = 12, int segV = 8);
Mesh generateWedge();
Mesh generatePlane();
Mesh generatePrimitive(PrimType type);

// Apply custom vertices to an existing mesh
void applyCustomVerts(Mesh& mesh, const std::vector<float>& customVerts);
