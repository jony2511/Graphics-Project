#include "HUD.h"
#include "ShaderSources.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cctype>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 5x7 Dot-Matrix Font Table (ASCII 32 to 90)
// Each character is 7 rows of 5-bit masks
static const uint8_t FONT_5X7[][7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x04}, // 33 '!'
    {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}, // 34 '"'
    {0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x00}, // 35 '#'
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}, // 36 '$'
    {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13}, // 37 '%'
    {0x08, 0x14, 0x14, 0x08, 0x15, 0x12, 0x0D}, // 38 '&'
    {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, // 39 '''
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}, // 40 '('
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}, // 41 ')'
    {0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00}, // 42 '*'
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}, // 43 '+'
    {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08}, // 44 ','
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // 45 '-'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}, // 46 '.'
    {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}, // 47 '/'
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 48 '0'
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 49 '1'
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F}, // 50 '2'
    {0x1F, 0x02, 0x04, 0x06, 0x01, 0x11, 0x0E}, // 51 '3'
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 52 '4'
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 53 '5'
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 54 '6'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 55 '7'
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 56 '8'
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 57 '9'
    {0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00}, // 58 ':'
    {0x00, 0x06, 0x06, 0x00, 0x06, 0x04, 0x08}, // 59 ';'
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}, // 60 '<'
    {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00}, // 61 '='
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}, // 62 '>'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, // 63 '?'
    {0x0E, 0x11, 0x17, 0x15, 0x1D, 0x10, 0x0E}, // 64 '@'
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // 65 'A'
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // 66 'B'
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // 67 'C'
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, // 68 'D'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // 69 'E'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // 70 'F'
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // 71 'G'
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // 72 'H'
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 73 'I'
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, // 74 'J'
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // 75 'K'
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // 76 'L'
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // 77 'M'
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // 78 'N'
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 79 'O'
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // 80 'P'
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // 81 'Q'
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // 82 'R'
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // 83 'S'
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // 84 'T'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 85 'U'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // 86 'V'
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, // 87 'W'
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // 88 'X'
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, // 89 'Y'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // 90 'Z'
    {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}, // 91 '['
    {0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00}, // 92 '\'
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}  // 93 ']'
};

HUD::HUD() : VAO(0), VBO(0), hudShader(nullptr), projection(1.0f) {}

HUD::~HUD() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    delete hudShader;
}

void HUD::init() {
    hudShader = new Shader(HUD_VERTEX_SHADER, HUD_FRAGMENT_SHADER);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(HUDVertex) * 4096, nullptr, GL_DYNAMIC_DRAW);

    // 0: Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HUDVertex), (void*)offsetof(HUDVertex, pos));

    // 1: Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(HUDVertex), (void*)offsetof(HUDVertex, color));

    glBindVertexArray(0);
}

void HUD::begin(int screenWidth, int screenHeight) {
    batchVertices.clear();
    projection = glm::ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);

    glViewport(0, 0, screenWidth, screenHeight);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hudShader->use();
    hudShader->setMat4("uProjection", projection);
}

void HUD::end() {
    flush();
    glEnable(GL_DEPTH_TEST);
}

void HUD::flush() {
    if (batchVertices.empty()) return;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, batchVertices.size() * sizeof(HUDVertex), batchVertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batchVertices.size()));
    glBindVertexArray(0);

    batchVertices.clear();
}

void HUD::addQuad(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, const glm::vec4& col) {
    batchVertices.push_back({p0, col});
    batchVertices.push_back({p1, col});
    batchVertices.push_back({p2, col});

    batchVertices.push_back({p0, col});
    batchVertices.push_back({p2, col});
    batchVertices.push_back({p3, col});
}

void HUD::drawRect(float x, float y, float w, float h, const glm::vec4& color) {
    glm::vec2 p0(x, y);
    glm::vec2 p1(x + w, y);
    glm::vec2 p2(x + w, y + h);
    glm::vec2 p3(x, y + h);
    addQuad(p0, p1, p2, p3, color);
}

void HUD::drawRectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color) {
    drawRect(x, y, w, thickness, color); // Top
    drawRect(x, y + h - thickness, w, thickness, color); // Bottom
    drawRect(x, y, thickness, h, color); // Left
    drawRect(x + w - thickness, y, thickness, h, color); // Right
}

