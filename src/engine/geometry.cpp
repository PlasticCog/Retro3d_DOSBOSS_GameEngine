#include "engine/geometry.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// upload(): create interleaved VBO [pos.x, pos.y, pos.z, norm.x, norm.y, norm.z]
//           and EBO from CPU-side positions, normals, indices.
// ---------------------------------------------------------------------------
void Mesh::upload() {
    vertexCount = (int)(positions.size() / 3);
    indexCount  = (int)indices.size();

    // Build interleaved buffer
    std::vector<float> interleaved;
    interleaved.reserve(vertexCount * 6);
    for (int i = 0; i < vertexCount; i++) {
        interleaved.push_back(positions[i * 3 + 0]);
        interleaved.push_back(positions[i * 3 + 1]);
        interleaved.push_back(positions[i * 3 + 2]);
        interleaved.push_back(normals[i * 3 + 0]);
        interleaved.push_back(normals[i * 3 + 1]);
        interleaved.push_back(normals[i * 3 + 2]);
    }

    // Delete old GPU objects if they exist
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 interleaved.size() * sizeof(float),
                 interleaved.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // location 0 = position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // location 1 = normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// destroy(): release GPU resources
// ---------------------------------------------------------------------------
void Mesh::destroy() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    indexCount = 0;
    vertexCount = 0;
}

// ---------------------------------------------------------------------------
// recomputeNormals(): recalculate flat normals from triangle faces
// ---------------------------------------------------------------------------
void Mesh::recomputeNormals() {
    normals.assign(positions.size(), 0.0f);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        unsigned int i0 = indices[i + 0];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 v0(positions[i0*3], positions[i0*3+1], positions[i0*3+2]);
        glm::vec3 v1(positions[i1*3], positions[i1*3+1], positions[i1*3+2]);
        glm::vec3 v2(positions[i2*3], positions[i2*3+1], positions[i2*3+2]);

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 n = glm::cross(edge1, edge2);

        // Accumulate (area-weighted)
        for (unsigned int idx : {i0, i1, i2}) {
            normals[idx*3 + 0] += n.x;
            normals[idx*3 + 1] += n.y;
            normals[idx*3 + 2] += n.z;
        }
    }

    // Normalize
    int vc = (int)(normals.size() / 3);
    for (int i = 0; i < vc; i++) {
        glm::vec3 n(normals[i*3], normals[i*3+1], normals[i*3+2]);
        float len = glm::length(n);
        if (len > 1e-8f) {
            n /= len;
        } else {
            n = glm::vec3(0, 1, 0);
        }
        normals[i*3 + 0] = n.x;
        normals[i*3 + 1] = n.y;
        normals[i*3 + 2] = n.z;
    }
}

// ---------------------------------------------------------------------------
// Helper: push a vertex (position + normal) and return its index
// ---------------------------------------------------------------------------
static int pushVert(Mesh& m, float px, float py, float pz,
                    float nx, float ny, float nz) {
    int idx = (int)(m.positions.size() / 3);
    m.positions.push_back(px);
    m.positions.push_back(py);
    m.positions.push_back(pz);
    m.normals.push_back(nx);
    m.normals.push_back(ny);
    m.normals.push_back(nz);
    return idx;
}

static void pushTri(Mesh& m, unsigned int a, unsigned int b, unsigned int c) {
    m.indices.push_back(a);
    m.indices.push_back(b);
    m.indices.push_back(c);
}

static void pushQuad(Mesh& m, unsigned int a, unsigned int b,
                     unsigned int c, unsigned int d) {
    pushTri(m, a, b, c);
    pushTri(m, a, c, d);
}

