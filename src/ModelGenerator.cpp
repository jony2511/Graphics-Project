#include "ModelGenerator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Mesh ModelGenerator::createCube(float width, float height, float depth, const glm::vec3& color) {
    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addFace = [&](glm::vec3 n, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        unsigned int base = (unsigned int)vertices.size();
        vertices.push_back({p0, n, color, {0.0f, 0.0f}});
        vertices.push_back({p1, n, color, {1.0f, 0.0f}});
        vertices.push_back({p2, n, color, {1.0f, 1.0f}});
        vertices.push_back({p3, n, color, {0.0f, 1.0f}});
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    };

    // Front, Back, Top, Bottom, Right, Left
    addFace({0, 0, 1}, {-w, -h,  d}, { w, -h,  d}, { w,  h,  d}, {-w,  h,  d});
    addFace({0, 0, -1}, { w, -h, -d}, {-w, -h, -d}, {-w,  h, -d}, { w,  h, -d});
    addFace({0, 1, 0}, {-w,  h,  d}, { w,  h,  d}, { w,  h, -d}, {-w,  h, -d});
    addFace({0, -1, 0}, {-w, -h, -d}, { w, -h, -d}, { w, -h,  d}, {-w, -h,  d});
    addFace({1, 0, 0}, { w, -h,  d}, { w, -h, -d}, { w,  h, -d}, { w,  h,  d});
    addFace({-1, 0, 0}, {-w, -h, -d}, {-w, -h,  d}, {-w,  h,  d}, {-w,  h, -d});

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createSphere(float radius, int rings, int sectors, const glm::vec3& color, bool verticalStripes) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    glm::vec3 stripeColors[4] = {
        glm::vec3(0.92f, 0.15f, 0.15f), // Red
        glm::vec3(0.98f, 0.82f, 0.12f), // Yellow
        glm::vec3(0.12f, 0.45f, 0.90f), // Blue
        glm::vec3(0.95f, 0.95f, 0.95f)  // White
    };

    for (int r = 0; r < rings; ++r) {
        float phi = (float)M_PI * (float)r * R;
        float y = std::cos(phi);
        float sinPhi = std::sin(phi);

        for (int s = 0; s < sectors; ++s) {
            float theta = 2.0f * (float)M_PI * (float)s * S;
            float x = std::cos(theta) * sinPhi;
            float z = std::sin(theta) * sinPhi;

            glm::vec3 norm(x, y, z);
            glm::vec3 pos = norm * radius;

            glm::vec3 vertColor = color;
            if (verticalStripes) {
                int stripeIndex = (s / 3) % 4;
                vertColor = stripeColors[stripeIndex];
            }

            vertices.push_back({pos, norm, vertColor, {(float)s * S, (float)r * R}});
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            int cur = r * sectors + s;
            int next = cur + sectors;

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(next + 1);

            indices.push_back(cur);
            indices.push_back(next + 1);
            indices.push_back(cur + 1);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createCylinder(float bottomRadius, float topRadius, float height, int sectors, const glm::vec3& color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfH = height * 0.5f;

    for (int i = 0; i <= sectors; ++i) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float c = std::cos(angle);
        float s = std::sin(angle);

        glm::vec3 norm(c, 0.0f, s);

        // Bottom ring vertex
        glm::vec3 pBot(c * bottomRadius, -halfH, s * bottomRadius);
        vertices.push_back({pBot, norm, color, {(float)i / (float)sectors, 0.0f}});

        // Top ring vertex
        glm::vec3 pTop(c * topRadius, halfH, s * topRadius);
        vertices.push_back({pTop, norm, color, {(float)i / (float)sectors, 1.0f}});
    }

    for (int i = 0; i < sectors; ++i) {
        unsigned int b1 = i * 2;
        unsigned int t1 = b1 + 1;
        unsigned int b2 = (i + 1) * 2;
        unsigned int t2 = b2 + 1;

        indices.push_back(b1);
        indices.push_back(b2);
        indices.push_back(t1);

        indices.push_back(b2);
        indices.push_back(t2);
        indices.push_back(t1);
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createCone(float radius, float height, int sectors, const glm::vec3& color) {
    return createCylinder(radius, 0.01f, height, sectors, color);
}

Mesh ModelGenerator::createPlane(float width, float depth, int subdivisions, const glm::vec3& color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;
    float stepX = width / (float)subdivisions;
    float stepZ = depth / (float)subdivisions;

    for (int z = 0; z <= subdivisions; ++z) {
        float posZ = -halfD + z * stepZ;
        for (int x = 0; x <= subdivisions; ++x) {
            float posX = -halfW + x * stepX;
            glm::vec3 pos(posX, 0.0f, posZ);
            glm::vec3 norm(0.0f, 1.0f, 0.0f);

            // Subtle color nuance across the meadow grid
            float checker = ((x + z) % 2 == 0) ? 1.0f : 0.94f;
            glm::vec3 vertColor = color * checker;

            vertices.push_back({pos, norm, vertColor, {(float)x / subdivisions, (float)z / subdivisions}});
        }
    }

    int stride = subdivisions + 1;
    for (int z = 0; z < subdivisions; ++z) {
        for (int x = 0; x < subdivisions; ++x) {
            unsigned int topLeft = z * stride + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * stride + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createBalloonEnvelope(float radius, float height, int rings, int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 stripeColors[6] = {
        glm::vec3(0.92f, 0.18f, 0.18f), // Crimson
        glm::vec3(0.98f, 0.85f, 0.15f), // Gold
        glm::vec3(0.15f, 0.55f, 0.95f), // Royal Azure
        glm::vec3(0.96f, 0.96f, 0.96f), // White
        glm::vec3(0.18f, 0.78f, 0.35f), // Emerald
        glm::vec3(0.95f, 0.45f, 0.12f)  // Orange
    };

    for (int r = 0; r < rings; ++r) {
        float v = (float)r / (float)(rings - 1); // 0 (top) to 1 (bottom throat)
        float angle = v * (float)M_PI;

        // Parametric balloon profile: bulbous sphere on top, tapered cone/throat on bottom
        float rScale;
        float y;

        if (v < 0.65f) {
            // Upper bulbous dome
            float localV = v / 0.65f;
            float phi = localV * 0.5f * (float)M_PI;
            y = (1.0f - std::sin(phi)) * (height * 0.45f);
            rScale = std::cos(phi) * radius;
        } else {
            // Lower tapering cone to throat
            float localV = (v - 0.65f) / 0.35f;
            y = -localV * (height * 0.55f);
            rScale = (1.0f - localV * 0.70f) * radius;
        }

        for (int s = 0; s < sectors; ++s) {
            float u = (float)s / (float)sectors;
            float theta = u * 2.0f * (float)M_PI;

            float x = std::cos(theta) * rScale;
            float z = std::sin(theta) * rScale;

            glm::vec3 pos(x, y, z);
            glm::vec3 norm = glm::normalize(glm::vec3(x, y * 0.5f, z));

            int stripeIndex = (s / 4) % 6;
            glm::vec3 vertColor = stripeColors[stripeIndex];

            // Gold accent band at the throat
            if (v > 0.90f) {
                vertColor = glm::vec3(0.85f, 0.70f, 0.15f);
            }

            vertices.push_back({pos, norm, vertColor, {u, v}});
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors; ++s) {
            int nextS = (s + 1) % sectors;
            unsigned int cur = r * sectors + s;
            unsigned int right = r * sectors + nextS;
            unsigned int next = (r + 1) * sectors + s;
            unsigned int nextRight = (r + 1) * sectors + nextS;

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(nextRight);

            indices.push_back(cur);
            indices.push_back(nextRight);
            indices.push_back(right);
        }
    }

    return Mesh(vertices, indices);
}