void HUD::drawCircle(float cx, float cy, float radius, int sectors, const glm::vec4& color) {
    for (int i = 0; i < sectors; ++i) {
        float a1 = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float a2 = 2.0f * (float)M_PI * (float)(i + 1) / (float)sectors;

        glm::vec2 center(cx, cy);
        glm::vec2 p1(cx + std::cos(a1) * radius, cy + std::sin(a1) * radius);
        glm::vec2 p2(cx + std::cos(a2) * radius, cy + std::sin(a2) * radius);

        batchVertices.push_back({center, color});
        batchVertices.push_back({p1, color});
        batchVertices.push_back({p2, color});
    }
}

void HUD::drawCircleOutline(float cx, float cy, float radius, float thickness, int sectors, const glm::vec4& color) {
    float rIn = radius - thickness * 0.5f;
    float rOut = radius + thickness * 0.5f;

    for (int i = 0; i < sectors; ++i) {
        float a1 = 2.0f * (float)M_PI * (float)i / (float)sectors;
        float a2 = 2.0f * (float)M_PI * (float)(i + 1) / (float)sectors;

        glm::vec2 p1In(cx + std::cos(a1) * rIn, cy + std::sin(a1) * rIn);
        glm::vec2 p1Out(cx + std::cos(a1) * rOut, cy + std::sin(a1) * rOut);
        glm::vec2 p2In(cx + std::cos(a2) * rIn, cy + std::sin(a2) * rIn);
        glm::vec2 p2Out(cx + std::cos(a2) * rOut, cy + std::sin(a2) * rOut);

        addQuad(p1In, p1Out, p2Out, p2In, color);
    }
}

void HUD::drawLine(float x1, float y1, float x2, float y2, float thickness, const glm::vec4& color) {
    glm::vec2 p1(x1, y1);
    glm::vec2 p2(x2, y2);
    glm::vec2 dir = p2 - p1;
    float len = glm::length(dir);
    if (len < 0.001f) return;

    dir /= len;
    glm::vec2 norm(-dir.y, dir.x);
    norm *= (thickness * 0.5f);

    addQuad(p1 - norm, p1 + norm, p2 + norm, p2 - norm, color);
}

void HUD::drawArrow(float cx, float cy, float angleRad, float length, float thickness, const glm::vec4& color) {
    float tipX = cx + std::cos(angleRad) * length;
    float tipY = cy + std::sin(angleRad) * length;
    float tailX = cx - std::cos(angleRad) * (length * 0.5f);
    float tailY = cy - std::sin(angleRad) * (length * 0.5f);

    drawLine(tailX, tailY, tipX, tipY, thickness, color);

    // Arrowhead wings
    float wingAngle1 = angleRad + (float)M_PI * 0.82f;
    float wingAngle2 = angleRad - (float)M_PI * 0.82f;
    float wingLen = length * 0.35f;

    drawLine(tipX, tipY, tipX + std::cos(wingAngle1) * wingLen, tipY + std::sin(wingAngle1) * wingLen, thickness, color);
    drawLine(tipX, tipY, tipX + std::cos(wingAngle2) * wingLen, tipY + std::sin(wingAngle2) * wingLen, thickness, color);
}

void HUD::drawGlyph(float x, float y, char c, float scale, const glm::vec4& color) {
    float px = scale * 1.6f;
    if (c == '|') {
        drawRect(x + 2.0f * px, y, px, 7.0f * px, color);
        return;
    }
    c = std::toupper(static_cast<unsigned char>(c));
    if (c < 32 || c > 93) c = ' ';

    int idx = c - 32;

    for (int r = 0; r < 7; ++r) {
        uint8_t rowBits = FONT_5X7[idx][r];
        for (int col = 0; col < 5; ++col) {
            if (rowBits & (1 << (4 - col))) {
                drawRect(x + col * px, y + r * px, px, px, color);
            }
        }
    }
}

void HUD::drawText(float x, float y, const std::string& text, float scale, const glm::vec4& color) {
    float px = scale * 1.6f;
    float charW = px * 6.0f; // 5 cols + 1 spacing

    for (size_t i = 0; i < text.size(); ++i) {
        drawGlyph(x + i * charW, y, text[i], scale, color);
    }
}

