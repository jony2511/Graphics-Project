#ifndef HUD_H
#define HUD_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Shader.h"

struct HUDVertex {
    glm::vec2 pos;
    glm::vec4 color;
};

class HUD {
public:
    HUD();
    ~HUD();

    void init();
    void begin(int screenWidth, int screenHeight);
    void end();

    // 2D Drawing Primitives
    void drawRect(float x, float y, float w, float h, const glm::vec4& color);
    void drawRectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color);
    void drawCircle(float cx, float cy, float radius, int sectors, const glm::vec4& color);
    void drawCircleOutline(float cx, float cy, float radius, float thickness, int sectors, const glm::vec4& color);
    void drawLine(float x1, float y1, float x2, float y2, float thickness, const glm::vec4& color);
    void drawArrow(float cx, float cy, float angleRad, float length, float thickness, const glm::vec4& color);

    // Vector Typography & UI Components
    void drawText(float x, float y, const std::string& text, float scale, const glm::vec4& color);
    void drawBadge(float x, float y, float w, float h, const std::string& text, const glm::vec4& bgColor, const glm::vec4& textColor);
    void drawProgressBar(float x, float y, float w, float h, float fraction, const glm::vec4& fillColor, const glm::vec4& bgColor);
    void drawCompassRose(float cx, float cy, float radius, float headingDeg, float speedVal);

    // Master Dashboard Overlay
    void renderDashboard(
        int screenWidth, int screenHeight,
        float altitude, float verticalSpeed,
        float windSpeed, float windHeadingDeg,
        bool burnerActive,
        int cameraMode,
        int lightingMode,
        float fps
    );

private:
    GLuint VAO, VBO;
    std::vector<HUDVertex> batchVertices;
    Shader* hudShader;
    glm::mat4 projection;

    void addQuad(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, const glm::vec4& col);
    void flush();
    void drawGlyph(float x, float y, char c, float scale, const glm::vec4& color);
};

#endif // HUD_H