// ===========================================================================
// generateBox(): 1x1x1 centered at origin, 24 vertices (4 per face)
// ===========================================================================
Mesh generateBox() {
    Mesh m;
    float h = 0.5f;

    // Front face (z = +0.5)
    int v0 = pushVert(m, -h, -h,  h,  0,  0,  1);
    int v1 = pushVert(m,  h, -h,  h,  0,  0,  1);
    int v2 = pushVert(m,  h,  h,  h,  0,  0,  1);
    int v3 = pushVert(m, -h,  h,  h,  0,  0,  1);
    pushQuad(m, v0, v1, v2, v3);

    // Back face (z = -0.5)
    v0 = pushVert(m,  h, -h, -h,  0,  0, -1);
    v1 = pushVert(m, -h, -h, -h,  0,  0, -1);
    v2 = pushVert(m, -h,  h, -h,  0,  0, -1);
    v3 = pushVert(m,  h,  h, -h,  0,  0, -1);
    pushQuad(m, v0, v1, v2, v3);

    // Top face (y = +0.5)
    v0 = pushVert(m, -h,  h,  h,  0,  1,  0);
    v1 = pushVert(m,  h,  h,  h,  0,  1,  0);
    v2 = pushVert(m,  h,  h, -h,  0,  1,  0);
    v3 = pushVert(m, -h,  h, -h,  0,  1,  0);
    pushQuad(m, v0, v1, v2, v3);

    // Bottom face (y = -0.5)
    v0 = pushVert(m, -h, -h, -h,  0, -1,  0);
    v1 = pushVert(m,  h, -h, -h,  0, -1,  0);
    v2 = pushVert(m,  h, -h,  h,  0, -1,  0);
    v3 = pushVert(m, -h, -h,  h,  0, -1,  0);
    pushQuad(m, v0, v1, v2, v3);

    // Right face (x = +0.5)
    v0 = pushVert(m,  h, -h,  h,  1,  0,  0);
    v1 = pushVert(m,  h, -h, -h,  1,  0,  0);
    v2 = pushVert(m,  h,  h, -h,  1,  0,  0);
    v3 = pushVert(m,  h,  h,  h,  1,  0,  0);
    pushQuad(m, v0, v1, v2, v3);

    // Left face (x = -0.5)
    v0 = pushVert(m, -h, -h, -h, -1,  0,  0);
    v1 = pushVert(m, -h, -h,  h, -1,  0,  0);
    v2 = pushVert(m, -h,  h,  h, -1,  0,  0);
    v3 = pushVert(m, -h,  h, -h, -1,  0,  0);
    pushQuad(m, v0, v1, v2, v3);

    m.upload();
    return m;
}

// ===========================================================================
// generateCylinder(): radius 0.5, height 1, centered at origin
// ===========================================================================
Mesh generateCylinder(int segments) {
    Mesh m;
    float r = 0.5f;
    float halfH = 0.5f;

    // Side vertices: two rings of duplicated vertices with outward normals
    for (int i = 0; i < segments; i++) {
        float a0 = (float)(2.0 * M_PI * i / segments);
        float a1 = (float)(2.0 * M_PI * ((i + 1) % segments) / segments);

        float c0 = cosf(a0), s0 = sinf(a0);
        float c1 = cosf(a1), s1 = sinf(a1);

        // Each side quad has 4 unique vertices (for flat-ish shading we use
        // per-vertex normals pointing radially outward)
        int v0 = pushVert(m, r*c0, -halfH, r*s0, c0, 0, s0);
        int v1 = pushVert(m, r*c1, -halfH, r*s1, c1, 0, s1);
        int v2 = pushVert(m, r*c1,  halfH, r*s1, c1, 0, s1);
        int v3 = pushVert(m, r*c0,  halfH, r*s0, c0, 0, s0);
        pushQuad(m, v0, v1, v2, v3);
    }

    // Top cap
    int topCenter = pushVert(m, 0, halfH, 0, 0, 1, 0);
    for (int i = 0; i < segments; i++) {
        float a0 = (float)(2.0 * M_PI * i / segments);
        float a1 = (float)(2.0 * M_PI * ((i + 1) % segments) / segments);
        int v0 = pushVert(m, r*cosf(a0), halfH, r*sinf(a0), 0, 1, 0);
        int v1 = pushVert(m, r*cosf(a1), halfH, r*sinf(a1), 0, 1, 0);
        pushTri(m, topCenter, v0, v1);
    }

    // Bottom cap
    int botCenter = pushVert(m, 0, -halfH, 0, 0, -1, 0);
    for (int i = 0; i < segments; i++) {
        float a0 = (float)(2.0 * M_PI * i / segments);
        float a1 = (float)(2.0 * M_PI * ((i + 1) % segments) / segments);
        int v0 = pushVert(m, r*cosf(a0), -halfH, r*sinf(a0), 0, -1, 0);
        int v1 = pushVert(m, r*cosf(a1), -halfH, r*sinf(a1), 0, -1, 0);
        pushTri(m, botCenter, v1, v0); // reversed winding for bottom
    }

    m.upload();
    return m;
}