void HUD::drawBadge(float x, float y, float w, float h, const std::string& text, const glm::vec4& bgColor, const glm::vec4& textColor) {
    drawRect(x, y, w, h, bgColor);
    drawRectOutline(x, y, w, h, 1.2f, textColor * 0.65f);
    float textX = x + (w - text.size() * 9.6f) * 0.5f;
    float textY = y + (h - 11.2f) * 0.5f;
    drawText(textX, textY, text, 1.0f, textColor);
}

void HUD::drawProgressBar(float x, float y, float w, float h, float fraction, const glm::vec4& fillColor, const glm::vec4& bgColor) {
    drawRect(x, y, w, h, bgColor);
    drawRectOutline(x, y, w, h, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.25f));

    float fillW = glm::clamp(fraction, 0.0f, 1.0f) * (w - 2.0f);
    if (fillW > 0.0f) {
        drawRect(x + 1.0f, y + 1.0f, fillW, h - 2.0f, fillColor);
    }
}

void HUD::drawCompassRose(float cx, float cy, float radius, float headingDeg, float speedVal) {
    // Compass dial backdrop
    drawCircle(cx, cy, radius, 32, glm::vec4(0.06f, 0.10f, 0.16f, 0.88f));
    drawCircleOutline(cx, cy, radius, 1.8f, 32, glm::vec4(0.20f, 0.70f, 0.90f, 0.75f));
    drawCircleOutline(cx, cy, radius * 0.65f, 1.0f, 24, glm::vec4(1.0f, 1.0f, 1.0f, 0.18f));

    // Cardinal tick labels (N, E, S, W)
    float labelDist = radius * 0.80f;
    drawText(cx - 4.0f, cy - labelDist - 5.0f, "N", 0.9f, glm::vec4(0.95f, 0.30f, 0.25f, 0.95f)); // North in Red
    drawText(cx + labelDist - 3.0f, cy - 5.0f, "E", 0.8f, glm::vec4(0.85f, 0.88f, 0.92f, 0.75f));
    drawText(cx - 4.0f, cy + labelDist - 5.0f, "S", 0.8f, glm::vec4(0.85f, 0.88f, 0.92f, 0.75f));
    drawText(cx - labelDist - 6.0f, cy - 5.0f, "W", 0.8f, glm::vec4(0.85f, 0.88f, 0.92f, 0.75f));

    // Rotating Wind Direction Arrow
    float rad = glm::radians(headingDeg - 90.0f); // 0 deg is North (Up)
    drawArrow(cx, cy, rad, radius * 0.70f, 2.4f, glm::vec4(0.98f, 0.82f, 0.15f, 0.95f));

    // Center pivot dot
    drawCircle(cx, cy, 3.2f, 12, glm::vec4(0.98f, 0.82f, 0.15f, 1.0f));

    (void)speedVal;
}

