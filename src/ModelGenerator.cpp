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

float ModelGenerator::getTerrainHeight(float x, float z) {
    float dist = std::sqrt(x * x + z * z);
    // Flat central area for launchpad platform and village clearing
    if (dist < 26.0f) return 0.0f;

    // Gentle natural rural meadow plain with subtle drainage knolls
    float weight = std::clamp((dist - 26.0f) / 48.0f, 0.0f, 1.0f);
    weight = weight * weight * (3.0f - 2.0f * weight); // smoothstep Hermite C1

    float h1 = 2.0f * std::sin(x * 0.022f + 0.35f) * std::cos(z * 0.026f - 0.25f);
    float h2 = 1.2f * std::sin(x * 0.048f + 1.2f) * std::sin(z * 0.042f + 0.7f);
    float h3 = 0.6f * std::cos(dist * 0.018f);
    float baseH = weight * (h1 + h2 + h3);

    // Natural pond basin depression around (46, 36)
    float distPond = std::sqrt((x - 46.0f) * (x - 46.0f) + (z - 36.0f) * (z - 36.0f));
    if (distPond < 20.0f) {
        float pondFactor = std::clamp(distPond / 20.0f, 0.0f, 1.0f);
        baseH = baseH * pondFactor - (1.0f - pondFactor) * 0.20f;
    }

    // Majestic distant rolling hill ridges along the outer perimeter (dist > 160m to 600m)
    if (dist > 160.0f) {
        float hillWeight = std::clamp((dist - 160.0f) / 180.0f, 0.0f, 1.0f);
        hillWeight = hillWeight * hillWeight * (3.0f - 2.0f * hillWeight);
        float ridge1 = 26.0f * std::pow(std::max(0.0f, std::sin(x * 0.007f + 1.2f) * std::cos(z * 0.006f - 0.9f)), 1.7f);
        float ridge2 = 16.0f * std::sin(dist * 0.009f + 1.5f) * std::cos(x * 0.004f);
        baseH += hillWeight * (ridge1 + ridge2);
    }

    return baseH;
}