// ===========================================================================
// generateCone(): radius 0.5, height 1, base at y=-0.5, tip at y=+0.5
// ===========================================================================
Mesh generateCone(int segments) {
    Mesh m;
    float r = 0.5f;
    float halfH = 0.5f;

    // The slope normal y-component: r / height = 0.5/1.0 = 0.5
    // Normalised: ny = r/sqrt(r*r + h*h) where h = full height = 1
    float slopeLen = sqrtf(r * r + 1.0f);
    float ny = r / slopeLen;      // upward component
    float nr = 1.0f / slopeLen;   // radial component

    // Side faces: each triangle from tip to base edge
    for (int i = 0; i < segments; i++) {
        float a0 = (float)(2.0 * M_PI * i / segments);
        float a1 = (float)(2.0 * M_PI * ((i + 1) % segments) / segments);
        float aMid = (a0 + a1) * 0.5f;

        float c0 = cosf(a0), s0 = sinf(a0);
        float c1 = cosf(a1), s1 = sinf(a1);
        float cm = cosf(aMid), sm = sinf(aMid);

        // Tip vertex gets the mid-face normal
        int vt = pushVert(m, 0, halfH, 0,  nr*cm, ny, nr*sm);
        int v0 = pushVert(m, r*c0, -halfH, r*s0,  nr*c0, ny, nr*s0);
        int v1 = pushVert(m, r*c1, -halfH, r*s1,  nr*c1, ny, nr*s1);
        pushTri(m, vt, v0, v1);
    }

    // Bottom cap
    int botCenter = pushVert(m, 0, -halfH, 0, 0, -1, 0);
    for (int i = 0; i < segments; i++) {
        float a0 = (float)(2.0 * M_PI * i / segments);
        float a1 = (float)(2.0 * M_PI * ((i + 1) % segments) / segments);
        int v0 = pushVert(m, r*cosf(a0), -halfH, r*sinf(a0), 0, -1, 0);
        int v1 = pushVert(m, r*cosf(a1), -halfH, r*sinf(a1), 0, -1, 0);
        pushTri(m, botCenter, v1, v0);
    }

    m.upload();
    return m;
}

// ===========================================================================
// generateSphere(): radius 0.5, UV sphere
// ===========================================================================
Mesh generateSphere(int segH, int segV) {
    Mesh m;
    float r = 0.5f;

    // Generate vertices
    for (int v = 0; v <= segV; v++) {
        float phi = (float)(M_PI * v / segV);
        for (int h = 0; h <= segH; h++) {
            float theta = (float)(2.0 * M_PI * h / segH);

            float nx = sinf(phi) * cosf(theta);
            float ny = cosf(phi);
            float nz = sinf(phi) * sinf(theta);

            pushVert(m, r * nx, r * ny, r * nz, nx, ny, nz);
        }
    }

    // Generate indices
    int cols = segH + 1;
    for (int v = 0; v < segV; v++) {
        for (int h = 0; h < segH; h++) {
            unsigned int cur  = v * cols + h;
            unsigned int next = cur + cols;

            // Skip degenerate triangles at poles
            if (v != 0) {
                pushTri(m, cur, next, cur + 1);
            }
            if (v != segV - 1) {
                pushTri(m, cur + 1, next, next + 1);
            }
        }
    }

    m.upload();
    return m;
}

