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
};

#endif // MODEL_GENERATOR_H