void HUD::renderDashboard(
    int screenWidth, int screenHeight,
    float altitude, float verticalSpeed,
    float windSpeed, float windHeadingDeg,
    bool burnerActive,
    int cameraMode,
    int lightingMode,
    float fps
) {
    begin(screenWidth, screenHeight);

    glm::vec4 textWhite(0.95f, 0.96f, 0.98f, 1.0f);
    glm::vec4 textCyan(0.25f, 0.85f, 0.95f, 1.0f);
    glm::vec4 textGold(0.98f, 0.82f, 0.18f, 1.0f);
    glm::vec4 panelBg(0.06f, 0.09f, 0.14f, 0.82f);
    glm::vec4 panelBorder(0.18f, 0.65f, 0.85f, 0.65f);

    // ==========================================
    // Panel 1: Flight Telemetry & Altimeter (Top-Left)
    // ==========================================
    float p1X = 22.0f;
    float p1Y = 22.0f;
    float p1W = 270.0f;
    float p1H = 175.0f;

    drawRect(p1X, p1Y, p1W, p1H, panelBg);
    drawRectOutline(p1X, p1Y, p1W, p1H, 1.5f, panelBorder);
    drawRect(p1X, p1Y, p1W, 26.0f, glm::vec4(0.10f, 0.20f, 0.32f, 0.85f));
    drawText(p1X + 12.0f, p1Y + 7.0f, "[ FLIGHT TELEMETRY ]", 1.0f, textCyan);

    // Altitude Readout
    std::ostringstream ssAlt;
    ssAlt << "ALTITUDE: " << std::fixed << std::setprecision(1) << altitude << " M";
    drawText(p1X + 14.0f, p1Y + 38.0f, ssAlt.str(), 1.15f, textWhite);

    // Altimeter Gauge Bar (0m to 40m)
    float altRatio = (altitude - 4.2f) / (38.0f - 4.2f);
    drawProgressBar(p1X + 14.0f, p1Y + 58.0f, p1W - 28.0f, 10.0f, altRatio, glm::vec4(0.20f, 0.85f, 0.45f, 0.90f), glm::vec4(0.12f, 0.16f, 0.22f, 0.85f));

    // Vertical Velocity (Climb / Descent Rate)
    std::ostringstream ssVsi;
    glm::vec4 vsiCol = textCyan;
    if (verticalSpeed > 0.15f) {
        ssVsi << "VSI:  +" << std::fixed << std::setprecision(1) << verticalSpeed << " M/S (CLIMB)";
        vsiCol = glm::vec4(0.25f, 0.90f, 0.45f, 1.0f);
    } else if (verticalSpeed < -0.15f) {
        ssVsi << "VSI:  " << std::fixed << std::setprecision(1) << verticalSpeed << " M/S (DESCENT)";
        vsiCol = glm::vec4(0.98f, 0.55f, 0.15f, 1.0f);
    } else {
        ssVsi << "VSI:   0.0 M/S (LEVEL)";
    }
    drawText(p1X + 14.0f, p1Y + 78.0f, ssVsi.str(), 1.0f, vsiCol);

    // Burner Status Badge
    float badgeW = p1W - 28.0f;
    float badgeH = 28.0f;
    float badgeY = p1Y + 102.0f;

    if (burnerActive) {
        drawBadge(p1X + 14.0f, badgeY, badgeW, badgeH, "BURNER: ACTIVE [ON]", glm::vec4(0.85f, 0.35f, 0.08f, 0.92f), glm::vec4(1.0f, 0.98f, 0.92f, 1.0f));
    } else {
        drawBadge(p1X + 14.0f, badgeY, badgeW, badgeH, "BURNER: IDLE [OFF]", glm::vec4(0.16f, 0.22f, 0.30f, 0.90f), glm::vec4(0.65f, 0.75f, 0.85f, 0.90f));
    }

    drawText(p1X + 14.0f, p1Y + 142.0f, "HOTKEY: [F] TOGGLE BURNER", 0.9f, glm::vec4(0.65f, 0.72f, 0.82f, 0.80f));

    // ==========================================
    // Panel 2: Wind & Compass (Top-Right)
    // ==========================================
    float p2W = 260.0f;
    float p2H = 175.0f;
    float p2X = static_cast<float>(screenWidth) - p2W - 22.0f;
    float p2Y = 22.0f;

    drawRect(p2X, p2Y, p2W, p2H, panelBg);
    drawRectOutline(p2X, p2Y, p2W, p2H, 1.5f, panelBorder);
    drawRect(p2X, p2Y, p2W, 26.0f, glm::vec4(0.10f, 0.20f, 0.32f, 0.85f));
    drawText(p2X + 12.0f, p2Y + 7.0f, "[ WIND & HEADING ]", 1.0f, textCyan);

    // Compass dial on left of panel 2
    float compassCx = p2X + 54.0f;
    float compassCy = p2Y + 98.0f;
    drawCompassRose(compassCx, compassCy, 38.0f, windHeadingDeg, windSpeed);

    // Wind telemetry details on right of panel 2
    float infoX = p2X + 108.0f;
    drawText(infoX, p2Y + 44.0f, "SPEED:", 0.95f, textCyan);
    std::ostringstream ssSpd;
    ssSpd << std::fixed << std::setprecision(1) << windSpeed << " KM/H";
    drawText(infoX, p2Y + 60.0f, ssSpd.str(), 1.1f, textGold);

    drawText(infoX, p2Y + 84.0f, "HEADING:", 0.95f, textCyan);
    std::ostringstream ssHdg;
    ssHdg << static_cast<int>(windHeadingDeg) << " DEG";
    drawText(infoX, p2Y + 100.0f, ssHdg.str(), 1.1f, textWhite);

    std::string cardDir = "NE (NORTHEAST)";
    if (windHeadingDeg >= 337.5f || windHeadingDeg < 22.5f) cardDir = "N (NORTH)";
    else if (windHeadingDeg < 67.5f) cardDir = "NE (NORTHEAST)";
    else if (windHeadingDeg < 112.5f) cardDir = "E (EAST)";
    else if (windHeadingDeg < 157.5f) cardDir = "SE (SOUTHEAST)";
    else if (windHeadingDeg < 202.5f) cardDir = "S (SOUTH)";
    else if (windHeadingDeg < 247.5f) cardDir = "SW (SOUTHWEST)";
    else if (windHeadingDeg < 292.5f) cardDir = "W (WEST)";
    else cardDir = "NW (NORTHWEST)";

    drawText(p2X + 14.0f, p2Y + 150.0f, cardDir, 0.95f, glm::vec4(0.80f, 0.85f, 0.92f, 0.85f));

    // ==========================================
    // Panel 3: Camera & Lighting Status (Top-Center)
    // ==========================================
    float p3W = 440.0f;
    float p3H = 58.0f;
    float p3X = (static_cast<float>(screenWidth) - p3W) * 0.5f;
    float p3Y = 22.0f;

    drawRect(p3X, p3Y, p3W, p3H, panelBg);
    drawRectOutline(p3X, p3Y, p3W, p3H, 1.2f, panelBorder);

    // Active Camera Mode label
    std::string camStr = "CAMERA: [1: OVERVIEW]";
    if (cameraMode == 2) camStr = "CAMERA: [2: FOLLOW]";
    else if (cameraMode == 3) camStr = "CAMERA: [3: FREE-FLY]";
    else if (cameraMode == 4) camStr = "CAMERA: [4: BASKET POV]";

    drawText(p3X + 16.0f, p3Y + 12.0f, camStr, 1.05f, textGold);

    // Active Lighting Mode label
    std::string lightStr = "LIGHT: DAY [L]";
    if (lightingMode == 1) lightStr = "LIGHT: SUNSET [L]";
    else if (lightingMode == 2) lightStr = "LIGHT: NIGHT [L]";
    else if (lightingMode == 3) lightStr = "LIGHT: DAWN [L]";

    drawText(p3X + 270.0f, p3Y + 12.0f, lightStr, 1.05f, textCyan);

    // Secondary sub-hints
    drawText(p3X + 16.0f, p3Y + 34.0f, "SWITCH CAM: [1] [2] [3] [4]   |   TIME: [L] CYCLE", 0.85f, glm::vec4(0.68f, 0.75f, 0.85f, 0.75f));

    // ==========================================
    // Panel 4: Quick-Reference Controls Bar (Bottom Banner)
    // ==========================================
    float p4H = 34.0f;
    float p4X = 22.0f;
    float p4Y = static_cast<float>(screenHeight) - p4H - 16.0f;
    float p4W = static_cast<float>(screenWidth) - 44.0f;

    drawRect(p4X, p4Y, p4W, p4H, glm::vec4(0.05f, 0.08f, 0.12f, 0.82f));
    drawRectOutline(p4X, p4Y, p4W, p4H, 1.0f, glm::vec4(0.18f, 0.45f, 0.65f, 0.50f));

    std::string controlsHelp = "[1-4] CAMERAS   [F] BURNER FIRE   [L] DAY/NIGHT   [SPACE] PAUSE   [R] RESET   [WASD/ARROWS] FLY   [ESC] EXIT";
    float helpX = p4X + (p4W - controlsHelp.size() * 8.6f) * 0.5f;
    drawText(helpX, p4Y + 11.0f, controlsHelp, 0.90f, glm::vec4(0.85f, 0.90f, 0.96f, 0.90f));

    // FPS Display (Bottom-Right corner above controls bar)
    std::ostringstream ssFps;
    ssFps << "FPS: " << static_cast<int>(fps);
    drawText(static_cast<float>(screenWidth) - 100.0f, p4Y - 18.0f, ssFps.str(), 0.95f, glm::vec4(0.35f, 0.85f, 0.45f, 0.85f));

    end();
}