// ===========================================================================
// generateWedge(): triangular prism - like a box cut diagonally.
//   Base is a 1x1 square at y=-0.5, top edge at y=+0.5 along z.
//   Think of it as a ramp: front and back are triangles.
// ===========================================================================
Mesh generateWedge() {
    Mesh m;
    float h = 0.5f;

    // Wedge vertices:
    //   Bottom-front-left  (-0.5, -0.5,  0.5)  A
    //   Bottom-front-right ( 0.5, -0.5,  0.5)  B
    //   Bottom-back-left   (-0.5, -0.5, -0.5)  C
    //   Bottom-back-right  ( 0.5, -0.5, -0.5)  D
    //   Top-back-left      (-0.5,  0.5, -0.5)  E
    //   Top-back-right     ( 0.5,  0.5, -0.5)  F
    //
    // The "slope" goes from bottom-front edge to top-back edge.

    // Bottom face (y = -0.5), normal (0, -1, 0)
    int v0 = pushVert(m, -h, -h, -h,  0, -1,  0); // C
    int v1 = pushVert(m,  h, -h, -h,  0, -1,  0); // D
    int v2 = pushVert(m,  h, -h,  h,  0, -1,  0); // B
    int v3 = pushVert(m, -h, -h,  h,  0, -1,  0); // A
    pushQuad(m, v0, v1, v2, v3);

    // Back face (z = -0.5), normal (0, 0, -1)
    v0 = pushVert(m,  h, -h, -h,  0,  0, -1); // D
    v1 = pushVert(m, -h, -h, -h,  0,  0, -1); // C
    v2 = pushVert(m, -h,  h, -h,  0,  0, -1); // E
    v3 = pushVert(m,  h,  h, -h,  0,  0, -1); // F
    pushQuad(m, v0, v1, v2, v3);

    // Slope face: from front-bottom edge to top-back edge
    // Normal points outward from slope: (0, 1, 1) normalised
    float sn = 1.0f / sqrtf(2.0f);
    v0 = pushVert(m, -h, -h,  h,  0, sn, sn); // A
    v1 = pushVert(m,  h, -h,  h,  0, sn, sn); // B
    v2 = pushVert(m,  h,  h, -h,  0, sn, sn); // F
    v3 = pushVert(m, -h,  h, -h,  0, sn, sn); // E
    pushQuad(m, v0, v1, v2, v3);

    // Left face (x = -0.5): triangle A, C, E
    v0 = pushVert(m, -h, -h,  h, -1, 0, 0); // A
    v1 = pushVert(m, -h,  h, -h, -1, 0, 0); // E
    v2 = pushVert(m, -h, -h, -h, -1, 0, 0); // C
    pushTri(m, v0, v1, v2);

    // Right face (x = +0.5): triangle B, D, F
    v0 = pushVert(m,  h, -h,  h,  1, 0, 0); // B
    v1 = pushVert(m,  h, -h, -h,  1, 0, 0); // D
    v2 = pushVert(m,  h,  h, -h,  1, 0, 0); // F
    pushTri(m, v0, v1, v2);

    m.upload();
    return m;
}

// ===========================================================================
// generatePlane(): 1x1 plane facing up at y=0
// ===========================================================================
Mesh generatePlane() {
    Mesh m;
    float h = 0.5f;

    int v0 = pushVert(m, -h, 0,  h,  0, 1, 0);
    int v1 = pushVert(m,  h, 0,  h,  0, 1, 0);
    int v2 = pushVert(m,  h, 0, -h,  0, 1, 0);
    int v3 = pushVert(m, -h, 0, -h,  0, 1, 0);
    pushQuad(m, v0, v1, v2, v3);

    // Back face so it's visible from below too
    int v4 = pushVert(m, -h, 0, -h,  0, -1, 0);
    int v5 = pushVert(m,  h, 0, -h,  0, -1, 0);
    int v6 = pushVert(m,  h, 0,  h,  0, -1, 0);
    int v7 = pushVert(m, -h, 0,  h,  0, -1, 0);
    pushQuad(m, v4, v5, v6, v7);

    m.upload();
    return m;
}

// ===========================================================================
// generatePrimitive(): dispatch by PrimType
// ===========================================================================
Mesh generatePrimitive(PrimType type) {
    switch (type) {
        case PrimType::Box:      return generateBox();
        case PrimType::Cylinder: return generateCylinder();
        case PrimType::Cone:     return generateCone();
        case PrimType::Sphere:   return generateSphere();
        case PrimType::Wedge:    return generateWedge();
        case PrimType::Plane:    return generatePlane();
        default:                 return generateBox();
    }
}

// ===========================================================================
// applyCustomVerts(): replace positions from a flat float array,
//                     recompute normals, re-upload
// ===========================================================================
void applyCustomVerts(Mesh& mesh, const std::vector<float>& customVerts) {
    if (customVerts.empty()) return;

    // customVerts is expected to be x,y,z per vertex
    int newVertCount = (int)(customVerts.size() / 3);
    if (newVertCount != mesh.vertexCount) {
        std::cerr << "applyCustomVerts: vertex count mismatch ("
                  << newVertCount << " vs " << mesh.vertexCount << ")\n";
        return;
    }

    mesh.positions = customVerts;
    mesh.recomputeNormals();
    mesh.upload();
}
