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

    // Phase 2: Rural Landscape & Architecture Generators
    static Mesh createRollingTerrain(float width, float depth, int subdivisions);
    static Mesh createPrism(float width, float height, float depth, const glm::vec3& color);
    static Mesh createStripedWindsock(float baseRadius, float tipRadius, float length, int sectors, int numStripes);
    static Mesh createWindmillBlade(float length, float width, const glm::vec3& woodColor, const glm::vec3& sailColor);
    static Mesh createCurvedDirtRoad();
    static Mesh createHayBale(float radius, float length);
};

#endif // MODEL_GENERATOR_H