Mesh ModelGenerator::createRollingTerrain(float width, float depth, int subdivisions) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;
    float stepX = width / (float)subdivisions;
    float stepZ = depth / (float)subdivisions;

    glm::vec3 uniformMeadowColor(0.20f, 0.52f, 0.18f);

    for (int z = 0; z <= subdivisions; ++z) {
        float posZ = -halfD + z * stepZ;
        for (int x = 0; x <= subdivisions; ++x) {
            float posX = -halfW + x * stepX;
            float posY = getTerrainHeight(posX, posZ);

            // Initialize vertex; normal will be accumulated smoothly from adjacent triangle faces
            vertices.push_back({{posX, posY, posZ}, glm::vec3(0.0f, 0.0f, 0.0f), uniformMeadowColor, {posX * 0.05f, posZ * 0.05f}});
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

    // Compute mathematically smooth, area-weighted vertex normals from triangle faces
    // (Completely eliminates ALL triangle creases and diagonal polygon seams!)
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 p0 = vertices[i0].position;
        glm::vec3 p1 = vertices[i1].position;
        glm::vec3 p2 = vertices[i2].position;

        glm::vec3 faceNorm = glm::cross(p1 - p0, p2 - p0);
        vertices[i0].normal += faceNorm;
        vertices[i1].normal += faceNorm;
        vertices[i2].normal += faceNorm;
    }

    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.0001f) {
            v.normal = glm::normalize(v.normal);
        } else {
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
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

    const int numSteps = 550;
    const float roadWidth = 4.8f;
    const int numCross = 13;

    // Catmull-Rom control points guiding the authentic Bengali rural earthen road:
    // 1. Starts in foreground right, skirting well clear of the launchpad (4m+ green buffer).
    // 2. Weaves gracefully through the village clearing, safely clear of Homestead 1 (14m+ front lawn).
    // 3. Extends continuously across the rolling meadows, far hills, and into the horizon mountain pass!
    struct SplinePoint {
        float x;
        float z;
    };

    const std::vector<SplinePoint> controlPoints = {
        {-28.0f, -34.0f}, // 0: Far foreground right (screen view)
        {-22.0f, -22.0f}, // 1: Approaching the launchpad clearing
        {-17.5f, -10.0f}, // 2: Generously outside the launchpad (X <= -17.5 vs platform edge X = -8)
        {-15.5f,   0.0f}, // 3: Along west perimeter of launchpad (platform edge is at X = -8, 5m+ clearance)
        {-14.5f,  10.0f}, // 4: Beyond the north-west platform corner (platform ends at Z = 8, 4m+ clearance)
        {-11.0f,  18.0f}, // 5: Curving smoothly in front of the platform clearing
        { -4.0f,  23.0f}, // 6: In front of platform gate (Z = 23 vs gate at Z = 8)
        {  3.0f,  26.0f}, // 7: Sweeping into the open meadow clearing
        {  9.5f,  31.0f}, // 8: Well clear of Homestead 1 at X = 27 (15m+ front yard!)
        { 15.0f,  40.0f}, // 9: Past Banyan tree and cottage front-yard
        { 19.0f,  54.0f}, // 10: Curving beside the pond bank
        { 22.0f,  72.0f}, // 11: Passing Homestead 2 area
        { 23.5f,  95.0f}, // 12: Winding past Homestead 4 area
        { 21.0f, 125.0f}, // 13: North grove corridor
        { 16.5f, 165.0f}, // 14: Rolling meadow vista
        { 11.0f, 215.0f}, // 15: Snaking towards distant rolling hills
        {  5.0f, 275.0f}, // 16: Visible meandering path in far landscape
        { -1.0f, 345.0f}, // 17: Cresting far horizon ridges
        { -7.0f, 425.0f}, // 18: Reaching the majestic distant mountain pass
        {-12.0f, 510.0f}  // 19: Seamlessly vanishing into the furthest horizon!
    };

    auto evaluateSpline = [&](float globalT, glm::vec2& outPos, glm::vec2& outTan) {
        int numSegments = (int)controlPoints.size() - 1;
        float scaledT = globalT * (float)numSegments;
        int i1 = std::clamp((int)std::floor(scaledT), 0, numSegments - 1);
        float u = scaledT - (float)i1;

        int i0 = std::max(0, i1 - 1);
        int i2 = std::min(numSegments, i1 + 1);
        int i3 = std::min(numSegments, i1 + 2);

        glm::vec2 p0(controlPoints[i0].x, controlPoints[i0].z);
        glm::vec2 p1(controlPoints[i1].x, controlPoints[i1].z);
        glm::vec2 p2(controlPoints[i2].x, controlPoints[i2].z);
        glm::vec2 p3(controlPoints[i3].x, controlPoints[i3].z);

        glm::vec2 a = -p0 + 3.0f * p1 - 3.0f * p2 + p3;
        glm::vec2 b = 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3;
        glm::vec2 c = -p0 + p2;
        glm::vec2 d = 2.0f * p1;

        outPos = 0.5f * (d + u * (c + u * (b + u * a)));

        glm::vec2 tan = 0.5f * (c + 2.0f * b * u + 3.0f * a * u * u);
        if (glm::length(tan) > 0.0001f) {
            outTan = glm::normalize(tan);
        } else {
            outTan = glm::vec2(0.0f, 1.0f);
        }
    };

    // 13 cross-sectional lateral fraction coordinates from -1.0 to +1.0
    const float crossFractions[numCross] = {
        -1.00f, -0.85f, -0.70f, -0.55f, -0.38f, -0.20f, 0.00f,
         0.20f,  0.38f,  0.55f,  0.70f,  0.85f,  1.00f
    };

    // Smooth crowned earthen profile (flush at grass verge, gently crowned at center)
    const float heightDeltas[numCross] = {
        0.005f, // -1.00: Flush with outer grass verge
        0.012f, // -0.85: Outer earth slope
        0.018f, // -0.70: Side track
        0.023f, // -0.55: Inner slope
        0.027f, // -0.38: Crown shoulder
        0.030f, // -0.20: Crown rise
        0.032f, //  0.00: Gentle crowned center ridge
        0.030f, // +0.20: Crown rise
        0.027f, // +0.38: Crown shoulder
        0.023f, // +0.55: Inner slope
        0.018f, // +0.70: Side track
        0.012f, // +0.85: Outer earth slope
        0.005f  // +1.00: Flush with outer grass verge
    };

    // Baseline colors for golden-ochre rural earthen road (matching reference art):
    glm::vec3 colGoldenRoad(0.91f, 0.68f, 0.22f);
    glm::vec3 colCaramelSide(0.74f, 0.46f, 0.12f);
    glm::vec3 colVergeGrass(0.22f, 0.56f, 0.20f);

    float accumDist = 0.0f;
    glm::vec2 prevCenter(controlPoints[0].x, controlPoints[0].z);

    const float epsNorm = 0.4f;

    for (int i = 0; i <= numSteps; ++i) {
        float t = (float)i / (float)numSteps;
        glm::vec2 centerPos2D, tan2D;
        evaluateSpline(t, centerPos2D, tan2D);

        accumDist += glm::length(centerPos2D - prevCenter);
        prevCenter = centerPos2D;

        glm::vec2 side2D(-tan2D.y, tan2D.x);

        for (int c = 0; c < numCross; ++c) {
            float latFrac = crossFractions[c];
            float uCoord = (latFrac + 1.0f) * 0.5f; // [0.0, 1.0]

            glm::vec2 vertXZ = centerPos2D + side2D * (latFrac * roadWidth * 0.5f);
            float groundY = getTerrainHeight(vertXZ.x, vertXZ.y);
            float vertY = groundY + heightDeltas[c];

            // Finite-difference surface normal conforming to ground contours
            float hL = getTerrainHeight(vertXZ.x - epsNorm, vertXZ.y);
            float hR = getTerrainHeight(vertXZ.x + epsNorm, vertXZ.y);
            float hD = getTerrainHeight(vertXZ.x, vertXZ.y - epsNorm);
            float hU = getTerrainHeight(vertXZ.x, vertXZ.y + epsNorm);
            glm::vec3 norm = glm::normalize(glm::vec3((hL - hR) / (2.0f * epsNorm), 1.0f, (hD - hU) / (2.0f * epsNorm)));

            // Color blending across cross-section
            glm::vec3 vColor;
            float absLat = std::abs(latFrac);
            if (absLat > 0.85f) {
                float blend = (absLat - 0.85f) / 0.15f;
                vColor = glm::mix(colCaramelSide, colVergeGrass, blend);
            } else if (absLat > 0.45f) {
                float blend = (absLat - 0.45f) / 0.40f;
                vColor = glm::mix(colGoldenRoad, colCaramelSide, blend);
            } else {
                vColor = colGoldenRoad;
            }

            vertices.push_back({{vertXZ.x, vertY, vertXZ.y}, norm, vColor, {uCoord, accumDist * 0.16f}});
        }
    }

    for (int i = 0; i < numSteps; ++i) {
        unsigned int row1 = i * numCross;
        unsigned int row2 = (i + 1) * numCross;

        for (int c = 0; c < numCross - 1; ++c) {
            indices.push_back(row1 + c);
            indices.push_back(row2 + c);
            indices.push_back(row1 + c + 1);

            indices.push_back(row1 + c + 1);
            indices.push_back(row2 + c);
            indices.push_back(row2 + c + 1);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createVillageWell() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addBox = [&](glm::vec3 pMin, glm::vec3 pMax, glm::vec3 col) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n;
        // Top (+Y)
        n = glm::vec3(0, 1, 0);
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col, {0, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Bottom (-Y)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, -1, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.7f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.7f, {1, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.7f, {1, 1}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.7f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Front (+Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, 1);
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.95f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.95f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.95f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.95f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Back (-Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, -1);
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.85f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.85f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.85f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.85f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Right (+X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(1, 0, 0);
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.90f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.90f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.90f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.90f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Left (-X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(-1, 0, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.80f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.80f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.80f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.80f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    };

    // Colors matching the traditional brick well in the reference painting:
    glm::vec3 colBrick(0.78f, 0.44f, 0.26f);     // Terracotta kiln brick wall
    glm::vec3 colBrickRim(0.85f, 0.52f, 0.32f);  // Raised brick rim
    glm::vec3 colApronStone(0.56f, 0.53f, 0.48f);// Base paved platform
    glm::vec3 colWood(0.42f, 0.28f, 0.17f);      // Heavy timber frame
    glm::vec3 colDarkWater(0.12f, 0.35f, 0.38f); // Deep well water
    glm::vec3 colRope(0.80f, 0.72f, 0.52f);      // Jute hemp rope
    glm::vec3 colBucket(0.50f, 0.35f, 0.20f);    // Wood bucket

    const int sectors = 24;
    const float rOuter = 0.95f;
    const float rInner = 0.76f;
    const float hWall = 1.05f;

    // 1. Apron Stone Base (Circular plinth around well)
    float rApron = 1.45f;
    unsigned int cBaseIdx = (unsigned int)vertices.size();
    vertices.push_back({{0.0f, 0.10f, 0.0f}, {0, 1, 0}, colApronStone, {0.5f, 0.5f}});
    for (int s = 0; s <= sectors; ++s) {
        float angle = (float)s / (float)sectors * 2.0f * (float)M_PI;
        float x = rApron * std::cos(angle);
        float z = rApron * std::sin(angle);
        vertices.push_back({{x, 0.10f, z}, {0, 1, 0}, colApronStone * 0.95f, {0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)}});
        if (s > 0) {
            indices.push_back(cBaseIdx);
            indices.push_back(cBaseIdx + s);
            indices.push_back(cBaseIdx + s + 1);
        }
    }

    // 2. Circular Outer Brick Wall (Cylinder side)
    for (int s = 0; s <= sectors; ++s) {
        float angle = (float)s / (float)sectors * 2.0f * (float)M_PI;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        glm::vec3 norm(cosA, 0.0f, sinA);

        glm::vec3 bCol = (s % 2 == 0) ? colBrick : colBrick * 1.08f;

        unsigned int b = (unsigned int)vertices.size();
        vertices.push_back({{rOuter * cosA, 0.10f, rOuter * sinA}, norm, bCol * 0.92f, {(float)s, 0.0f}});
        vertices.push_back({{rOuter * cosA, hWall, rOuter * sinA}, norm, bCol, {(float)s, 1.0f}});

        if (s > 0) {
            indices.push_back(b - 2); indices.push_back(b - 1); indices.push_back(b);
            indices.push_back(b - 1); indices.push_back(b + 1); indices.push_back(b);
        }
    }

    // 3. Inner Wall (facing inside)
    for (int s = 0; s <= sectors; ++s) {
        float angle = (float)s / (float)sectors * 2.0f * (float)M_PI;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        glm::vec3 norm(-cosA, 0.0f, -sinA);

        unsigned int b = (unsigned int)vertices.size();
        vertices.push_back({{rInner * cosA, 0.35f, rInner * sinA}, norm, colBrick * 0.70f, {(float)s, 0.0f}});
        vertices.push_back({{rInner * cosA, hWall, rInner * sinA}, norm, colBrick * 0.85f, {(float)s, 1.0f}});

        if (s > 0) {
            indices.push_back(b - 2); indices.push_back(b);     indices.push_back(b - 1);
            indices.push_back(b - 1); indices.push_back(b);     indices.push_back(b + 1);
        }
    }

    // 4. Well Rim Cap (Ring joining outer and inner wall at top)
    for (int s = 0; s <= sectors; ++s) {
        float angle = (float)s / (float)sectors * 2.0f * (float)M_PI;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        glm::vec3 norm(0.0f, 1.0f, 0.0f);

        unsigned int b = (unsigned int)vertices.size();
        vertices.push_back({{rInner * cosA, hWall, rInner * sinA}, norm, colBrickRim, {(float)s, 0.0f}});
        vertices.push_back({{rOuter * cosA, hWall, rOuter * sinA}, norm, colBrickRim * 0.95f, {(float)s, 1.0f}});

        if (s > 0) {
            indices.push_back(b - 2); indices.push_back(b - 1); indices.push_back(b);
            indices.push_back(b - 1); indices.push_back(b + 1); indices.push_back(b);
        }
    }

    // 5. Water Surface Inside (Reflective disc at height 0.40m)
    unsigned int waterIdx = (unsigned int)vertices.size();
    vertices.push_back({{0.0f, 0.40f, 0.0f}, {0, 1, 0}, colDarkWater, {0.5f, 0.5f}});
    for (int s = 0; s <= sectors; ++s) {
        float angle = (float)s / (float)sectors * 2.0f * (float)M_PI;
        float x = (rInner - 0.02f) * std::cos(angle);
        float z = (rInner - 0.02f) * std::sin(angle);
        vertices.push_back({{x, 0.40f, z}, {0, 1, 0}, colDarkWater * 1.15f, {0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)}});
        if (s > 0) {
            indices.push_back(waterIdx);
            indices.push_back(waterIdx + s);
            indices.push_back(waterIdx + s + 1);
        }
    }

    // 6. Upright Wooden Posts & Timber Pulley Gantry
    addBox(glm::vec3(-0.06f, 0.10f, -0.92f), glm::vec3(0.06f, 2.35f, -0.80f), colWood);
    addBox(glm::vec3(-0.06f, 0.10f,  0.80f), glm::vec3(0.06f, 2.35f,  0.92f), colWood);
    // Crossbeam
    addBox(glm::vec3(-0.07f, 2.25f, -1.02f), glm::vec3(0.07f, 2.39f,  1.02f), colWood * 1.05f);

    // Pulley spindle & wheel
    addBox(glm::vec3(-0.03f, 2.08f, -0.15f), glm::vec3(0.03f, 2.25f,  0.15f), glm::vec3(0.30f, 0.28f, 0.25f));
    // Hanging rope
    addBox(glm::vec3(-0.015f, 1.20f, -0.015f), glm::vec3(0.015f, 2.15f, 0.015f), colRope);
    // Suspended wood bucket
    addBox(glm::vec3(-0.15f, 0.95f, -0.15f), glm::vec3(0.15f, 1.25f, 0.15f), colBucket);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createRusticLanternPost() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addBox = [&](glm::vec3 pMin, glm::vec3 pMax, glm::vec3 col) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n;
        // Top (+Y)
        n = glm::vec3(0, 1, 0);
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col, {0, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Bottom (-Y)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, -1, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.7f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.7f, {1, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.7f, {1, 1}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.7f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Front (+Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, 1);
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.95f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.95f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.95f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.95f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Back (-Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, -1);
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.85f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.85f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.85f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.85f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Right (+X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(1, 0, 0);
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.90f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.90f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.90f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.90f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Left (-X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(-1, 0, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.80f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.80f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.80f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.80f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    };

    glm::vec3 colStoneBase(0.48f, 0.46f, 0.44f);
    glm::vec3 colDarkWood(0.35f, 0.23f, 0.15f);
    glm::vec3 colIron(0.20f, 0.20f, 0.22f);
    glm::vec3 colWarmGlass(0.98f, 0.86f, 0.52f);

    // 1. Chiseled Stone Base Plinth
    addBox(glm::vec3(-0.22f, 0.0f, -0.22f), glm::vec3(0.22f, 0.35f, 0.22f), colStoneBase);
    addBox(glm::vec3(-0.18f, 0.35f, -0.18f), glm::vec3(0.18f, 0.45f, 0.18f), colStoneBase * 1.05f);

    // 2. Heavy Timber Upright Post (2.75m height)
    addBox(glm::vec3(-0.10f, 0.45f, -0.10f), glm::vec3(0.10f, 2.75f, 0.10f), colDarkWood);

    // 3. Post Timber Cap & Finial
    addBox(glm::vec3(-0.13f, 2.75f, -0.13f), glm::vec3(0.13f, 2.88f, 0.13f), colDarkWood * 1.1f);
    addBox(glm::vec3(-0.06f, 2.88f, -0.06f), glm::vec3(0.06f, 2.98f, 0.06f), colIron);

    // 4. Wrought Iron Extended Cantilever Arm (extends 0.65m outward)
    addBox(glm::vec3(-0.035f, 2.65f, -0.035f), glm::vec3(0.65f, 2.73f, 0.035f), colIron);
    // Diagonal decorative iron strut brace
    addBox(glm::vec3(0.08f, 2.25f, -0.025f), glm::vec3(0.45f, 2.65f, 0.025f), colIron);

    // 5. Hanging Lantern Housing at arm end (x = 0.55m)
    float lx = 0.55f;
    addBox(glm::vec3(lx - 0.02f, 2.55f, -0.02f), glm::vec3(lx + 0.02f, 2.65f, 0.02f), colIron);
    addBox(glm::vec3(lx - 0.16f, 2.45f, -0.16f), glm::vec3(lx + 0.16f, 2.55f, 0.16f), colIron);
    addBox(glm::vec3(lx - 0.12f, 2.10f, -0.12f), glm::vec3(lx + 0.12f, 2.15f, 0.12f), colIron);

    // 4 Corner Iron Struts
    addBox(glm::vec3(lx - 0.14f, 2.15f, -0.14f), glm::vec3(lx - 0.11f, 2.45f, -0.11f), colIron);
    addBox(glm::vec3(lx + 0.11f, 2.15f, -0.14f), glm::vec3(lx + 0.14f, 2.45f, -0.11f), colIron);
    addBox(glm::vec3(lx - 0.14f, 2.15f,  0.11f), glm::vec3(lx - 0.11f, 2.45f,  0.14f), colIron);
    addBox(glm::vec3(lx + 0.11f, 2.15f,  0.11f), glm::vec3(lx + 0.14f, 2.45f,  0.14f), colIron);

    // Warm Amber Glass Core
    addBox(glm::vec3(lx - 0.10f, 2.15f, -0.10f), glm::vec3(lx + 0.10f, 2.45f, 0.10f), colWarmGlass);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createWaypointSignpost() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addBox = [&](glm::vec3 pMin, glm::vec3 pMax, glm::vec3 col) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n;
        // Top (+Y)
        n = glm::vec3(0, 1, 0);
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col, {0, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Bottom (-Y)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, -1, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.7f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.7f, {1, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.7f, {1, 1}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.7f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Front (+Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, 1);
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.95f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.95f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.95f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.95f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Back (-Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, -1);
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.85f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.85f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.85f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.85f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Right (+X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(1, 0, 0);
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.90f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.90f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.90f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.90f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Left (-X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(-1, 0, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.80f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.80f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.80f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.80f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    };

    glm::vec3 colPostWood(0.40f, 0.28f, 0.18f);
    glm::vec3 colBoard1(0.76f, 0.62f, 0.44f);
    glm::vec3 colBoard2(0.68f, 0.54f, 0.38f);
    glm::vec3 colIron(0.20f, 0.20f, 0.22f);

    // 1. Base mound stones
    addBox(glm::vec3(-0.25f, 0.0f, -0.25f), glm::vec3(0.25f, 0.20f, 0.25f), glm::vec3(0.50f, 0.48f, 0.45f));

    // 2. Main timber post (2.3m height)
    addBox(glm::vec3(-0.09f, 0.20f, -0.09f), glm::vec3(0.09f, 2.30f, 0.09f), colPostWood);
    // Pyramid tip cap
    addBox(glm::vec3(-0.11f, 2.30f, -0.11f), glm::vec3(0.11f, 2.42f, 0.11f), colPostWood * 0.9f);

    // 3. Top Signboard pointing +X (Meadow Path & Village Pond)
    addBox(glm::vec3(-0.05f, 2.02f, -0.04f), glm::vec3(0.85f, 2.22f, 0.04f), colBoard1);
    addBox(glm::vec3(0.85f, 2.06f, -0.035f), glm::vec3(0.96f, 2.18f, 0.035f), colBoard1 * 0.95f);

    // 4. Lower Signboard pointing -Z (Airfield / Launch Base)
    addBox(glm::vec3(-0.04f, 1.76f, -0.80f), glm::vec3(0.04f, 1.96f, 0.05f), colBoard2);
    addBox(glm::vec3(-0.035f, 1.80f, -0.92f), glm::vec3(0.035f, 1.92f, -0.80f), colBoard2 * 0.95f);

    // 5. Iron mounting bands & bolts
    addBox(glm::vec3(-0.10f, 2.04f, -0.10f), glm::vec3(0.10f, 2.08f, 0.10f), colIron);
    addBox(glm::vec3(-0.10f, 1.78f, -0.10f), glm::vec3(0.10f, 1.82f, 0.10f), colIron);

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

    const int sectorsPerGore = 4;
    const int totalSectors = numGores * sectorsPerGore;

    // Geometric Envelope Proportions:
    // Classic aerodynamic bulbous teardrop hot air balloon profile
    // The upper dome is an ellipse closing at the top apex (r = 0, dy/dr = 0)
    // The lower cone tapers smoothly into the throat collar (r = throatRadius)
    const float splitV = 0.36f;        // Equator location: 36% down from apex
    const float hTop = height * 0.38f; // Height of upper ellipsoidal dome (3.65m)
    const float hBot = height * 0.62f; // Height of lower tapering body (5.95m)
    const float yEq = height * 0.12f;  // Equator Y level relative to balloon origin (+1.15m)
    const float throatRadius = 1.40f;  // Fits into the throat skirt collar (1.55m top radius)

    for (int r = 0; r < rings; ++r) {
        float v = (float)r / (float)(rings - 1); // 0 (top apex) to 1 (bottom throat)

        float rBase;
        float y;
        float nr, ny; // Outward radial and vertical normal components

        if (v <= splitV) {
            // ==========================================
            // Upper Bulbous Ellipsoidal Dome (v in [0, splitV])
            // ==========================================
            float u = v / splitV; // 0 (apex) to 1 (equator)
            float phi = u * 0.5f * (float)M_PI; // 0 to PI/2

            // Smooth ellipse:
            // At phi = 0: r = 0, y = yEq + hTop, dy/dr = 0 (horizontal dome apex)
            // At phi = PI/2: r = radius, y = yEq, dr/dy = 0 (vertical side profile)
            rBase = radius * std::sin(phi);
            y = yEq + hTop * std::cos(phi);

            nr = hTop * std::sin(phi);
            ny = radius * std::cos(phi);
        } else {
            // ==========================================
            // Lower Aerodynamic Tapering Body (v in (splitV, 1.0])
            // ==========================================
            float u = (v - splitV) / (1.0f - splitV); // 0 (equator) to 1 (throat)
            float psi = u * 0.5f * (float)M_PI; // 0 to PI/2

            // Smooth cosine transition:
            // At psi = 0: r = radius, dr/du = 0 (matches vertical tangent of upper dome!)
            // At psi = PI/2: r = throatRadius
            rBase = throatRadius + (radius - throatRadius) * std::cos(psi);
            y = yEq - u * hBot;

            nr = hBot;
            ny = -(radius - throatRadius) * 0.5f * (float)M_PI * std::sin(psi);
        }

        // Puffy gore bulge envelope (smoothly fades to 0 at apex and throat)
        float gorePuffEnvelope = std::sin(v * (float)M_PI);

        for (int s = 0; s < totalSectors; ++s) {
            int goreIndex = s / sectorsPerGore;
            int goreSector = s % sectorsPerGore;
            float goreFrac = (float)goreSector / (float)sectorsPerGore;

            // 3D Puffy gore bulge between vertical load tapes
            float bulge = 1.0f + 0.046f * gorePuffEnvelope * std::sin(goreFrac * (float)M_PI);
            float currentRadius = rBase * bulge;

            float uHoriz = (float)s / (float)totalSectors;
            float theta = uHoriz * 2.0f * (float)M_PI;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            float x = cosT * currentRadius;
            float z = sinT * currentRadius;

            glm::vec3 pos(x, y, z);
            glm::vec3 norm = glm::normalize(glm::vec3(nr * cosT, ny, nr * sinT));
            if (v == 0.0f) norm = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec3 vertColor;
            if (colorScheme == 0) {
                // Staggered horizontal rainbow bands matching the user's reference photograph
                float staggeredV = v + (float)(goreIndex % 2) * 0.045f;
                if (staggeredV < 0.16f) {
                    // 1. Royal Sky Blue (Crown & upper dome)
                    vertColor = glm::vec3(0.12f, 0.44f, 0.88f);
                } else if (staggeredV < 0.28f) {
                    // 2. Magenta / Violet Purple
                    vertColor = glm::vec3(0.56f, 0.16f, 0.72f);
                } else if (staggeredV < 0.44f) {
                    // 3. Crimson Red
                    vertColor = glm::vec3(0.92f, 0.18f, 0.20f);
                } else if (staggeredV < 0.62f) {
                    // 4. Vivid Sunset Orange
                    vertColor = glm::vec3(0.98f, 0.52f, 0.10f);
                } else if (staggeredV < 0.82f) {
                    // 5. Sunny Yellow
                    vertColor = glm::vec3(0.98f, 0.86f, 0.08f);
                } else {
                    // 6. Bright Golden Lime Yellow (Throat)
                    vertColor = glm::vec3(0.96f, 0.90f, 0.22f);
                }

                // Top crown cap (deep navy apex valve cap)
                if (v < 0.028f) {
                    vertColor = glm::vec3(0.08f, 0.24f, 0.52f);
                }
            } else if (colorScheme == 1) {
                // Sunset Fire (Background Balloon 1)
                const glm::vec3 sunsetColors[5] = {
                    glm::vec3(0.94f, 0.22f, 0.18f), // Coral Red
                    glm::vec3(0.98f, 0.55f, 0.12f), // Orange
                    glm::vec3(0.98f, 0.84f, 0.15f), // Gold
                    glm::vec3(0.96f, 0.96f, 0.96f), // White
                    glm::vec3(0.92f, 0.28f, 0.24f)  // Crimson
                };
                int cIdx = (int)((v * 5.0f) + (goreIndex % 2) * 0.5f) % 5;
                vertColor = sunsetColors[cIdx];
                if (v < 0.028f) vertColor = glm::vec3(0.35f, 0.12f, 0.10f);
            } else if (colorScheme == 2) {
                // Ocean Teal (Background Balloon 2)
                const glm::vec3 oceanColors[5] = {
                    glm::vec3(0.08f, 0.32f, 0.72f), // Royal Blue
                    glm::vec3(0.12f, 0.64f, 0.82f), // Cyan
                    glm::vec3(0.20f, 0.80f, 0.72f), // Teal Mint
                    glm::vec3(0.96f, 0.96f, 0.96f), // White
                    glm::vec3(0.10f, 0.40f, 0.78f)  // Navy
                };
                int cIdx = (int)((v * 5.0f) + (goreIndex % 2) * 0.5f) % 5;
                vertColor = oceanColors[cIdx];
                if (v < 0.028f) vertColor = glm::vec3(0.06f, 0.18f, 0.35f);
            } else {
                // Emerald Gold (Background Balloon 3)
                const glm::vec3 emeraldColors[5] = {
                    glm::vec3(0.12f, 0.65f, 0.28f), // Emerald Green
                    glm::vec3(0.96f, 0.85f, 0.15f), // Golden Yellow
                    glm::vec3(0.18f, 0.78f, 0.45f), // Spring Mint
                    glm::vec3(0.96f, 0.96f, 0.96f), // Pure White
                    glm::vec3(0.08f, 0.45f, 0.20f)  // Forest Green
                };
                int cIdx = (int)((v * 5.0f) + (goreIndex % 2) * 0.5f) % 5;
                vertColor = emeraldColors[cIdx];
                if (v < 0.028f) vertColor = glm::vec3(0.06f, 0.28f, 0.12f);
            }

            // Realistic vertical load tape seam groove between gores
            if (goreFrac < 0.08f || goreFrac > 0.92f) {
                vertColor *= 0.80f;
            }

            vertices.push_back({pos, norm, vertColor, {uHoriz, v}});
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < totalSectors; ++s) {
            int nextS = (s + 1) % totalSectors;
            unsigned int cur = r * totalSectors + s;
            unsigned int right = r * totalSectors + nextS;
            unsigned int next = (r + 1) * totalSectors + s;
            unsigned int nextRight = (r + 1) * totalSectors + nextS;

            if (r == 0) {
                // Apex cap: clean triangle fan from top pole
                indices.push_back(cur);
                indices.push_back(next);
                indices.push_back(nextRight);
            } else {
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

    glm::vec3 shadowCol(0.02f, 0.04f, 0.02f);
    glm::vec3 norm(0.0f, 1.0f, 0.0f);

    // Center vertex: TexCoords = (0.0, 0.0) -> r = 0.0
    vertices.push_back({{0.0f, 0.0f, 0.0f}, norm, shadowCol, {0.0f, 0.0f}});

    // Perimeter vertices at radius with TexCoords = (cos, sin) -> length(TexCoords) = 1.0
    for (int i = 0; i < sectors; ++i) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float c = std::cos(angle);
        float s = std::sin(angle);
        vertices.push_back({{c * radius, 0.0f, s * radius}, norm, shadowCol, {c, s}});
    }

    // Triangular fan from center to perimeter
    for (int i = 0; i < sectors; ++i) {
        int next = (i + 1) % sectors;
        indices.push_back(0);
        indices.push_back(1 + i);
        indices.push_back(1 + next);
    }

    return Mesh(vertices, indices);
}

// ========================================================
// Authentic Rural Architecture & Environment (Matching Photo)
// ========================================================

Mesh ModelGenerator::createVillageHut() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addBox = [&](glm::vec3 pMin, glm::vec3 pMax, glm::vec3 col) {
        unsigned int b = (unsigned int)vertices.size();
        glm::vec3 n;
        // Top (+Y)
        n = glm::vec3(0, 1, 0);
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col, {0, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Bottom (-Y)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, -1, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.7f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.7f, {1, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.7f, {1, 1}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.7f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Front (+Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, 1);
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.95f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.95f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.95f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.95f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Back (-Z)
        b = (unsigned int)vertices.size();
        n = glm::vec3(0, 0, -1);
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.85f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.85f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.85f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.85f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Left (-X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(-1, 0, 0);
        vertices.push_back({{pMin.x, pMin.y, pMin.z}, n, col * 0.90f, {0, 0}});
        vertices.push_back({{pMin.x, pMin.y, pMax.z}, n, col * 0.90f, {1, 0}});
        vertices.push_back({{pMin.x, pMax.y, pMax.z}, n, col * 0.90f, {1, 1}});
        vertices.push_back({{pMin.x, pMax.y, pMin.z}, n, col * 0.90f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Right (+X)
        b = (unsigned int)vertices.size();
        n = glm::vec3(1, 0, 0);
        vertices.push_back({{pMax.x, pMin.y, pMax.z}, n, col * 0.92f, {0, 0}});
        vertices.push_back({{pMax.x, pMin.y, pMin.z}, n, col * 0.92f, {1, 0}});
        vertices.push_back({{pMax.x, pMax.y, pMin.z}, n, col * 0.92f, {1, 1}});
        vertices.push_back({{pMax.x, pMax.y, pMax.z}, n, col * 0.92f, {0, 1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    };

    // Color Palette from Photograph
    glm::vec3 plinthCol(0.55f, 0.50f, 0.44f);     // Weathered earthen concrete plinth
    glm::vec3 plinthTrim(0.40f, 0.36f, 0.30f);    // Damp foundation soil edge
    glm::vec3 wallPlaster(0.72f, 0.68f, 0.60f);   // Weathered beige lime/mud plaster
    glm::vec3 wallShadow(0.58f, 0.54f, 0.46f);    // Stained plaster base
    glm::vec3 pillarCol(0.74f, 0.71f, 0.65f);     // Rustic veranda pillars
    glm::vec3 ledgeCol(0.62f, 0.58f, 0.50f);      // Veranda sitting ledge
    glm::vec3 darkInterior(0.10f, 0.08f, 0.06f);  // Shaded doorway interior
    glm::vec3 tileRed1(0.56f, 0.25f, 0.18f);      // Terracotta clay tile base
    glm::vec3 tileRed2(0.46f, 0.20f, 0.14f);      // Weathered dark tile
    glm::vec3 tileRed3(0.62f, 0.29f, 0.21f);      // Sunlit tile highlight
    glm::vec3 tileRidge(0.38f, 0.17f, 0.12f);     // Dark ridge cap tiles
    glm::vec3 woodBeam(0.28f, 0.18f, 0.12f);      // Roof underside timbers

    // 1. Raised Foundation Plinth
    addBox({-5.6f, 0.0f, -4.5f}, {5.6f, 0.65f, 4.5f}, plinthCol);
    addBox({-5.7f, 0.0f, -4.6f}, {5.7f, 0.22f, 4.6f}, plinthTrim); // Damp ground border
    addBox({-1.4f, 0.0f, 4.5f}, {1.4f, 0.35f, 5.3f}, plinthCol * 0.95f); // Front entry steps

    // 2. Enclosed Main Room (Back Section)
    // Back wall
    addBox({-5.0f, 0.65f, -4.0f}, {5.0f, 3.65f, -3.6f}, wallPlaster);
    // Left exterior wall
    addBox({-5.0f, 0.65f, -4.0f}, {-4.6f, 3.65f, 1.0f}, wallPlaster);
    // Right exterior wall
    addBox({4.6f, 0.65f, -4.0f}, {5.0f, 3.65f, 1.0f}, wallPlaster);
    // Plaster stain band along bottom of walls
    addBox({-5.05f, 0.65f, -4.05f}, {5.05f, 1.25f, -3.55f}, wallShadow);
    addBox({-5.05f, 0.65f, -4.05f}, {-4.55f, 1.25f, 1.05f}, wallShadow);
    addBox({4.55f, 0.65f, -4.05f}, {5.05f, 1.25f, 1.05f}, wallShadow);

    // Front interior partition wall with open doorway and window
    addBox({-4.6f, 0.65f, 0.7f}, {-1.3f, 3.65f, 1.0f}, wallPlaster); // Left front wall
    addBox({1.3f, 0.65f, 0.7f}, {4.6f, 3.65f, 1.0f}, wallPlaster);   // Right front wall
    addBox({-1.3f, 2.95f, 0.7f}, {1.3f, 3.65f, 1.0f}, wallPlaster);  // Doorway lintel
    addBox({-1.25f, 0.65f, 0.5f}, {1.25f, 2.95f, 0.75f}, darkInterior); // Recessed interior void
    // Side window on right
    addBox({2.2f, 1.7f, 0.65f}, {3.7f, 2.7f, 1.05f}, darkInterior);
    addBox({2.1f, 2.7f, 0.62f}, {3.8f, 2.85f, 1.08f}, woodBeam);     // Window wooden lintel

    // 3. Open Front Veranda with Rustic Square Pillars
    // Veranda floor
    addBox({-4.8f, 0.65f, 1.0f}, {4.8f, 0.68f, 4.0f}, glm::vec3(0.58f, 0.54f, 0.46f));

    // 4 Rustic square pillars supporting the veranda roof
    float pillarXs[4] = {-4.5f, -1.6f, 1.6f, 4.5f};
    for (int p = 0; p < 4; ++p) {
        float px = pillarXs[p];
        addBox({px - 0.22f, 0.65f, 3.7f - 0.22f}, {px + 0.22f, 3.65f, 3.7f + 0.22f}, pillarCol);
        addBox({px - 0.26f, 0.65f, 3.7f - 0.26f}, {px + 0.26f, 1.15f, 3.7f + 0.26f}, pillarCol * 0.85f); // Pillar base
    }

    // Low waist-high sitting parapet ledges between outer pillars
    addBox({-4.3f, 0.65f, 3.60f}, {-1.8f, 1.55f, 3.82f}, ledgeCol); // Left front ledge
    addBox({1.8f, 0.65f, 3.60f}, {4.3f, 1.55f, 3.82f}, ledgeCol);   // Right front ledge
    addBox({-4.7f, 0.65f, 1.0f}, {-4.45f, 1.55f, 3.5f}, ledgeCol);  // Left side veranda wall
    addBox({4.45f, 0.65f, 1.0f}, {4.7f, 1.55f, 3.5f}, ledgeCol);   // Right side veranda wall

    // Horizontal wooden eave beams
    addBox({-5.2f, 3.55f, 3.55f}, {5.2f, 3.75f, 3.85f}, woodBeam);
    addBox({-5.0f, 3.55f, -4.1f}, {5.0f, 3.75f, -3.8f}, woodBeam);
    addBox({-5.1f, 3.55f, -4.0f}, {-4.8f, 3.75f, 3.8f}, woodBeam);
    addBox({4.8f, 3.55f, -4.0f}, {5.1f, 3.75f, 3.8f}, woodBeam);

    // 4. Authentic 4-Sided Terracotta Clay Tiled Hip Roof
    // Eaves footprint: X in [-6.4, 6.4], Z in [-5.3, 5.2], Y = 3.65
    // Apex Ridge: X in [-2.6, 2.6], Z = -0.2, Y = 6.40
    float eavesY = 3.65f;
    float ridgeY = 6.40f;
    float eaveXMin = -6.4f, eaveXMax = 6.4f;
    float eaveZMin = -5.3f, eaveZMax = 5.2f;
    float ridgeXMin = -2.6f, ridgeXMax = 2.6f;
    float ridgeZ = -0.15f;

    const int tileTiers = 7;
    for (int t = 0; t < tileTiers; ++t) {
        float f0 = (float)t / (float)tileTiers;
        float f1 = (float)(t + 1) / (float)tileTiers;

        // Front Hip Slope Rows (Z from ridgeZ to eaveZMax)
        float y0 = glm::mix(ridgeY, eavesY, f0);
        float y1 = glm::mix(ridgeY, eavesY, f1);

        float zF0 = glm::mix(ridgeZ, eaveZMax, f0);
        float zF1 = glm::mix(ridgeZ, eaveZMax, f1);
        float xLF0 = glm::mix(ridgeXMin, eaveXMin, f0);
        float xRF0 = glm::mix(ridgeXMax, eaveXMax, f0);
        float xLF1 = glm::mix(ridgeXMin, eaveXMin, f1);
        float xRF1 = glm::mix(ridgeXMax, eaveXMax, f1);

        glm::vec3 nFront = glm::normalize(glm::vec3(0.0f, (eaveZMax - ridgeZ), (ridgeY - eavesY)));
        glm::vec3 colFront = (t % 2 == 0) ? tileRed1 : ((t % 3 == 0) ? tileRed3 : tileRed2);

        unsigned int b = (unsigned int)vertices.size();
        vertices.push_back({{xLF0, y0, zF0}, nFront, colFront, {0, f0}});
        vertices.push_back({{xRF0, y0, zF0}, nFront, colFront, {1, f0}});
        vertices.push_back({{xRF1, y1, zF1}, nFront, colFront * 0.96f, {1, f1}});
        vertices.push_back({{xLF1, y1, zF1}, nFront, colFront * 0.96f, {0, f1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Back Hip Slope Rows (Z from ridgeZ to eaveZMin)
        float zB0 = glm::mix(ridgeZ, eaveZMin, f0);
        float zB1 = glm::mix(ridgeZ, eaveZMin, f1);
        float xLB0 = glm::mix(ridgeXMin, eaveXMin, f0);
        float xRB0 = glm::mix(ridgeXMax, eaveXMax, f0);
        float xLB1 = glm::mix(ridgeXMin, eaveXMin, f1);
        float xRB1 = glm::mix(ridgeXMax, eaveXMax, f1);

        glm::vec3 nBack = glm::normalize(glm::vec3(0.0f, (ridgeZ - eaveZMin), -(ridgeY - eavesY)));
        glm::vec3 colBack = (t % 2 == 1) ? tileRed1 : tileRed2;

        b = (unsigned int)vertices.size();
        vertices.push_back({{xRB0, y0, zB0}, nBack, colBack, {1, f0}});
        vertices.push_back({{xLB0, y0, zB0}, nBack, colBack, {0, f0}});
        vertices.push_back({{xLB1, y1, zB1}, nBack, colBack * 0.94f, {0, f1}});
        vertices.push_back({{xRB1, y1, zB1}, nBack, colBack * 0.94f, {1, f1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Left Triangular Hip Face Rows
        glm::vec3 nLeft = glm::normalize(glm::vec3(-(ridgeY - eavesY), (ridgeXMax - ridgeXMin) * 0.4f, 0.0f));
        glm::vec3 colLeft = (t % 2 == 0) ? tileRed2 : tileRed1;

        b = (unsigned int)vertices.size();
        vertices.push_back({{xLF0, y0, zF0}, nLeft, colLeft, {0, f0}});
        vertices.push_back({{xLF1, y1, zF1}, nLeft, colLeft, {0, f1}});
        vertices.push_back({{xLB1, y1, zB1}, nLeft, colLeft, {1, f1}});
        vertices.push_back({{xLB0, y0, zB0}, nLeft, colLeft, {1, f0}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);

        // Right Triangular Hip Face Rows
        glm::vec3 nRight = glm::normalize(glm::vec3((ridgeY - eavesY), (ridgeXMax - ridgeXMin) * 0.4f, 0.0f));
        glm::vec3 colRight = (t % 2 == 1) ? tileRed3 : tileRed1;

        b = (unsigned int)vertices.size();
        vertices.push_back({{xRF0, y0, zF0}, nRight, colRight, {0, f0}});
        vertices.push_back({{xRB0, y0, zB0}, nRight, colRight, {1, f0}});
        vertices.push_back({{xRB1, y1, zB1}, nRight, colRight, {1, f1}});
        vertices.push_back({{xRF1, y1, zF1}, nRight, colRight, {0, f1}});
        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
    }

    // Top Main Ridge Cap Beam
    addBox({ridgeXMin - 0.2f, ridgeY - 0.05f, ridgeZ - 0.22f}, {ridgeXMax + 0.2f, ridgeY + 0.18f, ridgeZ + 0.22f}, tileRidge);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createBanyanShadeTree() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto addCylinderSeg = [&](glm::vec3 pBot, glm::vec3 pTop, float rBot, float rTop, glm::vec3 col, int sectors = 10) {
        glm::vec3 dir = pTop - pBot;
        float len = glm::length(dir);
        if (len < 0.001f) return;
        dir = glm::normalize(dir);

        glm::vec3 up(0, 1, 0);
        glm::vec3 side = glm::cross(up, dir);
        if (glm::length(side) < 0.001f) side = glm::cross(glm::vec3(1, 0, 0), dir);
        side = glm::normalize(side);
        glm::vec3 forward = glm::normalize(glm::cross(dir, side));

        unsigned int b = (unsigned int)vertices.size();
        for (int i = 0; i <= sectors; ++i) {
            float angle = 2.0f * (float)M_PI * (float)i / (float)sectors;
            float c = std::cos(angle);
            float s = std::sin(angle);
            glm::vec3 radial = side * c + forward * s;

            glm::vec3 vBot = pBot + radial * rBot;
            glm::vec3 vTop = pTop + radial * rTop;

            vertices.push_back({vBot, radial, col, {(float)i / sectors, 0.0f}});
            vertices.push_back({vTop, radial, col * 0.95f, {(float)i / sectors, 1.0f}});
        }

        for (int i = 0; i < sectors; ++i) {
            unsigned int idx = b + i * 2;
            indices.push_back(idx);
            indices.push_back(idx + 1);
            indices.push_back(idx + 3);

            indices.push_back(idx);
            indices.push_back(idx + 3);
            indices.push_back(idx + 2);
        }
    };

    auto addFoliageCluster = [&](glm::vec3 center, glm::vec3 radii, glm::vec3 col, int rings = 10, int sectors = 12) {
        unsigned int b = (unsigned int)vertices.size();
        for (int r = 0; r <= rings; ++r) {
            float phi = (float)r / (float)rings * (float)M_PI;
            float sinP = std::sin(phi);
            float cosP = std::cos(phi);

            for (int s = 0; s <= sectors; ++s) {
                float theta = (float)s / (float)sectors * 2.0f * (float)M_PI;
                float sinT = std::sin(theta);
                float cosT = std::cos(theta);

                glm::vec3 unitNorm(sinP * cosT, cosP, sinP * sinT);
                glm::vec3 pos = center + unitNorm * radii;

                // Subtle sunlit tint on upper leaves
                glm::vec3 leafCol = col * (0.85f + 0.30f * std::max(cosP, 0.0f));
                vertices.push_back({pos, unitNorm, leafCol, {(float)s / sectors, (float)r / rings}});
            }
        }

        for (int r = 0; r < rings; ++r) {
            for (int s = 0; s < sectors; ++s) {
                unsigned int cur = b + r * (sectors + 1) + s;
                unsigned int next = cur + (sectors + 1);

                indices.push_back(cur);
                indices.push_back(next);
                indices.push_back(cur + 1);

                indices.push_back(cur + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    };

    glm::vec3 barkCol(0.30f, 0.22f, 0.15f);

    // 1. Organic Root Flares & Main Trunk
    addCylinderSeg({0.0f, 0.0f, 0.0f}, {0.0f, 3.8f, 0.0f}, 1.25f, 0.85f, barkCol, 12);
    addCylinderSeg({-0.8f, 0.0f, -0.6f}, {0.0f, 1.4f, 0.0f}, 0.55f, 0.40f, barkCol * 0.9f, 8);
    addCylinderSeg({0.9f, 0.0f, -0.4f}, {0.0f, 1.6f, 0.0f}, 0.50f, 0.35f, barkCol * 0.9f, 8);
    addCylinderSeg({0.3f, 0.0f, 0.8f}, {0.0f, 1.5f, 0.0f}, 0.60f, 0.40f, barkCol * 0.9f, 8);

    // 2. Thick Spreading Boughs
    addCylinderSeg({0.0f, 3.8f, 0.0f}, {-2.4f, 6.2f, 1.2f}, 0.70f, 0.42f, barkCol, 10);
    addCylinderSeg({0.0f, 3.8f, 0.0f}, {2.8f, 6.0f, 0.8f}, 0.65f, 0.38f, barkCol, 10);
    addCylinderSeg({0.0f, 3.8f, 0.0f}, {-0.8f, 6.8f, -2.2f}, 0.60f, 0.35f, barkCol, 10);
    addCylinderSeg({0.0f, 3.8f, 0.0f}, {0.5f, 7.5f, 0.2f}, 0.65f, 0.36f, barkCol, 10);

    // Secondary branches spreading over the hut roof
    addCylinderSeg({2.8f, 6.0f, 0.8f}, {4.8f, 7.2f, 1.8f}, 0.38f, 0.22f, barkCol, 8);
    addCylinderSeg({-2.4f, 6.2f, 1.2f}, {-4.2f, 7.0f, 2.2f}, 0.42f, 0.24f, barkCol, 8);

    // 3. Multi-Tiered Layered Foliage Clusters (Lush Vibrant Green Canopy)
    glm::vec3 darkGreen(0.10f, 0.44f, 0.14f);    // Deep rich shade leaves
    glm::vec3 emeraldGreen(0.16f, 0.60f, 0.18f); // Rich lush green canopy
    glm::vec3 sunlitGreen(0.24f, 0.70f, 0.22f);  // Sunlit vibrant foliage

    addFoliageCluster({0.0f, 8.8f, 0.0f}, {3.8f, 2.8f, 3.8f}, sunlitGreen);
    addFoliageCluster({-2.8f, 7.2f, 1.4f}, {3.2f, 2.4f, 3.2f}, emeraldGreen);
    addFoliageCluster({3.2f, 7.0f, 1.2f}, {3.5f, 2.5f, 3.4f}, sunlitGreen);
    addFoliageCluster({-1.0f, 7.5f, -2.6f}, {3.2f, 2.2f, 3.0f}, darkGreen);
    addFoliageCluster({1.4f, 8.2f, -1.8f}, {3.0f, 2.4f, 2.8f}, darkGreen);
    addFoliageCluster({-4.2f, 7.2f, 2.4f}, {2.6f, 2.0f, 2.6f}, emeraldGreen);
    addFoliageCluster({5.0f, 7.4f, 2.0f}, {2.8f, 2.2f, 2.8f}, sunlitGreen);
    addFoliageCluster({0.0f, 6.5f, 2.6f}, {2.8f, 1.8f, 2.5f}, emeraldGreen);
    addFoliageCluster({-1.8f, 8.8f, 0.8f}, {2.6f, 2.0f, 2.6f}, sunlitGreen);

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createPalmTree(float height, float tiltAngleDeg) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int trunkSegs = 14;
    const int sectors = 8;
    float tiltRad = glm::radians(tiltAngleDeg);

    glm::vec3 barkDark(0.36f, 0.30f, 0.22f);
    glm::vec3 barkLight(0.48f, 0.40f, 0.30f);

    float curX = 0.0f, curY = 0.0f, curZ = 0.0f;
    float rBase = 0.42f;

    // 1. Slender ringed trunk with natural graceful curve
    for (int seg = 0; seg <= trunkSegs; ++seg) {
        float t = (float)seg / (float)trunkSegs;
        float segY = t * height;
        float segX = std::sin(t * 1.8f) * (height * 0.16f * std::sin(tiltRad));
        float segZ = std::cos(t * 1.8f) * (height * 0.14f * std::cos(tiltRad));

        float r = rBase * (1.0f - t * 0.45f);
        glm::vec3 col = (seg % 2 == 0) ? barkDark : barkLight;

        for (int s = 0; s <= sectors; ++s) {
            float angle = 2.0f * (float)M_PI * (float)s / (float)sectors;
            glm::vec3 norm(std::cos(angle), 0.15f, std::sin(angle));
            norm = glm::normalize(norm);
            glm::vec3 pos(segX + norm.x * r, segY, segZ + norm.z * r);

            vertices.push_back({pos, norm, col, {(float)s / sectors, t}});
        }

        if (seg == trunkSegs) {
            curX = segX; curY = segY; curZ = segZ;
        }
    }

    for (int seg = 0; seg < trunkSegs; ++seg) {
        for (int s = 0; s < sectors; ++s) {
            unsigned int cur = seg * (sectors + 1) + s;
            unsigned int next = cur + (sectors + 1);

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(cur + 1);

            indices.push_back(cur + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    // 2. Tropical Palm Fronds (12 Drooping Arching Leaves)
    const int numFronds = 12;
    const int frondSteps = 8;
    float frondLen = 4.2f;
    glm::vec3 frondCol(0.12f, 0.62f, 0.16f);

    for (int f = 0; f < numFronds; ++f) {
        float fAngle = 2.0f * (float)M_PI * (float)f / (float)numFronds;
        float fTilt = 0.35f + 0.30f * std::sin((float)f * 2.5f);

        for (int step = 0; step < frondSteps; ++step) {
            float t0 = (float)step / (float)frondSteps;
            float t1 = (float)(step + 1) / (float)frondSteps;

            float r0 = t0 * frondLen;
            float r1 = t1 * frondLen;
            float y0 = curY - t0 * t0 * 1.8f + t0 * 0.4f;
            float y1 = curY - t1 * t1 * 1.8f + t1 * 0.4f;

            float w0 = (1.0f - t0) * 0.55f;
            float w1 = (1.0f - t1) * 0.55f;

            glm::vec3 dir(std::cos(fAngle), 0.0f, std::sin(fAngle));
            glm::vec3 side(-dir.z, 0.0f, dir.x);
            glm::vec3 norm(0.0f, 1.0f, 0.0f);

            glm::vec3 p0L = glm::vec3(curX, y0, curZ) + dir * r0 - side * w0;
            glm::vec3 p0R = glm::vec3(curX, y0, curZ) + dir * r0 + side * w0;
            glm::vec3 p1L = glm::vec3(curX, y1, curZ) + dir * r1 - side * w1;
            glm::vec3 p1R = glm::vec3(curX, y1, curZ) + dir * r1 + side * w1;

            glm::vec3 stepCol = frondCol * (0.85f + 0.15f * (1.0f - t0));

            unsigned int b = (unsigned int)vertices.size();
            vertices.push_back({p0L, norm, stepCol, {0, t0}});
            vertices.push_back({p0R, norm, stepCol, {1, t0}});
            vertices.push_back({p1R, norm, stepCol, {1, t1}});
            vertices.push_back({p1L, norm, stepCol, {0, t1}});

            indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
            indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createReedCluster(int bladeCount, float height) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 stalkBase(0.14f, 0.62f, 0.18f); // Fresh vibrant green reed stalk
    glm::vec3 stalkTip(0.25f, 0.74f, 0.22f);  // Spring green reed tip

    for (int i = 0; i < bladeCount; ++i) {
        float angle = (float)i * (2.0f * (float)M_PI / (float)bladeCount);
        float rOff = 0.15f + 0.25f * ((float)(i % 5) / 5.0f);
        float bHeight = height * (0.75f + 0.40f * std::sin((float)i * 1.7f));
        float leanX = std::cos(angle) * (0.35f + 0.25f * std::sin((float)i * 2.3f));
        float leanZ = std::sin(angle) * (0.35f + 0.25f * std::cos((float)i * 1.9f));

        glm::vec3 p0(std::cos(angle) * rOff, 0.0f, std::sin(angle) * rOff);
        glm::vec3 pMid(p0.x + leanX * 0.4f, bHeight * 0.55f, p0.z + leanZ * 0.4f);
        glm::vec3 pTop(p0.x + leanX, bHeight, p0.z + leanZ);

        glm::vec3 side(-std::sin(angle) * 0.06f, 0.0f, std::cos(angle) * 0.06f);
        glm::vec3 norm(std::cos(angle), 0.2f, std::sin(angle));

        unsigned int b = (unsigned int)vertices.size();
        vertices.push_back({p0 - side, norm, stalkBase, {0, 0}});
        vertices.push_back({p0 + side, norm, stalkBase, {1, 0}});
        vertices.push_back({pMid + side * 0.7f, norm, stalkBase * 1.15f, {1, 0.5f}});
        vertices.push_back({pMid - side * 0.7f, norm, stalkBase * 1.15f, {0, 0.5f}});
        vertices.push_back({pTop, norm, stalkTip, {0.5f, 1.0f}});

        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b); indices.push_back(b+2); indices.push_back(b+3);
        indices.push_back(b+3); indices.push_back(b+2); indices.push_back(b+4);
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createWetlandWater(float width, float depth) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int subsX = 32;
    const int subsZ = 32;
    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;

    glm::vec3 waterDeep(0.14f, 0.40f, 0.52f); // Fresh reflective village pond water
    glm::vec3 norm(0.0f, 1.0f, 0.0f);

    for (int z = 0; z <= subsZ; ++z) {
        float fz = (float)z / (float)subsZ;
        float pz = -halfD + fz * depth;
        for (int x = 0; x <= subsX; ++x) {
            float fx = (float)x / (float)subsX;
            float px = -halfW + fx * width;

            // Subtle rippling surface
            float py = -0.06f + 0.015f * std::sin(px * 0.8f + pz * 0.6f);
            vertices.push_back({{px, py, pz}, norm, waterDeep, {fx, fz}});
        }
    }

    for (int z = 0; z < subsZ; ++z) {
        for (int x = 0; x < subsX; ++x) {
            unsigned int cur = z * (subsX + 1) + x;
            unsigned int next = cur + (subsX + 1);

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(cur + 1);

            indices.push_back(cur + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createSkyBackdrop(float radius, float height) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Panoramic Horizon Backdrop matching the dramatic sunset break in photograph:
    // Top: Moody overcast dark slate indigo
    // Middle: Intense golden-orange sunset glow
    // Low: Warm peach-gold horizon mist
    const int sectors = 48;
    const int tiers = 5;

    glm::vec3 skyColors[5] = {
        glm::vec3(0.92f, 0.58f, 0.28f), // Tier 0 (Ground horizon): Warm peach-gold mist
        glm::vec3(0.98f, 0.74f, 0.22f), // Tier 1 (Low sky): Radiant golden-orange sunset break
        glm::vec3(0.85f, 0.52f, 0.24f), // Tier 2 (Under-cloud glow): Amber gold fading
        glm::vec3(0.24f, 0.28f, 0.32f), // Tier 3 (Cloud base): Dark charcoal storm fringe
        glm::vec3(0.08f, 0.14f, 0.22f)  // Tier 4 (Upper sky): Deep moody overcast slate navy
    };

    float tierHeights[5] = {0.0f, height * 0.14f, height * 0.32f, height * 0.55f, height};

    for (int t = 0; t < tiers; ++t) {
        float y = tierHeights[t];
        glm::vec3 col = skyColors[t];

        for (int s = 0; s <= sectors; ++s) {
            float angle = (float)s / (float)sectors * 2.0f * (float)M_PI;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;

            glm::vec3 norm = -glm::normalize(glm::vec3(x, 0.0f, z)); // Inward pointing
            vertices.push_back({{x, y, z}, norm, col, {(float)s / sectors, (float)t / (tiers - 1)}});
        }
    }

    for (int t = 0; t < tiers - 1; ++t) {
        for (int s = 0; s < sectors; ++s) {
            unsigned int cur = t * (sectors + 1) + s;
            unsigned int next = cur + (sectors + 1);

            indices.push_back(cur);
            indices.push_back(cur + 1);
            indices.push_back(next);

            indices.push_back(cur + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createBananaTree() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 1. Soft green layered pseudo-stem trunk (tapered & gently tilted)
    const int trunkRings = 8;
    const int trunkSectors = 8;
    float trunkHeight = 3.6f;
    float rBase = 0.32f;
    float rTop = 0.18f;

    glm::vec3 stemBase(0.38f, 0.62f, 0.25f);
    glm::vec3 stemTop(0.48f, 0.74f, 0.28f);

    for (int r = 0; r <= trunkRings; ++r) {
        float frac = (float)r / (float)trunkRings;
        float y = frac * trunkHeight;
        float radius = glm::mix(rBase, rTop, frac);
        float leanX = 0.25f * frac * frac;
        float leanZ = 0.15f * frac;
        glm::vec3 col = glm::mix(stemBase, stemTop, frac);

        for (int s = 0; s <= trunkSectors; ++s) {
            float angle = 2.0f * (float)M_PI * (float)s / (float)trunkSectors;
            float cx = std::cos(angle);
            float cz = std::sin(angle);
            glm::vec3 norm(cx, 0.15f, cz);
            norm = glm::normalize(norm);
            glm::vec3 pos(leanX + cx * radius, y, leanZ + cz * radius);
            vertices.push_back({pos, norm, col, {(float)s / trunkSectors, frac}});
        }
    }

    for (int r = 0; r < trunkRings; ++r) {
        for (int s = 0; s < trunkSectors; ++s) {
            unsigned int cur = r * (trunkSectors + 1) + s;
            unsigned int next = cur + (trunkSectors + 1);

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(cur + 1);

            indices.push_back(cur + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    // 2. Large broad drooping emerald paddle leaves (8 leaves around top)
    const int numLeaves = 8;
    const int leafSteps = 7;
    float leafLen = 3.2f;
    glm::vec3 leafMidrib(0.55f, 0.78f, 0.32f);
    glm::vec3 leafBlade(0.18f, 0.70f, 0.22f);

    glm::vec3 trunkCrown(0.25f, trunkHeight, 0.15f);

    for (int l = 0; l < numLeaves; ++l) {
        float lAngle = 2.0f * (float)M_PI * (float)l / (float)numLeaves + (float)l * 0.15f;
        float droopRate = 0.95f + 0.35f * std::sin((float)l * 2.1f);

        glm::vec3 dir(std::cos(lAngle), 0.0f, std::sin(lAngle));
        glm::vec3 side(-dir.z, 0.0f, dir.x);
        glm::vec3 norm(0.0f, 1.0f, 0.0f);

        for (int st = 0; st < leafSteps; ++st) {
            float t0 = (float)st / (float)leafSteps;
            float t1 = (float)(st + 1) / (float)leafSteps;

            float r0 = t0 * leafLen;
            float r1 = t1 * leafLen;
            float y0 = trunkCrown.y + t0 * 0.6f - t0 * t0 * droopRate * 1.8f;
            float y1 = trunkCrown.y + t1 * 0.6f - t1 * t1 * droopRate * 1.8f;

            float w0 = std::sin(t0 * (float)M_PI) * 0.75f + 0.08f;
            float w1 = std::sin(t1 * (float)M_PI) * 0.75f + 0.08f;

            glm::vec3 c0 = trunkCrown + dir * r0; c0.y = y0;
            glm::vec3 c1 = trunkCrown + dir * r1; c1.y = y1;

            glm::vec3 p0L = c0 - side * w0;
            glm::vec3 p0R = c0 + side * w0;
            glm::vec3 p1L = c1 - side * w1;
            glm::vec3 p1R = c1 + side * w1;

            unsigned int b = (unsigned int)vertices.size();
            vertices.push_back({p0L, norm, leafBlade, {0.0f, t0}});
            vertices.push_back({c0, norm, leafMidrib, {0.5f, t0}});
            vertices.push_back({p0R, norm, leafBlade, {1.0f, t0}});
            vertices.push_back({p1L, norm, leafBlade, {0.0f, t1}});
            vertices.push_back({c1, norm, leafMidrib, {0.5f, t1}});
            vertices.push_back({p1R, norm, leafBlade, {1.0f, t1}});

            // Left quad
            indices.push_back(b); indices.push_back(b+3); indices.push_back(b+1);
            indices.push_back(b+1); indices.push_back(b+3); indices.push_back(b+4);
            // Right quad
            indices.push_back(b+1); indices.push_back(b+4); indices.push_back(b+2);
            indices.push_back(b+2); indices.push_back(b+4); indices.push_back(b+5);
        }
    }

    return Mesh(vertices, indices);
}

Mesh ModelGenerator::createLushBush(float radius) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int rings = 9;
    const int sectors = 12;

    glm::vec3 bushDark(0.14f, 0.52f, 0.16f);
    glm::vec3 bushBright(0.24f, 0.72f, 0.22f);

    for (int r = 0; r <= rings; ++r) {
        float phi = (float)r / (float)rings * ((float)M_PI * 0.55f);
        float sinP = std::sin(phi);
        float cosP = std::cos(phi);

        for (int s = 0; s <= sectors; ++s) {
            float theta = (float)s / (float)sectors * 2.0f * (float)M_PI;
            float sinT = std::sin(theta);
            float cosT = std::cos(theta);

            float ruffle = 1.0f + 0.18f * std::sin(theta * 3.0f) * std::cos(phi * 4.0f)
                                + 0.12f * std::cos(theta * 5.0f);

            glm::vec3 norm(sinP * cosT, cosP, sinP * sinT);
            glm::vec3 pos = norm * (radius * ruffle);
            pos.y *= 0.75f;

            float sunFactor = std::max(cosP, 0.0f);
            glm::vec3 col = glm::mix(bushDark, bushBright, sunFactor);

            vertices.push_back({pos, norm, col, {(float)s / sectors, (float)r / rings}});
        }
    }

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            unsigned int cur = r * (sectors + 1) + s;
            unsigned int next = cur + (sectors + 1);

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(cur + 1);

            indices.push_back(cur + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    return Mesh(vertices, indices);
}

