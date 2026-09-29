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
