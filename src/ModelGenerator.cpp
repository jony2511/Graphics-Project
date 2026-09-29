#include "ModelGenerator.h"
#include <cmath>
#include <algorithm>

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
        float v = (float)r / (float)(rings - 1);
        float rScale;
        float y;

        if (v < 0.65f) {
            float localV = v / 0.65f;
            float phi = localV * 0.5f * (float)M_PI;
            y = (1.0f - std::sin(phi)) * (height * 0.45f);
            rScale = std::cos(phi) * radius;
        } else {
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

// ========================================================
// Phase 2: Rural Landscape & Architecture Generators
// ========================================================

static float getTerrainHeight(float x, float z) {
    float dist = std::sqrt(x * x + z * z);
    // Keep central meadow and village clearing flat
    if (dist < 22.0f) return 0.0f;

    float weight = std::clamp((dist - 22.0f) / 36.0f, 0.0f, 1.0f);
    weight = weight * weight * (3.0f - 2.0f * weight); // smoothstep

    float h1 = 6.5f * std::sin(x * 0.038f) * std::cos(z * 0.042f);
    float h2 = 4.0f * std::sin(x * 0.072f + 1.2f) * std::sin(z * 0.065f - 0.7f);
    float h3 = 3.5f * std::cos(dist * 0.035f);

    return weight * (h1 + h2 + h3 + 1.8f);
}

Mesh ModelGenerator::createRollingTerrain(float width, float depth, int subdivisions) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;
    float stepX = width / (float)subdivisions;
    float stepZ = depth / (float)subdivisions;

    const float eps = 0.5f;

    for (int z = 0; z <= subdivisions; ++z) {
        float posZ = -halfD + z * stepZ;
        for (int x = 0; x <= subdivisions; ++x) {
            float posX = -halfW + x * stepX;
            float posY = getTerrainHeight(posX, posZ);

            // Compute surface normal via finite difference
            float hL = getTerrainHeight(posX - eps, posZ);
            float hR = getTerrainHeight(posX + eps, posZ);
            float hD = getTerrainHeight(posX, posZ - eps);
            float hU = getTerrainHeight(posX, posZ + eps);

            glm::vec3 norm = glm::normalize(glm::vec3((hL - hR) / (2.0f * eps), 1.0f, (hD - hU) / (2.0f * eps)));

            // Color gradient: lush valley green to warm sunny slope green
            float heightRatio = std::clamp(posY / 10.0f, 0.0f, 1.0f);
            glm::vec3 valleyGreen(0.28f, 0.58f, 0.22f);
            glm::vec3 hillGreen(0.36f, 0.62f, 0.25f);
            float subtleChecker = ((x + z) % 2 == 0) ? 1.0f : 0.94f;
            glm::vec3 vertColor = glm::mix(valleyGreen, hillGreen, heightRatio) * subtleChecker;

            vertices.push_back({{posX, posY, posZ}, norm, vertColor, {(float)x / subdivisions, (float)z / subdivisions}});
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

Mesh ModelGenerator::createPrism(float width, float height, float depth, const glm::vec3& color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float w = width * 0.5f;
    float d = depth * 0.5f;

    // Triangular Gable Front (Z = +d)
    glm::vec3 pFrontL(-w, 0.0f,  d);
    glm::vec3 pFrontR( w, 0.0f,  d);
    glm::vec3 pFrontTop(0.0f, height, d);
    glm::vec3 nFront(0.0f, 0.0f, 1.0f);

    unsigned int b = (unsigned int)vertices.size();
    vertices.push_back({pFrontL, nFront, color * 0.95f, {0, 0}});
    vertices.push_back({pFrontR, nFront, color * 0.95f, {1, 0}});
    vertices.push_back({pFrontTop, nFront, color * 0.95f, {0.5f, 1}});
    indices.push_back(b); indices.push_back(b + 1); indices.push_back(b + 2);

    // Triangular Gable Back (Z = -d)
    glm::vec3 pBackL(-w, 0.0f, -d);
    glm::vec3 pBackR( w, 0.0f, -d);
    glm::vec3 pBackTop(0.0f, height, -d);
    glm::vec3 nBack(0.0f, 0.0f, -1.0f);

    b = (unsigned int)vertices.size();
    vertices.push_back({pBackL, nBack, color * 0.95f, {0, 0}});
    vertices.push_back({pBackTop, nBack, color * 0.95f, {0.5f, 1}});
    vertices.push_back({pBackR, nBack, color * 0.95f, {1, 0}});
    indices.push_back(b); indices.push_back(b + 1); indices.push_back(b + 2);

    // Left Roof Slope
    glm::vec3 nLeft = glm::normalize(glm::vec3(-height, w, 0.0f));
    b = (unsigned int)vertices.size();
    vertices.push_back({pFrontL, nLeft, color, {0, 0}});
    vertices.push_back({pFrontTop, nLeft, color, {0, 1}});
    vertices.push_back({pBackTop, nLeft, color, {1, 1}});
    vertices.push_back({pBackL, nLeft, color, {1, 0}});
    indices.push_back(b); indices.push_back(b + 1); indices.push_back(b + 2);
    indices.push_back(b); indices.push_back(b + 2); indices.push_back(b + 3);

    // Right Roof Slope
    glm::vec3 nRight = glm::normalize(glm::vec3(height, w, 0.0f));
    b = (unsigned int)vertices.size();
    vertices.push_back({pFrontR, nRight, color * 1.05f, {0, 0}});
    vertices.push_back({pBackR, nRight, color * 1.05f, {1, 0}});
    vertices.push_back({pBackTop, nRight, color * 1.05f, {1, 1}});
    vertices.push_back({pFrontTop, nRight, color * 1.05f, {0, 1}});
    indices.push_back(b); indices.push_back(b + 1); indices.push_back(b + 2);
    indices.push_back(b); indices.push_back(b + 2); indices.push_back(b + 3);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createStripedWindsock(float baseRadius, float tipRadius, float length, int sectors, int numStripes) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 red(0.95f, 0.22f, 0.12f);
    glm::vec3 white(0.96f, 0.96f, 0.96f);

    int numRings = numStripes * 2;
    for (int r = 0; r <= numRings; ++r) {
        float frac = (float)r / (float)numRings;
        float currentRadius = glm::mix(baseRadius, tipRadius, frac);
        float currentZ = frac * length;

        int stripeIndex = (r * numStripes) / (numRings + 1);
        glm::vec3 bandColor = (stripeIndex % 2 == 0) ? red : white;

        for (int s = 0; s <= sectors; ++s) {
            float theta = 2.0f * (float)M_PI * (float)s / (float)sectors;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            glm::vec3 pos(cosT * currentRadius, sinT * currentRadius, currentZ);
            glm::vec3 norm(cosT, sinT, 0.0f);

            vertices.push_back({pos, norm, bandColor, {(float)s / sectors, frac}});
        }
    }

    int stride = sectors + 1;
    for (int r = 0; r < numRings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            unsigned int c1 = r * stride + s;
            unsigned int c2 = (r + 1) * stride + s;
            unsigned int c3 = c1 + 1;
            unsigned int c4 = c2 + 1;

            indices.push_back(c1); indices.push_back(c2); indices.push_back(c3);
            indices.push_back(c3); indices.push_back(c2); indices.push_back(c4);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createWindmillBlade(float length, float width, const glm::vec3& woodColor, const glm::vec3& sailColor) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float sparThick = width * 0.15f;

    // 1. Central wooden spar along +Y axis
    auto addBox = [&](glm::vec3 pMin, glm::vec3 pMax, glm::vec3 col) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n(0, 0, 1);
        // Front quad
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Back quad
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, -1);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col, {0, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col, {0, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col, {1, 1}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col, {1, 0}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    };

    // Central spar beam
    addBox({-sparThick * 0.5f, 0.0f, -sparThick * 0.5f},
           { sparThick * 0.5f, length,  sparThick * 0.5f}, woodColor);

    // Canvas lattice sail cloth attached along the spar
    unsigned int b = (unsigned int)vertices.size();
    glm::vec3 norm(0.15f, 0.0f, 0.98f);
    norm = glm::normalize(norm);
    float sailStart = length * 0.22f;
    vertices.push_back({{0.0f, sailStart, 0.02f}, norm, sailColor, {0, 0}});
    vertices.push_back({{width, sailStart, 0.02f}, norm, sailColor, {1, 0}});
    vertices.push_back({{width, length, 0.02f}, norm, sailColor, {1, 1}});
    vertices.push_back({{0.0f, length, 0.02f}, norm, sailColor, {0, 1}});
    indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
    indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

    // Two-sided sail cloth
    b = (unsigned int)vertices.size();
    norm = -norm;
    vertices.push_back({{0.0f, sailStart, -0.02f}, norm, sailColor * 0.92f, {0, 0}});
    vertices.push_back({{0.0f, length, -0.02f}, norm, sailColor * 0.92f, {0, 1}});
    vertices.push_back({{width, length, -0.02f}, norm, sailColor * 0.92f, {1, 1}});
    vertices.push_back({{width, sailStart, -0.02f}, norm, sailColor * 0.92f, {1, 0}});
    indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
    indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createCurvedDirtRoad() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int numSteps = 40;
    const float roadWidth = 4.8f;
    glm::vec3 roadColorCenter(0.62f, 0.50f, 0.35f); // Warm sandstone dirt
    glm::vec3 roadColorEdge(0.48f, 0.38f, 0.25f);   // Darker roadside earth

    for (int i = 0; i <= numSteps; ++i) {
        float t = (float)i / (float)numSteps;
        // S-curve parametric spline from launchpad (0, 0, 8) into the rural meadow (28, 0, 85)
        float z = 8.0f + t * 78.0f;
        float x = 24.0f * std::sin(t * (float)M_PI * 1.15f) + 4.0f * std::sin(t * 4.0f);
        float y = 0.06f; // Elevated slightly above terrain to prevent z-fighting

        // Tangent & Normal
        float dz = 78.0f;
        float dx = 24.0f * (float)M_PI * 1.15f * std::cos(t * (float)M_PI * 1.15f) + 16.0f * std::cos(t * 4.0f);
        glm::vec3 tangent = glm::normalize(glm::vec3(dx, 0.0f, dz));
        glm::vec3 side = glm::normalize(glm::vec3(-tangent.z, 0.0f, tangent.x));
        glm::vec3 norm(0.0f, 1.0f, 0.0f);

        glm::vec3 pLeft = glm::vec3(x, y, z) - side * (roadWidth * 0.5f);
        glm::vec3 pCenter = glm::vec3(x, y + 0.015f, z);
        glm::vec3 pRight = glm::vec3(x, y, z) + side * (roadWidth * 0.5f);

        vertices.push_back({pLeft, norm, roadColorEdge, {0.0f, t}});
        vertices.push_back({pCenter, norm, roadColorCenter, {0.5f, t}});
        vertices.push_back({pRight, norm, roadColorEdge, {1.0f, t}});
    }

    for (int i = 0; i < numSteps; ++i) {
        unsigned int row1 = i * 3;
        unsigned int row2 = (i + 1) * 3;

        // Left quad
        indices.push_back(row1);
        indices.push_back(row2);
        indices.push_back(row1 + 1);

        indices.push_back(row1 + 1);
        indices.push_back(row2);
        indices.push_back(row2 + 1);

        // Right quad
        indices.push_back(row1 + 1);
        indices.push_back(row2 + 1);
        indices.push_back(row1 + 2);

        indices.push_back(row1 + 2);
        indices.push_back(row2 + 1);
        indices.push_back(row2 + 2);
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createHayBale(float radius, float length) {
    // Horizontally oriented cylindrical hay bale
    Mesh cylinder = createCylinder(radius, radius, length, 14, glm::vec3(0.85f, 0.74f, 0.32f));
    return cylinder;
}

// ========================================================
// Phase 3: High-Fidelity Eye-Catching Hot Air Balloon Rig
// ========================================================

Mesh ModelGenerator::createRainbowBalloonEnvelope(float radius, float height, int rings, int numGores, int colorScheme) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 7-color Rainbow spectrum (matching user's reference image)
    const glm::vec3 rainbowColors[7] = {
        glm::vec3(0.94f, 0.20f, 0.18f), // 1. Crimson Red
        glm::vec3(0.98f, 0.52f, 0.12f), // 2. Vivid Orange
        glm::vec3(0.98f, 0.86f, 0.08f), // 3. Sunny Yellow
        glm::vec3(0.16f, 0.78f, 0.32f), // 4. Fresh Green
        glm::vec3(0.12f, 0.66f, 0.94f), // 5. Sky Cyan
        glm::vec3(0.14f, 0.36f, 0.88f), // 6. Royal Blue
        glm::vec3(0.52f, 0.18f, 0.70f)  // 7. Violet Purple
    };

    // Sunset Fire (Background Balloon 1)
    const glm::vec3 sunsetColors[3] = {
        glm::vec3(0.95f, 0.28f, 0.22f), // Coral Red
        glm::vec3(0.98f, 0.78f, 0.15f), // Goldenrod
        glm::vec3(0.96f, 0.96f, 0.96f)  // Pure White
    };

    // Ocean Teal (Background Balloon 2)
    const glm::vec3 oceanColors[3] = {
        glm::vec3(0.12f, 0.65f, 0.72f), // Teal Cyan
        glm::vec3(0.10f, 0.25f, 0.68f), // Deep Navy
        glm::vec3(0.96f, 0.96f, 0.96f)  // Clean White
    };

    const int sectorsPerGore = 4;
    const int totalSectors = numGores * sectorsPerGore;

    for (int r = 0; r < rings; ++r) {
        float v = (float)r / (float)(rings - 1); // 0 (top pole) to 1 (bottom throat)

        float rBase;
        float y;

        if (v < 0.46f) {
            // Upper bulbous dome
            float localV = v / 0.46f;
            float phi = localV * 0.5f * (float)M_PI;
            y = (1.0f - std::sin(phi)) * (height * 0.46f);
            rBase = std::cos(phi) * radius;
        } else {
            // Lower tapering cone towards the throat
            float localV = (v - 0.46f) / 0.54f;
            y = -localV * (height * 0.54f);
            rBase = radius * (1.0f - 0.70f * std::sqrt(localV));
        }

        for (int s = 0; s < totalSectors; ++s) {
            int goreIndex = s / sectorsPerGore;
            int goreSector = s % sectorsPerGore;
            float goreFrac = (float)goreSector / (float)sectorsPerGore;

            // 3D Puffy gore bulge between vertical load tapes
            float bulge = 1.0f + 0.042f * std::sin(goreFrac * (float)M_PI);
            float currentRadius = rBase * bulge;

            float u = (float)s / (float)totalSectors;
            float theta = u * 2.0f * (float)M_PI;

            float x = std::cos(theta) * currentRadius;
            float z = std::sin(theta) * currentRadius;

            glm::vec3 pos(x, y, z);
            glm::vec3 norm = glm::normalize(glm::vec3(x, y * 0.45f, z));

            glm::vec3 vertColor;
            if (colorScheme == 0) {
                vertColor = rainbowColors[goreIndex % 7];
            } else if (colorScheme == 1) {
                vertColor = sunsetColors[goreIndex % 3];
            } else {
                vertColor = oceanColors[goreIndex % 3];
            }

            // Subtle dark groove between gores for realistic load-tape seam lines
            if (goreFrac < 0.08f || goreFrac > 0.92f) {
                vertColor *= 0.82f;
            }

            // Top crown cap
            if (v < 0.035f) {
                vertColor = glm::vec3(0.24f, 0.24f, 0.26f);
            }

            vertices.push_back({pos, norm, vertColor, {u, v}});
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < totalSectors; ++s) {
            int nextS = (s + 1) % totalSectors;
            unsigned int cur = r * totalSectors + s;
            unsigned int right = r * totalSectors + nextS;
            unsigned int next = (r + 1) * totalSectors + s;
            unsigned int nextRight = (r + 1) * totalSectors + nextS;

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

Mesh ModelGenerator::createBalloonEquatorBelt(float radius, float thickness) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int sectors = 64;
    const float beltHeight = 0.32f;
    const float beltThickness = thickness;
    const glm::vec3 white(0.98f, 0.98f, 0.98f);
    const glm::vec3 darkRim(0.25f, 0.25f, 0.28f);

    float halfH = beltHeight * 0.5f;

    // Outer cylindrical belt ring
    for (int i = 0; i <= sectors; ++i) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float c = std::cos(angle);
        float s = std::sin(angle);

        glm::vec3 norm(c, 0.0f, s);
        float rOut = radius + beltThickness;

        vertices.push_back({{c * rOut, -halfH, s * rOut}, norm, white, {(float)i / sectors, 0.0f}});
        vertices.push_back({{c * rOut,  halfH, s * rOut}, norm, white, {(float)i / sectors, 1.0f}});
    }

    for (int i = 0; i < sectors; ++i) {
        unsigned int b1 = i * 2;
        unsigned int t1 = b1 + 1;
        unsigned int b2 = (i + 1) * 2;
        unsigned int t2 = b2 + 1;

        indices.push_back(b1); indices.push_back(b2); indices.push_back(t1);
        indices.push_back(b2); indices.push_back(t2); indices.push_back(t1);
    }

    // Scalloped swags (draped decorative curves below the belt)
    const int numGores = 14;
    for (int g = 0; g < numGores; ++g) {
        float startAngle = 2.0f * (float)M_PI * (float)g / (float)numGores;
        float endAngle = 2.0f * (float)M_PI * (float)(g + 1) / (float)numGores;

        const int drapeSteps = 6;
        for (int step = 0; step < drapeSteps; ++step) {
            float t1 = (float)step / (float)drapeSteps;
            float t2 = (float)(step + 1) / (float)drapeSteps;

            float a1 = glm::mix(startAngle, endAngle, t1);
            float a2 = glm::mix(startAngle, endAngle, t2);

            // Catenary sag curve
            float sag1 = std::sin(t1 * (float)M_PI) * 0.38f;
            float sag2 = std::sin(t2 * (float)M_PI) * 0.38f;

            float rDrape = radius + 0.02f;
            glm::vec3 p1(std::cos(a1) * rDrape, -halfH - sag1, std::sin(a1) * rDrape);
            glm::vec3 p2(std::cos(a2) * rDrape, -halfH - sag2, std::sin(a2) * rDrape);

            unsigned int b = (unsigned int)vertices.size();
            glm::vec3 n(std::cos(a1), 0.0f, std::sin(a1));
            float thick = 0.04f;

            vertices.push_back({p1, n, darkRim, {0, 0}});
            vertices.push_back({p2, n, darkRim, {1, 0}});
            vertices.push_back({p2 - glm::vec3(0, thick, 0), n, darkRim, {1, 1}});
            vertices.push_back({p1 - glm::vec3(0, thick, 0), n, darkRim, {0, 1}});

            indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
            indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createBalloonWhiteSkirt(float topRadius, float botRadius, float height, int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const glm::vec3 white(0.96f, 0.96f, 0.98f);
    const glm::vec3 darkRim(0.20f, 0.20f, 0.22f);

    float halfH = height * 0.5f;

    for (int i = 0; i <= sectors; ++i) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float c = std::cos(angle);
        float s = std::sin(angle);

        glm::vec3 norm(c, 0.1f, s);
        norm = glm::normalize(norm);

        // Bottom vertex with dark trim
        vertices.push_back({{c * botRadius, -halfH, s * botRadius}, norm, darkRim, {(float)i / sectors, 0.0f}});
        // Mid vertex
        vertices.push_back({{c * (botRadius * 0.8f + topRadius * 0.2f), -halfH + height * 0.15f, s * (botRadius * 0.8f + topRadius * 0.2f)}, norm, white, {(float)i / sectors, 0.15f}});
        // Top vertex
        vertices.push_back({{c * topRadius, halfH, s * topRadius}, norm, white, {(float)i / sectors, 1.0f}});
    }

    for (int i = 0; i < sectors; ++i) {
        unsigned int b1 = i * 3;
        unsigned int m1 = b1 + 1;
        unsigned int t1 = b1 + 2;

        unsigned int b2 = (i + 1) * 3;
        unsigned int m2 = b2 + 1;
        unsigned int t2 = b2 + 2;

        // Bottom dark trim quad
        indices.push_back(b1); indices.push_back(b2); indices.push_back(m1);
        indices.push_back(m1); indices.push_back(b2); indices.push_back(m2);

        // Main white collar quad
        indices.push_back(m1); indices.push_back(m2); indices.push_back(t1);
        indices.push_back(t1); indices.push_back(m2); indices.push_back(t2);
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createWovenBasket(float width, float height, float depth) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    glm::vec3 wickerLight(0.72f, 0.48f, 0.28f);
    glm::vec3 wickerDark(0.58f, 0.38f, 0.20f);
    glm::vec3 woodPost(0.42f, 0.25f, 0.14f);
    glm::vec3 rimRail(0.36f, 0.20f, 0.10f);

    const int numBands = 5;
    float bandH = height / (float)numBands;

    auto addQuad = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 norm, glm::vec3 col) {
        unsigned int base = (unsigned int)vertices.size();
        vertices.push_back({p0, norm, col, {0, 0}});
        vertices.push_back({p1, norm, col, {1, 0}});
        vertices.push_back({p2, norm, col, {1, 1}});
        vertices.push_back({p3, norm, col, {0, 1}});
        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
    };

    // 1. Horizontal woven bands for 4 walls
    for (int b = 0; b < numBands; ++b) {
        float y0 = -h + b * bandH;
        float y1 = y0 + bandH;
        glm::vec3 col = (b % 2 == 0) ? wickerLight : wickerDark;

        // Front (Z = +d)
        addQuad({-w, y0, d}, { w, y0, d}, { w, y1, d}, {-w, y1, d}, {0, 0, 1}, col);
        // Back (Z = -d)
        addQuad({ w, y0, -d}, {-w, y0, -d}, {-w, y1, -d}, { w, y1, -d}, {0, 0, -1}, col);
        // Right (X = +w)
        addQuad({ w, y0, d}, { w, y0, -d}, { w, y1, -d}, { w, y1, d}, {1, 0, 0}, col);
        // Left (X = -w)
        addQuad({-w, y0, -d}, {-w, y0, d}, {-w, y1, d}, {-w, y1, -d}, {-1, 0, 0}, col);
    }

    // 2. Bottom Floor
    addQuad({-w, -h, -d}, { w, -h, -d}, { w, -h, d}, {-w, -h, d}, {0, 1, 0}, wickerDark * 0.8f);

    // 3. Top Rim Handrail
    float rimThick = 0.14f;
    float rimW = w + 0.08f;
    float rimD = d + 0.08f;

    // Top surface of handrail
    addQuad({-rimW, h, -rimD}, { rimW, h, -rimD}, { rimW, h + rimThick, -rimD}, {-rimW, h + rimThick, -rimD}, {0, 0, -1}, rimRail);
    addQuad({-rimW, h,  rimD}, { rimW, h,  rimD}, { rimW, h + rimThick,  rimD}, {-rimW, h + rimThick,  rimD}, {0, 0, 1}, rimRail);
    addQuad({ rimW, h, -rimD}, { rimW, h,  rimD}, { rimW, h + rimThick,  rimD}, { rimW, h + rimThick, -rimD}, {1, 0, 0}, rimRail);
    addQuad({-rimW, h,  rimD}, {-rimW, h, -rimD}, {-rimW, h + rimThick, -rimD}, {-rimW, h + rimThick,  rimD}, {-1, 0, 0}, rimRail);

    // 4. Vertical Corner Posts
    float postR = 0.08f;
    float corners[4][2] = {{-w, -d}, {w, -d}, {w, d}, {-w, d}};
    for (int i = 0; i < 4; ++i) {
        float cx = corners[i][0];
        float cz = corners[i][1];
        addQuad({cx - postR, -h, cz - postR}, {cx + postR, -h, cz - postR}, {cx + postR, h, cz - postR}, {cx - postR, h, cz - postR}, {0, 0, -1}, woodPost);
        addQuad({cx - postR, -h, cz + postR}, {cx + postR, -h, cz + postR}, {cx + postR, h, cz + postR}, {cx - postR, h, cz + postR}, {0, 0, 1}, woodPost);
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createCloudCluster() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    struct CloudSphere {
        glm::vec3 offset;
        float radius;
        glm::vec3 scale;
    };

    std::vector<CloudSphere> puffs = {
        {{ 0.0f,  0.0f,  0.0f}, 3.6f, {1.0f, 0.62f, 1.0f}},
        {{-3.0f, -0.2f,  0.4f}, 2.6f, {1.0f, 0.58f, 0.95f}},
        {{ 2.8f, -0.3f, -0.3f}, 2.5f, {1.0f, 0.60f, 0.95f}},
        {{ 0.5f, -0.4f,  2.0f}, 2.2f, {1.1f, 0.55f, 0.9f}},
        {{-0.8f, -0.3f, -1.8f}, 2.4f, {1.0f, 0.56f, 1.0f}},
        {{ 1.6f,  0.4f,  0.2f}, 2.0f, {0.9f, 0.65f, 0.9f}}
    };

    glm::vec3 cloudWhite(0.98f, 0.98f, 1.0f);
    const int rings = 12;
    const int sectors = 16;

    for (const auto& p : puffs) {
        unsigned int baseIndex = (unsigned int)vertices.size();

        for (int r = 0; r < rings; ++r) {
            float phi = (float)M_PI * (float)r / (float)(rings - 1);
            float y = std::cos(phi);
            float sinPhi = std::sin(phi);

            for (int s = 0; s < sectors; ++s) {
                float theta = 2.0f * (float)M_PI * (float)s / (float)sectors;
                float x = std::cos(theta) * sinPhi;
                float z = std::sin(theta) * sinPhi;

                glm::vec3 norm = glm::normalize(glm::vec3(x, y * 1.5f, z)); // Upward-biased normals
                glm::vec3 localPos(x * p.radius * p.scale.x, y * p.radius * p.scale.y, z * p.radius * p.scale.z);
                glm::vec3 worldPos = p.offset + localPos;

                vertices.push_back({worldPos, norm, cloudWhite, {(float)s / sectors, (float)r / rings}});
            }
        }

        for (int r = 0; r < rings - 1; ++r) {
            for (int s = 0; s < sectors; ++s) {
                int nextS = (s + 1) % sectors;
                unsigned int cur = baseIndex + r * sectors + s;
                unsigned int right = baseIndex + r * sectors + nextS;
                unsigned int next = baseIndex + (r + 1) * sectors + s;
                unsigned int nextRight = baseIndex + (r + 1) * sectors + nextS;

                indices.push_back(cur);
                indices.push_back(next);
                indices.push_back(nextRight);

                indices.push_back(cur);
                indices.push_back(nextRight);
                indices.push_back(right);
            }
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createBirdBody() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 birdBlack(0.12f, 0.12f, 0.14f);

    // Aerodynamic tapered bird fuselage
    // Head at +Z, Tail at -Z
    glm::vec3 beak(0.0f, 0.0f, 0.8f);
    glm::vec3 headTop(0.0f, 0.15f, 0.4f);
    glm::vec3 headBot(0.0f, -0.12f, 0.4f);
    glm::vec3 headR(0.14f, 0.0f, 0.4f);
    glm::vec3 headL(-0.14f, 0.0f, 0.4f);

    glm::vec3 tailTip(0.0f, 0.05f, -0.9f);
    glm::vec3 tailR(0.18f, 0.02f, -0.85f);
    glm::vec3 tailL(-0.18f, 0.02f, -0.85f);

    auto addTri = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        vertices.push_back({p0, n, birdBlack, {0, 0}});
        vertices.push_back({p1, n, birdBlack, {1, 0}});
        vertices.push_back({p2, n, birdBlack, {0.5f, 1}});
        indices.push_back(b); indices.push_back(b + 1); indices.push_back(b + 2);
    };

    // Head cone
    addTri(beak, headR, headTop);
    addTri(beak, headTop, headL);
    addTri(beak, headL, headBot);
    addTri(beak, headBot, headR);

    // Body to tail
    addTri(headTop, headR, tailR);
    addTri(headTop, tailR, tailTip);
    addTri(headTop, tailTip, tailL);
    addTri(headTop, tailL, headL);

    addTri(headBot, tailR, headR);
    addTri(headBot, tailTip, tailR);
    addTri(headBot, tailL, tailTip);
    addTri(headBot, headL, tailL);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createBirdWing(bool isLeft) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 birdBlack(0.12f, 0.12f, 0.14f);
    float sign = isLeft ? -1.0f : 1.0f;

    // Wing root at shoulder, elbow joint, swept tip
    glm::vec3 root(0.0f, 0.05f, 0.2f);
    glm::vec3 rootRear(0.0f, 0.02f, -0.2f);
    glm::vec3 elbow(sign * 0.8f, 0.18f, 0.05f);
    glm::vec3 elbowRear(sign * 0.75f, 0.12f, -0.25f);
    glm::vec3 tip(sign * 1.6f, 0.25f, -0.35f);

    auto addQuadTwoSided = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n(0, 1, 0);
        vertices.push_back({p0, n, birdBlack, {0, 0}});
        vertices.push_back({p1, n, birdBlack, {1, 0}});
        vertices.push_back({p2, n, birdBlack, {1, 1}});
        vertices.push_back({p3, n, birdBlack, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        b = (unsigned int)vertices.size();
        n = glm::vec3(0, -1, 0);
        vertices.push_back({p0, n, birdBlack, {0, 0}});
        vertices.push_back({p3, n, birdBlack, {0, 1}});
        vertices.push_back({p2, n, birdBlack, {1, 1}});
        vertices.push_back({p1, n, birdBlack, {1, 0}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    };

    // Inner wing section
    addQuadTwoSided(root, elbow, elbowRear, rootRear);

    // Outer wing section
    unsigned int b = (unsigned int)vertices.size();
    glm::vec3 n(0, 1, 0);
    vertices.push_back({elbow, n, birdBlack, {0, 0}});
    vertices.push_back({tip, n, birdBlack, {1, 0}});
    vertices.push_back({elbowRear, n, birdBlack, {0.5f, 1}});
    indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);

    b = (unsigned int)vertices.size();
    n = glm::vec3(0, -1, 0);
    vertices.push_back({elbow, n, birdBlack, {0, 0}});
    vertices.push_back({elbowRear, n, birdBlack, {0.5f, 1}});
    vertices.push_back({tip, n, birdBlack, {1, 0}});
    indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createShadowDisc(float radius, int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 shadowCol(0.06f, 0.09f, 0.05f); // Deep meadow shadow tone
    glm::vec3 norm(0.0f, 1.0f, 0.0f);

    // Center vertex
    vertices.push_back({{0.0f, 0.0f, 0.0f}, norm, shadowCol, {0.5f, 0.5f}});

    // Inner ring (70% radius) for dense core
    float innerR = radius * 0.70f;
    for (int i = 0; i < sectors; ++i) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float x = std::cos(angle) * innerR;
        float z = std::sin(angle) * innerR;
        vertices.push_back({{x, 0.0f, z}, norm, shadowCol * 0.9f, {(x / radius) * 0.5f + 0.5f, (z / radius) * 0.5f + 0.5f}});
    }

    // Outer ring (full radius) for soft feathered boundary
    for (int i = 0; i < sectors; ++i) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        vertices.push_back({{x, 0.0f, z}, norm, shadowCol * 0.4f, {(x / radius) * 0.5f + 0.5f, (z / radius) * 0.5f + 0.5f}});
    }

    // Indices for center to inner ring
    for (int i = 0; i < sectors; ++i) {
        int next = (i + 1) % sectors;
        indices.push_back(0);
        indices.push_back(1 + i);
        indices.push_back(1 + next);
    }

    // Indices for inner ring to outer ring
    int outerOffset = 1 + sectors;
    for (int i = 0; i < sectors; ++i) {
        int next = (i + 1) % sectors;
        unsigned int inCur = 1 + i;
        unsigned int inNext = 1 + next;
        unsigned int outCur = outerOffset + i;
        unsigned int outNext = outerOffset + next;

        indices.push_back(inCur);
        indices.push_back(outCur);
        indices.push_back(outNext);

        indices.push_back(inCur);
        indices.push_back(outNext);
        indices.push_back(inNext);
    }

    return Mesh(vertices, indices);
}

