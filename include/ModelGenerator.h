#ifndef MODEL_GENERATOR_H
#define MODEL_GENERATOR_H

#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

class ModelGenerator {
public:
    static Mesh createCube(float width, float height, float depth, const glm::vec3& color);
    static Mesh createSphere(float radius, int rings, int sectors, const glm::vec3& color, bool verticalStripes = false);
    static Mesh createCylinder(float bottomRadius, float topRadius, float height, int sectors, const glm::vec3& color);
    static Mesh createCone(float radius, float height, int sectors, const glm::vec3& color);
    static Mesh createPlane(float width, float depth, int subdivisions, const glm::vec3& color);
    static Mesh createBalloonEnvelope(float radius, float height, int rings, int sectors);

    // Phase 2: Rural Landscape & Architecture
    static Mesh createRollingTerrain(float width, float depth, int subdivisions);
    static Mesh createPrism(float width, float height, float depth, const glm::vec3& color);
    static Mesh createStripedWindsock(float baseRadius, float tipRadius, float length, int sectors, int numStripes);
    static Mesh createWindmillBlade(float length, float width, const glm::vec3& woodColor, const glm::vec3& sailColor);
    static Mesh createCurvedDirtRoad();
    static Mesh createHayBale(float radius, float length);

    // Authentic Rural Landscape & Architecture (Matching Reference Photo)
    static Mesh createVillageHut();
    static Mesh createBanyanShadeTree();
    static Mesh createPalmTree(float height = 11.0f, float tiltAngleDeg = 4.5f);
    static Mesh createReedCluster(int bladeCount = 14, float height = 1.8f);
    static Mesh createWetlandWater(float width, float depth);
    static Mesh createSkyBackdrop(float radius, float height);

    // Phase 3: High-Fidelity Eye-Catching Hot Air Balloon Rig & Atmosphere
    static Mesh createRainbowBalloonEnvelope(float radius, float height, int rings, int numGores, int colorScheme = 0);
    static Mesh createBalloonEquatorBelt(float radius, float thickness);
    static Mesh createBalloonWhiteSkirt(float topRadius, float botRadius, float height, int sectors);
    static Mesh createWovenBasket(float width, float height, float depth);
    static Mesh createCloudCluster();
    static Mesh createBirdBody();
    static Mesh createBirdWing(bool isLeft);
    static Mesh createShadowDisc(float radius, int sectors);
};

#endif // MODEL_GENERATOR_H
