#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Mesh.h"
#include "ModelGenerator.h"
#include "Shader.h"
#include "ShaderSources.h"
#include "HUD.h"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Window settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Camera
Camera camera(glm::vec3(0.0f, 18.0f, 44.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool rightMousePressed = false;
bool leftMousePressed = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Simulation State
bool isPaused = false;
bool burnerActive = true;
bool showHud = false; // Set to false per user request: "scene er modde thaka lekha gula bad daw"
bool isRayTracingActive = false;
float simulationTime = 0.0f;

// Physical Balloon Flight Parameters
float balloonAltitude = 4.2f;
glm::vec3 balloonPosition(0.0f, 4.2f, 0.0f);
glm::vec3 balloonVelocity(0.0f, 0.0f, 0.0f);
float basketSwayRoll = 0.0f;
float basketSwayPitch = 0.0f;

// Manual Balloon Steering Controls (Arrow Keys & WASD)
float userSteerX = 0.0f; // -1.0 (Left), +1.0 (Right)
float userSteerZ = 0.0f; // +1.0 (Forward), -1.0 (Backward)

// Wind Physics System
float windSpeed = 14.5f;       // km/h
float windHeadingDeg = 48.0f;  // Degrees from North
glm::vec3 windVector(0.0f);    // Computed normalized horizontal direction * speed

// Windmill & Environmental Rotation
float windmillAngle = 0.0f;

// Day-to-Night Lighting Modes
enum LightingMode {
    LIGHT_DAY = 0,
    LIGHT_SUNSET = 1,
    LIGHT_NIGHT = 2,
    LIGHT_DAWN = 3
};
LightingMode currentLightMode = LIGHT_DAY;

// Shading Models (Per-Fragment Phong vs Per-Vertex Gouraud)
enum ShadingModel {
    SHADING_PHONG = 0,
    SHADING_GOURAUD = 1
};
ShadingModel currentShadingMode = SHADING_PHONG;

const char* getShadingModelName(ShadingModel s) {
    switch (s) {
        case SHADING_PHONG: return "PHONG (Per-Fragment Lighting)";
        case SHADING_GOURAUD: return "GOURAUD (Per-Vertex Lighting)";
        default: return "PHONG";
    }
}

// Lighting Lerp Profile
struct LightingProfile {
    glm::vec3 sunDir;
    glm::vec3 sunColor;
    glm::vec3 ambientColor;
    glm::vec3 groundBounce;
    glm::vec3 skyColor;
    float spotIntensity;
};

LightingProfile profiles[4] = {
    // 0: LUSH GREEN DAY (Default - bright clear sky, warm daylight making all green vegetation pop)
    {
        glm::normalize(glm::vec3(0.42f, -0.88f, -0.32f)), // High daytime sun
        glm::vec3(1.0f, 0.98f, 0.94f),                    // Warm clean white daylight
        glm::vec3(0.42f, 0.50f, 0.62f),                    // Fresh open blue sky ambient fill
        glm::vec3(0.22f, 0.52f, 0.22f),                    // Lush emerald meadow ground bounce
        glm::vec3(0.44f, 0.72f, 0.96f),                    // Clear vibrant sky blue
        0.0f
    },
    // 1: VILLAGE GOLDEN SUNSET
    {
        glm::normalize(glm::vec3(0.78f, -0.22f, -0.44f)),
        glm::vec3(1.0f, 0.68f, 0.36f),
        glm::vec3(0.38f, 0.32f, 0.42f),
        glm::vec3(0.24f, 0.38f, 0.18f),
        glm::vec3(0.70f, 0.50f, 0.60f),
        1.5f
    },
    // 2: MOONLIT VILLAGE NIGHT
    {
        glm::normalize(glm::vec3(0.35f, -0.88f, 0.32f)),
        glm::vec3(0.28f, 0.38f, 0.55f),
        glm::vec3(0.08f, 0.10f, 0.16f),
        glm::vec3(0.06f, 0.12f, 0.08f),
        glm::vec3(0.04f, 0.08f, 0.16f),
        3.8f
    },
    // 3: FRESH MISTY DAWN
    {
        glm::normalize(glm::vec3(-0.75f, -0.28f, -0.55f)),
        glm::vec3(0.98f, 0.78f, 0.56f),
        glm::vec3(0.38f, 0.42f, 0.54f),
        glm::vec3(0.18f, 0.40f, 0.18f),
        glm::vec3(0.55f, 0.68f, 0.82f),
        0.5f
    }
};

glm::vec3 curSunDir = profiles[0].sunDir;
glm::vec3 curSunColor = profiles[0].sunColor;
glm::vec3 curAmbientColor = profiles[0].ambientColor;
glm::vec3 curGroundBounce = profiles[0].groundBounce;
glm::vec3 curSkyColor = profiles[0].skyColor;
float curSpotIntensity = profiles[0].spotIntensity;

// Cloud data structure
struct Cloud {
    glm::vec3 position;
    float scale;
    float speedMultiplier;
};

// Bird in flock data structure
struct Bird {
    glm::vec3 offset;
    float flapPhase;
};

// G-Buffer for Real-Time Screen-Space Ray Tracing (SSR + RTAO + Soft Shadows)
unsigned int gBufferFBO = 0;
unsigned int gColor = 0, gNormal = 0, gPosition = 0;
unsigned int gDepthRBO = 0;
int gBufferWidth = 0, gBufferHeight = 0;

void initGBuffer(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (gBufferFBO != 0 && width == gBufferWidth && height == gBufferHeight) return;

    if (gBufferFBO != 0) {
        glDeleteFramebuffers(1, &gBufferFBO);
        glDeleteTextures(1, &gColor);
        glDeleteTextures(1, &gNormal);
        glDeleteTextures(1, &gPosition);
        glDeleteRenderbuffers(1, &gDepthRBO);
    }

    gBufferWidth = width;
    gBufferHeight = height;

    glGenFramebuffers(1, &gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

    // 0: Rendered Scene Color / Albedo
    glGenTextures(1, &gColor);
    glBindTexture(GL_TEXTURE_2D, gColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gColor, 0);

    // 1: Normal (RGB) + Reflectivity (A)
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // 2: World Position (RGB) + Distance (A)
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gPosition, 0);

    // Depth Renderbuffer
    glGenRenderbuffers(1, &gDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, gDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gDepthRBO);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ERROR] G-Buffer Framebuffer is incomplete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
    initGBuffer(width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    (void)window;
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    bool isDragging = leftMousePressed || rightMousePressed;

    if (camera.mode == CAMERA_FREE_FLY && rightMousePressed) {
        camera.processMouseMovement(xoffset, yoffset);
    } else if (isDragging) {
        camera.set360Mode(true);
        camera.processMouseMovement(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window;
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightMousePressed = (action == GLFW_PRESS);
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftMousePressed = (action == GLFW_PRESS);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    camera.processMouseScroll(static_cast<float>(yoffset));
}

const char* getLightingModeName(LightingMode m) {
    switch (m) {
        case LIGHT_DAY: return "LUSH GREEN DAY";
        case LIGHT_SUNSET: return "VILLAGE SUNSET";
        case LIGHT_NIGHT: return "MOONLIT NIGHT";
        case LIGHT_DAWN: return "MISTY DAWN";
        default: return "LUSH GREEN DAY";
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera Mode Switching
    static bool key1Pressed = false;
    static bool key2Pressed = false;
    static bool key3Pressed = false;
    static bool key4Pressed = false;
    static bool spacePressed = false;
    static bool fPressed = false;
    static bool rPressed = false;
    static bool lPressed = false;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
        camera.setMode(CAMERA_OVERVIEW);
        std::cout << "[CAMERA] Switched to Mode 1: Overview\n";
        key1Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) {
        key1Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed) {
        camera.setMode(CAMERA_FOLLOW);
        std::cout << "[CAMERA] Switched to Mode 2: Follow Balloon\n";
        key2Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) {
        key2Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed) {
        camera.setMode(CAMERA_FREE_FLY);
        std::cout << "[CAMERA] Switched to Mode 3: Free-fly (WASD + QE + Arrows / Right Mouse)\n";
        key3Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) {
        key3Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !key4Pressed) {
        camera.setMode(CAMERA_BASKET_POV);
        std::cout << "[CAMERA] Switched to Mode 4: Basket POV\n";
        key4Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) {
        key4Pressed = false;
    }

    // Space: Pause / Resume
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        isPaused = !isPaused;
        std::cout << "[SIMULATION] " << (isPaused ? "PAUSED" : "RESUMED") << "\n";
        spacePressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spacePressed = false;
    }

    // F: Toggle Burner Flame
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fPressed) {
        burnerActive = !burnerActive;
        std::cout << "[BURNER] " << (burnerActive ? "FIRE ON (Ascending)" : "OFF (Idle/Descending)") << "\n";
        fPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fPressed = false;
    }

    // L: Toggle / Cycle Day-to-Night Lighting
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lPressed) {
        currentLightMode = static_cast<LightingMode>((currentLightMode + 1) % 4);
        std::cout << "[LIGHTING] Switched to " << getLightingModeName(currentLightMode) << "\n";
        lPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lPressed = false;
    }

    // M / P: Toggle Shading Model (Phong [Per-Fragment] vs Gouraud [Per-Vertex])
    static bool shadingKeyPressed = false;
    if ((glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) && !shadingKeyPressed) {
        currentShadingMode = (currentShadingMode == SHADING_PHONG) ? SHADING_GOURAUD : SHADING_PHONG;
        std::cout << "[SHADING MODEL] Switched to " << getShadingModelName(currentShadingMode) << "\n";
        shadingKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        shadingKeyPressed = false;
    }

    // T: Toggle Real-Time Ray Tracing vs OpenGL Hardware Rasterization
    static bool tPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed) {
        isRayTracingActive = !isRayTracingActive;
        std::cout << "[RENDER PIPELINE] Switched to "
                  << (isRayTracingActive ? "REAL-TIME RAY TRACING (Analytical Ray Casting, Dynamic Pond Reflections & Ray Shadows)" : "OPENGL HARDWARE RASTERIZATION (Phong & Gouraud Pipeline)")
                  << "\n";
        tPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tPressed = false;
    }

    // R: Reset Scene
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        balloonAltitude = 4.2f;
        balloonPosition = glm::vec3(0.0f, 4.2f, 0.0f);
        balloonVelocity = glm::vec3(0.0f);
        simulationTime = 0.0f;
        camera.set360Mode(false);
        std::cout << "[SCENE] Reset to Launch Position\n";
        rPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rPressed = false;
    }

    // G: Toggle 360-Degree Free Look Mode (in all camera modes)
    static bool gPressed = false;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !gPressed) {
        camera.toggle360Mode();
        std::cout << "[360 VIEW] 360-Degree Free Look "
                  << (camera.is360Active() ? "ENABLED (Drag Mouse or Q/E/Z/C to look 360 deg in any mode)" : "DISABLED (Locked to default camera tracking)")
                  << "\n";
        gPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE) {
        gPressed = false;
    }

    // H: Toggle HUD Text Overlay
    static bool hPressed = false;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hPressed) {
        showHud = !showHud;
        std::cout << "[HUD] " << (showHud ? "ENABLED" : "DISABLED") << "\n";
        hPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
        hPressed = false;
    }

    // ==========================================
    // Interactive Balloon Flight Steering (Arrow Keys & WASD)
    // ==========================================
    userSteerX = 0.0f;
    userSteerZ = 0.0f;

    // Arrow keys steer the hot air balloon in all camera modes
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        userSteerX -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        userSteerX += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        userSteerZ += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        userSteerZ -= 1.0f;

    // In Overview, Follow, or Basket POV modes, WASD can also steer the balloon
    if (camera.mode != CAMERA_FREE_FLY) {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            userSteerX -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            userSteerX += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            userSteerZ += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            userSteerZ -= 1.0f;
    } else {
        // In Free-fly spectator mode, WASD / QE controls the spectator camera
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(CAM_FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(CAM_BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(CAM_LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(CAM_RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.processKeyboard(CAM_UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.processKeyboard(CAM_DOWN, deltaTime);
    }

    // 360-degree pan & tilt look controls (Q/E yaw pan, Z/C pitch tilt) across all camera modes
    if (camera.mode != CAMERA_FREE_FLY) {
        float rotRate = 65.0f * deltaTime;
        bool keyAction = false;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera.yaw -= rotRate;
            keyAction = true;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera.yaw += rotRate;
            keyAction = true;
        }
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            camera.pitch -= rotRate;
            if (camera.pitch < -89.0f) camera.pitch = -89.0f;
            keyAction = true;
        }
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            camera.pitch += rotRate;
            if (camera.pitch > 89.0f) camera.pitch = 89.0f;
            keyAction = true;
        }
        if (keyAction) {
            camera.set360Mode(true);
            camera.updateCameraVectors();
        }
    }
}

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // 2. Create Window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hot Air Balloon 3D - Floating Sky Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 3. Initialize GLAD
    int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (!version) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "========================================================\n";
    std::cout << "  Hot Air Balloon 3D: Phase 6 Fully Operational\n";
    std::cout << "  OpenGL Version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << "\n";
    std::cout << "  Renderer:       " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Flight & Steering Controls:\n";
    std::cout << "  * ARROW KEYS (Left / Right): Steer Balloon Left / Right\n";
    std::cout << "  * ARROW KEYS (Up / Down)   : Steer Balloon Forward / Backward\n";
    std::cout << "  * F                        : Toggle Burner Flame (Ascend / Descend)\n";
    std::cout << "  * SPACE                    : Pause / Resume Simulation\n";
    std::cout << "  * 1, 2, 3, 4               : Switch Camera Modes (Overview, Follow, Free-fly, Basket POV)\n";
    std::cout << "  * G                        : Toggle 360-Degree Free Look (ON / OFF in All Camera Modes)\n";
    std::cout << "  * 360 Look Controls        : Click & Drag (Left / Right Mouse) or Q / E / Z / C (All Modes)\n";
    std::cout << "  * L                        : Cycle Day / Sunset / Night / Dawn Lighting\n";
    std::cout << "  * M / P                    : Toggle Shading Model (Phong [Per-Fragment] vs Gouraud [Per-Vertex])\n";
    std::cout << "  * T                        : Toggle Real-Time Ray Tracing (Ray Shadows & Water Reflections)\n";
    std::cout << "  * R                        : Reset Balloon to Launch Position\n";
    std::cout << "  * H                        : Toggle HUD Dashboard\n";
    std::cout << "========================================================\n";

    initGBuffer(SCR_WIDTH, SCR_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 4. Build and Compile Shaders & HUD System
    Shader sceneShader(SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER);
    Shader rayTracingShader(RAYTRACING_VERTEX_SHADER, RAYTRACING_FRAGMENT_SHADER);
    HUD hud;
    hud.init();

    // Full-screen Quad for Real-Time Ray Tracing
    float screenQuadVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };
    unsigned int screenQuadVAO, screenQuadVBO;
    glGenVertexArrays(1, &screenQuadVAO);
    glGenBuffers(1, &screenQuadVBO);
    glBindVertexArray(screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), screenQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // 5. Generate Procedural 3D Meshes
    // A. Rural Terrain & Dirt Road
    Mesh rollingTerrain = ModelGenerator::createRollingTerrain(1200.0f, 1200.0f, 240);
    Mesh dirtRoad = ModelGenerator::createCurvedDirtRoad();
    Mesh rusticLantern = ModelGenerator::createRusticLanternPost();
    Mesh waypointSign = ModelGenerator::createWaypointSignpost();
    Mesh haystackCone = ModelGenerator::createCone(2.2f, 3.2f, 16, glm::vec3(0.86f, 0.74f, 0.32f));
    Mesh haystackBase = ModelGenerator::createCylinder(2.3f, 2.2f, 0.45f, 16, glm::vec3(0.78f, 0.65f, 0.28f));

    // B. Launch Platform, Fences & Mast
    Mesh launchPlatform = ModelGenerator::createCube(16.0f, 0.4f, 16.0f, glm::vec3(0.56f, 0.38f, 0.24f));
    Mesh platformBorder = ModelGenerator::createCube(16.4f, 0.45f, 0.4f, glm::vec3(0.38f, 0.24f, 0.15f));
    Mesh fencePost = ModelGenerator::createCube(0.25f, 1.4f, 0.25f, glm::vec3(0.42f, 0.28f, 0.18f));
    Mesh fenceRail = ModelGenerator::createCube(3.8f, 0.12f, 0.12f, glm::vec3(0.48f, 0.32f, 0.20f));

    Mesh windsockPole = ModelGenerator::createCylinder(0.12f, 0.12f, 8.5f, 12, glm::vec3(0.72f, 0.72f, 0.76f));
    Mesh windsockCone = ModelGenerator::createStripedWindsock(0.50f, 0.22f, 2.4f, 16, 5);
    Mesh floodlightHead = ModelGenerator::createCone(0.42f, 0.65f, 14, glm::vec3(0.20f, 0.20f, 0.22f));
    Mesh floodlightLens = ModelGenerator::createCylinder(0.38f, 0.38f, 0.05f, 14, glm::vec3(1.0f, 0.98f, 0.85f));

    // C. Traditional Rural Landscape Elements (Lush Green Village)
    Mesh villageHut = ModelGenerator::createVillageHut();
    Mesh banyanTree = ModelGenerator::createBanyanShadeTree();
    Mesh palmTreeStd = ModelGenerator::createPalmTree(10.5f, 3.5f);
    Mesh palmTreeTall = ModelGenerator::createPalmTree(13.5f, -4.5f);
    Mesh bananaTree = ModelGenerator::createBananaTree();
    Mesh lushBush = ModelGenerator::createLushBush(1.5f);
    Mesh reedClusterDense = ModelGenerator::createReedCluster(18, 1.9f);
    Mesh reedClusterLight = ModelGenerator::createReedCluster(10, 1.4f);
    Mesh wetlandWater = ModelGenerator::createWetlandWater(140.0f, 75.0f);
    Mesh rusticBoulder = ModelGenerator::createSphere(1.1f, 10, 10, glm::vec3(0.38f, 0.36f, 0.32f));

    // D. Main Hot Air Balloon (Vibrant Rainbow Envelope + Basket)
    Mesh rainbowEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 48, 14, 0);
    Mesh equatorBelt = ModelGenerator::createBalloonEquatorBelt(4.72f, 0.08f);
    Mesh whiteSkirt = ModelGenerator::createBalloonWhiteSkirt(1.55f, 1.25f, 1.35f, 32);
    Mesh wovenBasket = ModelGenerator::createWovenBasket(2.4f, 1.8f, 2.4f);
    Mesh burnerRing = ModelGenerator::createCylinder(0.65f, 0.65f, 0.25f, 16, glm::vec3(0.28f, 0.28f, 0.32f));
    Mesh burnerFlame = ModelGenerator::createCone(0.55f, 1.4f, 16, glm::vec3(1.0f, 0.55f, 0.05f));
    Mesh riggingCable = ModelGenerator::createCylinder(0.025f, 0.025f, 3.4f, 8, glm::vec3(0.20f, 0.20f, 0.22f));

    // E. Background Balloons (Decoupled Visual Themes & Scales)
    Mesh sunsetEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 36, 12, 1);
    Mesh oceanEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 36, 12, 2);
    Mesh emeraldEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 36, 12, 3);

    // F. Clouds, Birds & Celestial Entities
    Mesh cloudCluster = ModelGenerator::createCloudCluster();
    Mesh birdBody = ModelGenerator::createBirdBody();
    Mesh birdLeftWing = ModelGenerator::createBirdWing(true);
    Mesh birdRightWing = ModelGenerator::createBirdWing(false);

    Mesh celestialDisc = ModelGenerator::createSphere(7.5f, 18, 18, glm::vec3(1.0f, 0.95f, 0.75f));
    Mesh groundShadow = ModelGenerator::createShadowDisc(1.0f, 36);

    // Clouds layout
    std::vector<Cloud> clouds = {
        {{-40.0f, 32.0f, -30.0f}, 1.3f, 1.0f},
        {{ 10.0f, 38.0f, -50.0f}, 1.6f, 1.2f},
        {{ 55.0f, 28.0f, -20.0f}, 1.1f, 0.9f},
        {{-20.0f, 42.0f,  30.0f}, 1.4f, 1.1f},
        {{ 35.0f, 35.0f,  45.0f}, 1.2f, 0.95f},
        {{-60.0f, 30.0f,  15.0f}, 1.5f, 1.05f}
    };

    // Bird flock
    std::vector<Bird> flock = {
        {{ 0.0f,  0.0f,   0.0f}, 0.0f},
        {{-2.2f, -0.4f,  -2.0f}, 0.5f},
        {{ 2.2f, -0.3f,  -2.0f}, 0.8f},
        {{-4.4f, -0.8f,  -4.0f}, 1.1f},
        {{ 4.4f, -0.7f,  -4.0f}, 1.4f}
    };

    // Multi-Homestead Rural Village Settlements (পূর্ণাঙ্গ শ্যামল গ্রাম)
    struct VillageHutInstance {
        glm::vec3 pos;
        float rotY;
        float scale;
    };
    std::vector<VillageHutInstance> villageHuts = {
        {{ 21.0f, 0.0f,  31.0f}, -28.0f, 1.00f},  // Homestead 1 (Main cottage near pond)
        {{ 38.0f, 0.0f,  68.0f},  18.0f, 1.05f},  // Homestead 2 (Farmhouse along northern road)
        {{-18.0f, 0.0f,  46.0f}, -55.0f, 0.95f},  // Homestead 3 (Cottage across the meadow trail)
        {{ 16.0f, 0.0f,  96.0f},  35.0f, 0.92f},  // Homestead 4 (North grove dwelling)
        {{-30.0f, 0.0f,  82.0f},  42.0f, 0.88f}   // Homestead 5 (West meadow homestead)
    };

    // Traditional Bengali Golden Straw Haystacks (খড়ের গাদা)
    struct HaystackInstance {
        glm::vec3 pos;
        float scale;
    };
    std::vector<HaystackInstance> haystacks = {
        {{ 27.5f, 0.0f,  34.0f}, 1.00f},
        {{ 44.0f, 0.0f,  65.0f}, 1.15f},
        {{ 46.5f, 0.0f,  69.0f}, 0.88f},
        {{-14.0f, 0.0f,  50.0f}, 1.05f},
        {{ 22.0f, 0.0f,  98.0f}, 1.10f},
        {{-25.0f, 0.0f,  86.0f}, 0.95f}
    };

    // Lush Banyan & Leafy Village Shade Trees
    struct BanyanInstance {
        glm::vec3 pos;
        float scale;
        float rotY;
    };
    std::vector<BanyanInstance> banyanTrees = {
        {{ 13.8f, 0.0f,  32.5f}, 1.18f,  15.0f}, // Overhanging cottage on the left
        {{-18.0f, 0.0f,  24.0f}, 1.25f, -35.0f}, // Across the dirt road
        {{-42.0f, 0.0f,  32.0f}, 1.35f,  45.0f}, // Far meadow
        {{ 35.0f, 0.0f,  18.0f}, 1.15f, -70.0f}, // Near the pond bank
        {{ 12.0f, 0.0f,  68.0f}, 1.30f,  25.0f}, // Background grove
        {{-26.0f, 0.0f, -18.0f}, 1.10f,  80.0f}, // Near launch platform edge
        {{ 45.0f, 0.0f,  76.0f}, 1.25f,  30.0f}, // Homestead 2 grove
        {{-24.0f, 0.0f,  56.0f}, 1.15f, -50.0f}, // Homestead 3 grove
        {{ 24.0f, 0.0f, 108.0f}, 1.30f,  40.0f}, // Homestead 4 orchard
        {{-38.0f, 0.0f,  92.0f}, 1.20f, -20.0f}, // West grove
        {{  6.0f, 0.0f, 135.0f}, 1.35f,  65.0f}  // Deep landscape shade tree
    };

    // Traditional Bengali Banana Tree Clusters (কলা বাগান)
    struct BananaInstance {
        glm::vec3 pos;
        float scale;
        float rotY;
    };
    std::vector<BananaInstance> bananaTrees = {
        // Cluster behind and to the right of Homestead 1
        {{ 27.5f, 0.0f,  33.0f}, 1.10f,  25.0f},
        {{ 28.8f, 0.0f,  35.2f}, 0.95f, -40.0f},
        {{ 26.2f, 0.0f,  36.5f}, 1.20f,  75.0f},
        // Cluster on the pond bank
        {{ 22.0f, 0.0f,  44.0f}, 1.15f, -15.0f},
        {{ 23.5f, 0.0f,  46.2f}, 1.00f,  60.0f},
        // Cluster near trail curve
        {{  9.5f, 0.0f,  24.0f}, 1.05f, -80.0f},
        {{ 11.2f, 0.0f,  25.5f}, 1.25f,  30.0f},
        // Cluster in left village garden
        {{-14.5f, 0.0f,  28.0f}, 1.10f,  45.0f},
        {{-16.0f, 0.0f,  30.2f}, 1.20f, -65.0f},
        // Cluster around Homestead 2
        {{ 34.0f, 0.0f,  66.0f}, 1.10f,  20.0f},
        {{ 35.5f, 0.0f,  68.2f}, 0.95f, -30.0f},
        // Cluster around Homestead 3
        {{-21.0f, 0.0f,  44.0f}, 1.15f,  50.0f},
        {{-22.5f, 0.0f,  46.5f}, 1.00f, -70.0f},
        // Cluster around Homestead 4
        {{ 12.0f, 0.0f,  94.0f}, 1.05f,  35.0f},
        {{ 13.5f, 0.0f,  96.5f}, 1.20f, -45.0f}
    };

    // Dense Green Bush & Shrub Clumps (গ্রামের সবুজ ঝোপঝাড়)
    struct BushInstance {
        glm::vec3 pos;
        float scale;
    };
    std::vector<BushInstance> villageBushes = {
        // Homestead 1 perimeter & garden
        {{ 16.5f, 0.0f,  30.5f}, 1.10f},
        {{ 17.5f, 0.0f,  34.0f}, 1.25f},
        {{ 25.5f, 0.0f,  28.0f}, 0.95f},
        {{ 26.8f, 0.0f,  30.0f}, 1.15f},
        // Along footpath
        {{  4.8f, 0.0f,  15.0f}, 1.05f},
        {{  8.5f, 0.0f,  20.0f}, 1.20f},
        {{ 13.5f, 0.0f,  26.0f}, 1.10f},
        // Around launchpad perimeter
        {{ -9.2f, 0.0f,   2.0f}, 1.15f},
        {{  9.2f, 0.0f,   2.0f}, 1.00f},
        {{ -8.5f, 0.0f,  -8.5f}, 1.20f},
        // Around banyan trees
        {{-17.0f, 0.0f,  22.5f}, 1.30f},
        {{-19.5f, 0.0f,  25.0f}, 1.10f},
        {{ 36.5f, 0.0f,  16.5f}, 1.25f},
        // Water edge
        {{ 30.0f, 0.0f,  40.0f}, 1.15f},
        {{ 40.0f, 0.0f,  36.0f}, 1.20f},
        // Homestead 2 & 3 perimeter
        {{ 32.0f, 0.0f,  64.0f}, 1.15f},
        {{ 42.0f, 0.0f,  72.0f}, 1.20f},
        {{-15.0f, 0.0f,  42.0f}, 1.10f},
        {{-22.0f, 0.0f,  50.0f}, 1.25f},
        // Homestead 4 & 5 perimeter
        {{ 12.0f, 0.0f,  90.0f}, 1.15f},
        {{ 20.0f, 0.0f,  98.0f}, 1.20f},
        {{-26.0f, 0.0f,  78.0f}, 1.10f}
    };

    // Coconut Palms Scattered in Scenic Groves & Field Borders
    struct HorizonPalm {
        glm::vec3 pos;
        float scale;
        float rotY;
        bool isTall;
    };
    std::vector<HorizonPalm> horizonPalms = {
        // Groves along pond and east meadows
        {{ 55.0f, 0.0f,  32.0f}, 1.15f,  20.0f, true},
        {{ 62.0f, 0.0f,  40.0f}, 0.95f, -45.0f, false},
        {{ 58.0f, 0.0f,  52.0f}, 1.05f,  45.0f, true},
        {{ 70.0f, 0.0f,  58.0f}, 0.90f, -35.0f, false},
        {{ 65.0f, 0.0f,  82.0f}, 1.20f,  30.0f, true},
        {{ 78.0f, 0.0f,  95.0f}, 1.10f, -60.0f, false},
        // West village border groves
        {{-35.0f, 0.0f,  38.0f}, 1.15f,  35.0f, true},
        {{-48.0f, 0.0f,  52.0f}, 1.00f, -25.0f, false},
        {{-52.0f, 0.0f,  70.0f}, 1.25f,  55.0f, true},
        {{-60.0f, 0.0f,  88.0f}, 1.10f, -40.0f, false},
        {{-42.0f, 0.0f, 105.0f}, 1.20f,  20.0f, true},
        // Distant north horizon groves
        {{-20.0f, 0.0f, 130.0f}, 1.30f,  15.0f, true},
        {{  0.0f, 0.0f, 135.0f}, 1.15f, -30.0f, false},
        {{ 18.0f, 0.0f, 132.0f}, 1.25f,  50.0f, true},
        {{ 35.0f, 0.0f, 128.0f}, 1.05f, -65.0f, false},
        {{ 52.0f, 0.0f, 134.0f}, 1.20f,  25.0f, true},
        // South meadows near launchpad
        {{-45.0f, 0.0f, -35.0f}, 1.10f,  30.0f, true},
        {{-25.0f, 0.0f, -45.0f}, 0.95f, -20.0f, false},
        {{ 35.0f, 0.0f, -40.0f}, 1.20f,  45.0f, true},
        {{ 55.0f, 0.0f, -30.0f}, 1.05f, -50.0f, false}
    };

    // Dense Foreground & Trailside Reed Clusters
    struct ReedPos {
        glm::vec3 pos;
        float scale;
        float rotY;
        bool isDense;
    };
    std::vector<ReedPos> reedPatches = {
        // Foreground along camera and trail
        {{  4.0f, 0.0f,  12.0f}, 1.25f,  35.0f, true},
        {{  6.5f, 0.0f,  14.0f}, 1.05f, -60.0f, false},
        {{  2.5f, 0.0f,  16.0f}, 1.15f,  80.0f, true},
        {{  8.0f, 0.0f,  18.0f}, 1.30f, -20.0f, true},
        {{ 10.5f, 0.0f,  21.0f}, 0.95f,  45.0f, false},
        {{  3.5f, 0.0f,  24.0f}, 1.20f, -75.0f, true},
        {{  7.0f, 0.0f,  27.0f}, 1.10f,  15.0f, false},
        {{ 12.0f, 0.0f,  29.0f}, 1.35f, -40.0f, true},
        // Around the village hut and tree base
        {{ 14.0f, 0.0f,  35.0f}, 1.15f,  50.0f, true},
        {{ 16.0f, 0.0f,  31.0f}, 1.25f, -15.0f, true},
        {{ 13.0f, 0.0f,  27.0f}, 1.00f,  70.0f, false},
        {{ 26.5f, 0.0f,  33.0f}, 1.10f, -85.0f, true},
        {{ 28.0f, 0.0f,  28.0f}, 1.20f,  20.0f, false},
        {{ 27.0f, 0.0f,  36.0f}, 0.90f, -45.0f, true},
        // Along the wetland water shore
        {{ 25.0f, 0.0f,  45.0f}, 1.30f,  65.0f, true},
        {{ 32.0f, 0.0f,  42.0f}, 1.15f, -30.0f, true},
        {{ 38.0f, 0.0f,  44.0f}, 1.25f,  10.0f, false},
        {{ 45.0f, 0.0f,  40.0f}, 1.35f, -55.0f, true},
        {{ 52.0f, 0.0f,  38.0f}, 1.10f,  40.0f, false},
        {{ 35.0f, 0.0f,  22.0f}, 1.20f, -70.0f, true},
        {{ 42.0f, 0.0f,  20.0f}, 1.05f,  25.0f, false},
        // Left meadows
        {{-12.0f, 0.0f,  18.0f}, 1.10f,  30.0f, true},
        {{-18.0f, 0.0f,  24.0f}, 1.25f, -40.0f, false},
        {{-15.0f, 0.0f,  32.0f}, 1.15f,  60.0f, true},
        {{-22.0f, 0.0f,  28.0f}, 1.30f, -25.0f, true}
    };

    std::vector<glm::vec3> boulderPositions = {
        { 16.0f, 0.6f, -10.0f}, {-12.0f, 0.5f,  12.0f},
        { 24.0f, 0.7f,  16.0f}, {-26.0f, 0.8f, -14.0f}
    };

    // Picturesque Countryside Roadside Props
    struct LanternPostInstance {
        glm::vec3 pos;
        float rotY;
    };
    std::vector<LanternPostInstance> lanternPosts = {
        {{-21.5f, 0.0f, -11.0f},  45.0f}, // Foreground scenic entrance bend (in Camera 1 view!)
        {{ -3.2f, 0.0f,   8.8f}, -15.0f}, // Launchpad entrance gate
        {{ 12.2f, 0.0f,  29.5f},  55.0f}, // Cottage 1 / Banyan bend
        {{ 23.5f, 0.0f,  48.0f},  35.0f}, // Village pond bank overlook
        {{ 33.5f, 0.0f,  74.0f}, -40.0f}  // Homestead 2 & haystacks bend
    };
    glm::vec3 waypointSignPos(5.8f, 0.0f, 17.5f);
    float waypointSignRot = 25.0f;

    // Frame counter
    double lastTitleUpdate = 0.0;
    int frameCount = 0;
    float currentFps = 60.0f;

    // Decoupled Background Balloons State
    glm::vec3 bg1Pos(88.0f, 56.0f, 58.0f);
    float bg1Scale = 0.68f;
    float bg1Roll = 0.0f, bg1Pitch = 0.0f;

    glm::vec3 bg2Pos(-78.0f, 74.0f, 72.0f);
    float bg2Scale = 0.56f;
    float bg2Roll = 0.0f, bg2Pitch = 0.0f;

    glm::vec3 bg3Pos(28.0f, 98.0f, 165.0f);
    float bg3Scale = 0.46f;
    float bg3Roll = 0.0f, bg3Pitch = 0.0f;

    // 6. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (!isPaused) {
            simulationTime += deltaTime;

            // ==========================================
            // 1. Dynamic Wind Vector Engine
            // ==========================================
            windSpeed = 14.0f + 4.2f * std::sin(simulationTime * 0.25f) + 2.0f * std::cos(simulationTime * 0.65f);
            windHeadingDeg = 48.0f + 8.5f * std::sin(simulationTime * 0.18f);
            float windHeadingRad = glm::radians(windHeadingDeg);
            windVector = glm::vec3(std::cos(windHeadingRad), 0.0f, std::sin(windHeadingRad)) * (windSpeed * 0.28f);

            // ==========================================
            // 2. Compute Decoupled Background Balloon Trajectories
            // ==========================================
            // Relocated to East, West, and North skies to avoid conflict corridor with hero balloon
            float bg1Alt = 56.0f + 3.2f * std::sin(simulationTime * 0.32f + 1.2f);
            float bg1X   = 88.0f + 6.0f * std::sin(simulationTime * 0.12f);
            float bg1Z   = 58.0f + 4.5f * std::cos(simulationTime * 0.10f);
            bg1Pos       = glm::vec3(bg1X, bg1Alt, bg1Z);
            bg1Roll      = 2.2f * std::sin(simulationTime * 1.6f);
            bg1Pitch     = 1.6f * std::cos(simulationTime * 1.3f);

            float bg2Alt = 74.0f + 3.8f * std::cos(simulationTime * 0.26f + 0.8f);
            float bg2X   = -78.0f + 5.5f * std::cos(simulationTime * 0.13f + 1.0f);
            float bg2Z   = 72.0f + 4.0f * std::sin(simulationTime * 0.11f + 0.5f);
            bg2Pos       = glm::vec3(bg2X, bg2Alt, bg2Z);
            bg2Roll      = -2.0f * std::cos(simulationTime * 1.5f);
            bg2Pitch     = 1.5f * std::sin(simulationTime * 1.2f);

            float bg3Alt = 98.0f + 4.2f * std::sin(simulationTime * 0.20f + 2.4f);
            float bg3X   = 28.0f + 7.0f * std::sin(simulationTime * 0.08f + 2.0f);
            float bg3Z   = 165.0f + 6.0f * std::cos(simulationTime * 0.07f + 1.2f);
            bg3Pos       = glm::vec3(bg3X, bg3Alt, bg3Z);
            bg3Roll      = 1.5f * std::sin(simulationTime * 1.2f);
            bg3Pitch     = 1.1f * std::cos(simulationTime * 1.0f);

            // ==========================================
            // 3. Aerodynamic Balloon Ascent & Drift Physics (Hero Balloon)
            // ==========================================
            if (burnerActive) {
                // High-performance hot air buoyancy lift with atmospheric ceiling cushioning
                float targetAscentRate = 2.8f;
                float altLimit = 150.0f; // Expanded ceiling allowing high-altitude panoramic exploration
                float buoyancyFactor = glm::clamp(1.0f - (balloonAltitude - 115.0f) / 40.0f, 0.20f, 1.0f);
                balloonVelocity.y += (targetAscentRate * buoyancyFactor - balloonVelocity.y) * 0.85f * deltaTime;
                balloonAltitude += balloonVelocity.y * deltaTime;
                if (balloonAltitude > altLimit) {
                    balloonAltitude = altLimit;
                    balloonVelocity.y = 0.0f;
                }
            } else {
                // Cooling descent with steady terminal velocity
                float targetDescentRate = -2.0f;
                balloonVelocity.y += (targetDescentRate - balloonVelocity.y) * 0.70f * deltaTime;
                balloonAltitude += balloonVelocity.y * deltaTime;
                if (balloonAltitude < 4.2f) {
                    balloonAltitude = 4.2f;
                    balloonVelocity.y = 0.0f;
                }
            }

            // Horizontal wind drag with gentle altitude wind shear and interactive flight steering (Arrow Keys)
            float maxSteerSpeed = 9.0f; // m/s (~32.4 km/h)
            float altWindShear = 1.0f + 0.75f * glm::clamp((balloonAltitude - 4.2f) / 140.0f, 0.0f, 1.0f);
            float driftTargetX = (windVector.x * altWindShear) + (userSteerX * maxSteerSpeed);
            float driftTargetZ = (windVector.z * altWindShear) + (userSteerZ * maxSteerSpeed);

            // Responsive steering acceleration when user is actively steering; smooth aerodynamic inertia when coasting
            bool isSteeringActive = (std::abs(userSteerX) > 0.01f || std::abs(userSteerZ) > 0.01f);
            float steerResponse = isSteeringActive ? 2.5f : 0.65f;

            balloonVelocity.x += (driftTargetX - balloonVelocity.x) * steerResponse * deltaTime;
            balloonVelocity.z += (driftTargetZ - balloonVelocity.z) * steerResponse * deltaTime;

            balloonPosition.x += balloonVelocity.x * deltaTime;
            balloonPosition.z += balloonVelocity.z * deltaTime;
            balloonPosition.y = balloonAltitude;

            // Expansive terrain flight boundaries (keeps balloon inside the 1200m scenic world)
            balloonPosition.x = glm::clamp(balloonPosition.x, -380.0f, 380.0f);
            balloonPosition.z = glm::clamp(balloonPosition.z, -380.0f, 380.0f);

            // 3D Aerodynamic Collision Avoidance & Slipstream Repulsion
            // Ensures hero balloon and background balloons never intersect, overlap, or clip
            auto resolveBalloonConflict = [&](const glm::vec3& otherPos, float minSafeDist) {
                glm::vec3 diff = balloonPosition - otherPos;
                float dist = glm::length(diff);
                if (dist < minSafeDist && dist > 0.0001f) {
                    glm::vec3 repelDir = glm::normalize(diff);
                    float overlap = minSafeDist - dist;
                    balloonPosition += repelDir * (overlap * 3.5f * deltaTime);
                    balloonVelocity += repelDir * (overlap * 4.0f * deltaTime);
                    balloonAltitude = balloonPosition.y;
                }
            };

            resolveBalloonConflict(bg1Pos, 16.0f);
            resolveBalloonConflict(bg2Pos, 14.0f);
            resolveBalloonConflict(bg3Pos, 12.0f);

            // Multi-Axis Basket Pendulum Sway Physics (dynamically tilts with steering forces)
            float swayFreq = 2.3f;
            float naturalDamping = 0.95f;
            float windGustForce = (windSpeed / 15.0f);
            basketSwayRoll = glm::clamp((3.4f * std::sin(simulationTime * swayFreq) + balloonVelocity.x * 2.2f) * windGustForce * naturalDamping, -18.0f, 18.0f);
            basketSwayPitch = glm::clamp((2.6f * std::cos(simulationTime * (swayFreq * 0.88f)) + balloonVelocity.z * 2.2f) * windGustForce * naturalDamping, -18.0f, 18.0f);

            // ==========================================
            // 4. Environmental Rotations & Drifts
            // ==========================================
            // Windmill rotation speed directly driven by wind speed
            windmillAngle += (windSpeed * 3.4f) * deltaTime;
            if (windmillAngle > 360.0f) windmillAngle -= 360.0f;

            // Clouds drift with wind vector
            for (auto& c : clouds) {
                c.position += windVector * (c.speedMultiplier * 0.4f) * deltaTime;
                if (c.position.x > 320.0f) c.position.x = -320.0f;
                if (c.position.z > 320.0f) c.position.z = -320.0f;
            }
        }

        // Smooth Lighting Transitions
        float blendSpeed = 2.4f * deltaTime;
        LightingProfile targetProf = profiles[currentLightMode];
        curSunDir = glm::normalize(glm::mix(curSunDir, targetProf.sunDir, blendSpeed));
        curSunColor = glm::mix(curSunColor, targetProf.sunColor, blendSpeed);
        curAmbientColor = glm::mix(curAmbientColor, targetProf.ambientColor, blendSpeed);
        curGroundBounce = glm::mix(curGroundBounce, targetProf.groundBounce, blendSpeed);
        curSkyColor = glm::mix(curSkyColor, targetProf.skyColor, blendSpeed);
        curSpotIntensity = glm::mix(curSpotIntensity, targetProf.spotIntensity, blendSpeed);

        processInput(window);
        camera.update(deltaTime, balloonPosition, basketSwayRoll, basketSwayPitch);

        // Update Window Title with Live Telemetry
        frameCount++;
        if (currentFrame - lastTitleUpdate >= 0.25) {
            float fps = frameCount / static_cast<float>(currentFrame - lastTitleUpdate);
            currentFps = fps;
            std::ostringstream ss;
            ss << "Hot Air Balloon 3D | Alt: " << std::fixed << std::setprecision(1) << balloonPosition.y << "m"
               << " | Pipeline: [" << (isRayTracingActive ? "RAY TRACED (T)" : "RASTER (T)") << "]"
               << " | Shading: [" << (currentShadingMode == SHADING_PHONG ? "PHONG (M/P)" : "GOURAUD (M/P)") << "]"
               << " | Steer: [Arrows]"
               << " | Burner: [" << (burnerActive ? "FIRE (F)" : "OFF (F)") << "]"
               << " | 360: [" << (camera.is360Active() ? "ON (G)" : "OFF (G)") << "]"
               << " | Cam: " << camera.getModeName()
               << " | " << getLightingModeName(currentLightMode) << " [L]"
               << " | FPS: " << static_cast<int>(fps);
            glfwSetWindowTitle(window, ss.str().c_str());
            frameCount = 0;
            lastTitleUpdate = currentFrame;
        }

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspectRatio = (fbHeight > 0) ? (static_cast<float>(fbWidth) / static_cast<float>(fbHeight)) : (static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT));

        // Ensure G-Buffer dimensions match current window framebuffer
        initGBuffer(fbWidth, fbHeight);

        // If Ray Tracing mode is active, render scene into G-Buffer FBO; else render directly to backbuffer
        if (isRayTracingActive) {
            glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glViewport(0, 0, fbWidth, fbHeight);

        // Dynamic Sky Color Clear
        glClearColor(curSkyColor.r, curSkyColor.g, curSkyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use();
        sceneShader.setFloat("uReflectivity", 0.04f);

        glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 view = camera.getViewMatrix();
        sceneShader.setMat4("uProjection", projection);
        sceneShader.setMat4("uView", view);
        sceneShader.setVec3("uViewPos", camera.position);
        sceneShader.setInt("uShadingModel", static_cast<int>(currentShadingMode));

        // Set Light Uniforms
        sceneShader.setVec3("uDirLightDir", curSunDir);
        sceneShader.setVec3("uDirLightColor", curSunColor);
        sceneShader.setVec3("uAmbientColor", curAmbientColor);
        sceneShader.setVec3("uGroundBounceColor", curGroundBounce);

        // Point Light (Burner Flame inside balloon)
        glm::vec3 burnerPos = balloonPosition + glm::vec3(0.0f, 1.3f, 0.0f);
        float flameFlicker = 1.0f + 0.24f * std::sin(simulationTime * 22.0f) * std::cos(simulationTime * 31.0f);
        sceneShader.setVec3("uPointLightPos", burnerPos);
        sceneShader.setVec3("uPointLightColor", glm::vec3(1.0f, 0.62f, 0.12f));
        sceneShader.setFloat("uPointLightIntensity", burnerActive ? (2.4f * flameFlicker) : 0.2f);

        // Spotlight (Launch-Pad Mast Night Light)
        glm::vec3 mastPos(9.8f, 0.0f, -9.8f);
        glm::vec3 spotLightPos = mastPos + glm::vec3(-0.35f, 8.2f, 0.35f);
        glm::vec3 spotLightDir = glm::normalize(glm::vec3(-0.75f, -1.0f, 0.75f));
        sceneShader.setVec3("uSpotLightPos", spotLightPos);
        sceneShader.setVec3("uSpotLightDir", spotLightDir);
        sceneShader.setVec3("uSpotLightColor", glm::vec3(1.0f, 0.96f, 0.82f));
        sceneShader.setFloat("uSpotLightCutOff", std::cos(glm::radians(34.0f)));
        sceneShader.setFloat("uSpotLightOuterCutOff", std::cos(glm::radians(48.0f)));
        sceneShader.setFloat("uSpotLightIntensity", curSpotIntensity);

        sceneShader.setVec3("uSkyColor", curSkyColor);
        sceneShader.setFloat("uFogDensity", 1.0f);

        sceneShader.setFloat("uSpecularStrength", 0.40f);
        sceneShader.setFloat("uShininess", 32.0f);
        sceneShader.setFloat("uAlpha", 1.0f);
        sceneShader.setFloat("uEmissive", 0.0f);

        glm::mat4 model(1.0f);

        // ==========================================
        // 1. Draw Celestial Sun / Moon Disc
        // ==========================================
        sceneShader.setFloat("uEmissive", 1.0f);
        sceneShader.setFloat("uFogDensity", 0.0f);

        // Sun disc
        glm::vec3 celestialPos = camera.position - curSunDir * 160.0f;
        model = glm::translate(glm::mat4(1.0f), celestialPos);
        sceneShader.setMat4("uModel", model);
        celestialDisc.draw();

        sceneShader.setFloat("uEmissive", 0.0f);
        sceneShader.setFloat("uFogDensity", 1.0f);

        // ==========================================
        // 2. Draw Rural Meadow & Dirt Road (Realistic Procedural Texturing)
        // ==========================================
        model = glm::mat4(1.0f);
        sceneShader.setMat4("uModel", model);
        sceneShader.setFloat("uSpecularStrength", 0.0f);
        sceneShader.setFloat("uShininess", 1.0f);

        // Textured Meadow Ground
        sceneShader.setInt("uMaterialType", 1);
        rollingTerrain.draw();

        // Textured Curved Dirt Road
        sceneShader.setInt("uMaterialType", 2);
        dirtRoad.draw();

        // Reset Material Type
        sceneShader.setInt("uMaterialType", 0);

        // ==========================================
        // 3. Draw Dynamic Ground Shadows (Props, Settlements, Foliage & Balloons)
        // ==========================================
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Soft overlapping shadows without depth-fighting
        sceneShader.setInt("uMaterialType", 3); // Soft feathered shadow shader (cubic penumbra falloff)
        sceneShader.setFloat("uSpecularStrength", 0.0f);
        sceneShader.setFloat("uShininess", 1.0f);

        float sunDenominator = std::max(-curSunDir.y, 0.18f);
        float sunStretch = std::clamp(1.0f / sunDenominator, 1.0f, 2.0f);

        float timeOfDayShadowAlpha = 0.28f;
        if (currentLightMode == LIGHT_NIGHT) timeOfDayShadowAlpha = 0.10f;
        else if (currentLightMode == LIGHT_SUNSET) timeOfDayShadowAlpha = 0.34f;
        else if (currentLightMode == LIGHT_DAWN) timeOfDayShadowAlpha = 0.22f;

        auto drawSoftGroundShadow = [&](const glm::vec3& objPos, float emitterHeight, float radX, float radZ, float baseAlpha) {
            if (curSunDir.y < -0.05f) {
                float tProj = emitterHeight / sunDenominator;
                float shadX = objPos.x + tProj * curSunDir.x;
                float shadZ = objPos.z + tProj * curSunDir.z;
                float groundY = ModelGenerator::getTerrainHeight(shadX, shadZ) + 0.035f;

                glm::mat4 sModel = glm::translate(glm::mat4(1.0f), glm::vec3(shadX, groundY, shadZ));
                sModel = glm::scale(sModel, glm::vec3(radX, 1.0f, radZ));
                sceneShader.setMat4("uModel", sModel);
                sceneShader.setFloat("uAlpha", baseAlpha * (timeOfDayShadowAlpha / 0.28f));
                groundShadow.draw();
            }
        };

        // A. Launchpad Platform & Mast
        drawSoftGroundShadow(glm::vec3(curSunDir.x * 0.35f, 0.0f, curSunDir.z * 0.35f), 0.2f, 8.4f, 8.4f, 0.20f);
        drawSoftGroundShadow(mastPos, 3.5f, 0.7f, 1.4f * sunStretch, 0.22f);

        // B. Large Spreading Banyan Shade Trees (One natural canopy shadow per tree)
        for (const auto& bt : banyanTrees) {
            float radX = 4.8f * bt.scale;
            float radZ = 4.8f * bt.scale * (1.0f + 0.15f * sunStretch);
            drawSoftGroundShadow(bt.pos, 5.2f * bt.scale, radX, radZ, 0.28f);
        }

        // C. Coconut Palm Trees (Directional crown shadow + small trunk base shadow)
        for (const auto& palm : horizonPalms) {
            drawSoftGroundShadow(palm.pos, 0.4f * palm.scale, 0.7f * palm.scale, 0.7f * palm.scale, 0.22f);
            float crownH = (palm.isTall ? 11.0f : 9.0f) * palm.scale;
            float cRad = 2.4f * palm.scale;
            drawSoftGroundShadow(palm.pos, crownH, cRad, cRad * (1.0f + 0.18f * sunStretch), 0.24f);
        }

        // D. Village Cottages / Homesteads
        for (const auto& hut : villageHuts) {
            float hRadX = 3.6f * hut.scale;
            float hRadZ = 3.2f * hut.scale * (1.0f + 0.12f * sunStretch);
            drawSoftGroundShadow(hut.pos, 2.2f * hut.scale, hRadX, hRadZ, 0.26f);
        }

        // E. Traditional Golden Straw Haystacks
        for (const auto& hs : haystacks) {
            float sRad = 2.0f * hs.scale;
            drawSoftGroundShadow(hs.pos, 1.6f * hs.scale, sRad, sRad * (1.0f + 0.12f * sunStretch), 0.25f);
        }

        // F. Weathered Boulders & Rocks
        for (size_t i = 0; i < boulderPositions.size(); ++i) {
            float bScale = 0.75f + (i % 3) * 0.25f;
            drawSoftGroundShadow(boulderPositions[i], 0.3f * bScale, 1.0f * bScale, 1.0f * bScale, 0.28f);
        }

        // G. Banana Tree Clusters
        for (const auto& bn : bananaTrees) {
            drawSoftGroundShadow(bn.pos, 2.0f * bn.scale, 1.6f * bn.scale, 1.6f * bn.scale, 0.20f);
        }

        // Roadside Lantern Posts & Waypoint Sign
        for (const auto& lp : lanternPosts) {
            drawSoftGroundShadow(lp.pos, 2.6f, 0.65f, 0.65f * (1.0f + 0.15f * sunStretch), 0.22f);
        }
        drawSoftGroundShadow(waypointSignPos, 2.2f, 0.55f, 0.55f * (1.0f + 0.15f * sunStretch), 0.22f);

        // H. Dynamic Ground Shadows for All Hot Air Balloons (Proportional to altitude)
        auto drawBalloonShadow = [&](const glm::vec3& bPos, float bScale) {
            if (curSunDir.y < -0.1f && bPos.y < 95.0f) {
                float tShadow = -(bPos.y - 0.06f) / curSunDir.y;
                float shadowX = bPos.x + tShadow * curSunDir.x;
                float shadowZ = bPos.z + tShadow * curSunDir.z;
                float groundY = ModelGenerator::getTerrainHeight(shadowX, shadowZ) + 0.035f;

                float altFactor = std::clamp(1.0f - (bPos.y / 95.0f), 0.0f, 1.0f);
                float shadowScale = bScale * (3.6f + 0.02f * bPos.y);
                float shadowAlpha = glm::clamp(0.36f * altFactor * bScale, 0.0f, 0.38f);

                if (currentLightMode == LIGHT_NIGHT) {
                    shadowAlpha *= 0.30f;
                }

                glm::mat4 sModel = glm::translate(glm::mat4(1.0f), glm::vec3(shadowX, groundY, shadowZ));
                sModel = glm::scale(sModel, glm::vec3(shadowScale, 1.0f, shadowScale * (1.0f + 0.10f * sunStretch)));
                sceneShader.setMat4("uModel", sModel);
                sceneShader.setFloat("uAlpha", shadowAlpha);
                groundShadow.draw();
            }
        };

        drawBalloonShadow(balloonPosition, 1.0f);
        drawBalloonShadow(bg1Pos, bg1Scale);
        drawBalloonShadow(bg2Pos, bg2Scale);
        drawBalloonShadow(bg3Pos, bg3Scale);

        glDepthMask(GL_TRUE); // Restore depth buffer writes
        sceneShader.setInt("uMaterialType", 0);
        sceneShader.setFloat("uAlpha", 1.0f);
        sceneShader.setFloat("uSpecularStrength", 0.40f);

        // ==========================================
        // 4. Draw Launch Platform, Fences & Mast
        // ==========================================
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f));
        sceneShader.setMat4("uModel", model);
        launchPlatform.draw();

        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.25f, -8.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(8.0f, 0.25f, 0.0f)), glm::radians(90.0f), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f, 0.25f, 0.0f)), glm::radians(90.0f), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.5f, 0.25f, 8.0f));
        model = glm::scale(model, glm::vec3(0.35f, 1.0f, 1.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::translate(glm::mat4(1.0f), glm::vec3(5.5f, 0.25f, 8.0f));
        model = glm::scale(model, glm::vec3(0.35f, 1.0f, 1.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        float platformPosts[8][2] = {
            {-7.8f, -7.8f}, {0.0f, -7.8f}, {7.8f, -7.8f},
            {-7.8f,  0.0f},                {7.8f,  0.0f},
            {-7.8f,  7.8f}, {-2.5f, 7.8f}, {2.5f, 7.8f}
        };
        for (int i = 0; i < 8; ++i) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(platformPosts[i][0], 0.9f, platformPosts[i][1]));
            sceneShader.setMat4("uModel", model);
            fencePost.draw();
        }

        float platformRails[5][4] = {
            {-3.9f, -7.8f, 0.0f, 0.0f}, {3.9f, -7.8f, 0.0f, 0.0f},
            {-7.8f, -3.9f, 90.0f, 0.0f}, {-7.8f, 3.9f, 90.0f, 0.0f},
            { 7.8f, -3.9f, 90.0f, 0.0f}
        };
        for (int i = 0; i < 5; ++i) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(platformRails[i][0], 1.0f, platformRails[i][1]));
            if (platformRails[i][2] != 0.0f) model = glm::rotate(model, glm::radians(platformRails[i][2]), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            fenceRail.draw();

            model = glm::translate(glm::mat4(1.0f), glm::vec3(platformRails[i][0], 0.5f, platformRails[i][1]));
            if (platformRails[i][2] != 0.0f) model = glm::rotate(model, glm::radians(platformRails[i][2]), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            fenceRail.draw();
        }

        // Windsock mast
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 4.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windsockPole.draw();

        // Spotlight fixture
        model = glm::translate(glm::mat4(1.0f), spotLightPos);
        model = glm::rotate(model, glm::radians(135.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        floodlightHead.draw();

        if (curSpotIntensity > 0.1f) {
            model = glm::translate(glm::mat4(1.0f), spotLightPos + glm::vec3(-0.1f, -0.1f, 0.1f));
            sceneShader.setMat4("uModel", model);
            sceneShader.setFloat("uEmissive", 1.0f);
            floodlightLens.draw();
            sceneShader.setFloat("uEmissive", 0.0f);
        }

        // Windsock physically reacting to wind heading and flutter
        float windHeading = windHeadingDeg;
        float baseTilt = glm::mix(105.0f, 74.0f, glm::clamp(windSpeed / 25.0f, 0.0f, 1.0f));
        float sockFlutter = baseTilt + 4.5f * std::sin(simulationTime * (8.0f + windSpeed * 0.4f));

        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 8.3f, 0.0f));
        model = glm::rotate(model, glm::radians(windHeading), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(sockFlutter), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        windsockCone.draw();

        // ==========================================
        // 5. Draw Lush Green Rural Village Environment (সবুজ শ্যামল গ্রাম)
        // ==========================================
        // A. Traditional Village Cottages / Huts with Terracotta Hip Roof
        for (const auto& hut : villageHuts) {
            model = glm::translate(glm::mat4(1.0f), hut.pos);
            model = glm::rotate(model, glm::radians(hut.rotY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(hut.scale));
            sceneShader.setMat4("uModel", model);
            villageHut.draw();
        }

        // B. Traditional Golden Straw Haystacks (খড়ের গাদা)
        for (const auto& hs : haystacks) {
            model = glm::translate(glm::mat4(1.0f), hs.pos);
            model = glm::scale(model, glm::vec3(hs.scale));
            sceneShader.setMat4("uModel", model);
            haystackBase.draw();
            model = glm::translate(model, glm::vec3(0.0f, 0.45f, 0.0f));
            sceneShader.setMat4("uModel", model);
            haystackCone.draw();
        }

        // B. Large Spreading Banyan / Mango Shade Trees (Shady Village Canopy)
        for (const auto& bt : banyanTrees) {
            model = glm::translate(glm::mat4(1.0f), bt.pos);
            model = glm::rotate(model, glm::radians(bt.rotY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(bt.scale, bt.scale, bt.scale));
            sceneShader.setMat4("uModel", model);
            banyanTree.draw();
        }

        // C. Traditional Bengali Banana Trees (কলা গাছ)
        for (const auto& bn : bananaTrees) {
            model = glm::translate(glm::mat4(1.0f), bn.pos);
            model = glm::rotate(model, glm::radians(bn.rotY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(bn.scale, bn.scale, bn.scale));
            sceneShader.setMat4("uModel", model);
            bananaTree.draw();
        }

        // D. Dense Green Bushes & Shrubbery (গ্রামের সবুজ ঝোপঝাড়)
        for (const auto& bu : villageBushes) {
            model = glm::translate(glm::mat4(1.0f), bu.pos);
            model = glm::scale(model, glm::vec3(bu.scale, bu.scale, bu.scale));
            sceneShader.setMat4("uModel", model);
            lushBush.draw();
        }

        // E. Village Pond Water Mirror (Fresh Blue-Green Reflective Water)
        glm::vec3 waterPos(46.0f, -0.02f, 36.0f);
        model = glm::translate(glm::mat4(1.0f), waterPos);
        model = glm::rotate(model, glm::radians(8.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        sceneShader.setFloat("uSpecularStrength", 0.85f);
        sceneShader.setFloat("uShininess", 64.0f);
        sceneShader.setFloat("uReflectivity", 0.85f);
        wetlandWater.draw();
        sceneShader.setFloat("uReflectivity", 0.04f);
        sceneShader.setFloat("uSpecularStrength", 0.40f);
        sceneShader.setFloat("uShininess", 32.0f);

        // F. Horizon Tropical Coconut Palm Groves
        for (const auto& palm : horizonPalms) {
            model = glm::translate(glm::mat4(1.0f), palm.pos);
            model = glm::rotate(model, glm::radians(palm.rotY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(palm.scale, palm.scale, palm.scale));
            sceneShader.setMat4("uModel", model);
            if (palm.isTall) {
                palmTreeTall.draw();
            } else {
                palmTreeStd.draw();
            }
        }

        // G. Fresh Green Wild Reeds & Tall Grass (Trailside, Garden & Pond Banks)
        for (const auto& reed : reedPatches) {
            model = glm::translate(glm::mat4(1.0f), reed.pos);
            model = glm::rotate(model, glm::radians(reed.rotY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(reed.scale, reed.scale, reed.scale));
            sceneShader.setMat4("uModel", model);
            if (reed.isDense) {
                reedClusterDense.draw();
            } else {
                reedClusterLight.draw();
            }
        }

        // H. Weathered Earth Boulders
        for (size_t i = 0; i < boulderPositions.size(); ++i) {
            float bScale = 0.75f + (i % 3) * 0.25f;
            model = glm::translate(glm::mat4(1.0f), boulderPositions[i]);
            model = glm::scale(model, glm::vec3(bScale * 1.25f, bScale * 0.70f, bScale * 0.95f));
            sceneShader.setMat4("uModel", model);
            rusticBoulder.draw();
        }

        // I. Roadside Scenic Props: Rustic Lantern Posts & Waypoint Signpost
        for (const auto& lp : lanternPosts) {
            float y = ModelGenerator::getTerrainHeight(lp.pos.x, lp.pos.z);
            model = glm::translate(glm::mat4(1.0f), glm::vec3(lp.pos.x, y, lp.pos.z));
            model = glm::rotate(model, glm::radians(lp.rotY), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            rusticLantern.draw();
        }

        float wy = ModelGenerator::getTerrainHeight(waypointSignPos.x, waypointSignPos.z);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(waypointSignPos.x, wy, waypointSignPos.z));
        model = glm::rotate(model, glm::radians(waypointSignRot), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        waypointSign.draw();

        // ==========================================
        // 6. Draw Clouds & Flocking Birds
        // ==========================================
        for (const auto& c : clouds) {
            model = glm::translate(glm::mat4(1.0f), c.position);
            model = glm::scale(model, glm::vec3(c.scale, c.scale, c.scale));
            sceneShader.setMat4("uModel", model);
            cloudCluster.draw();
        }

        // Flocking Birds with Dynamic Banking Path
        float flightAngle = simulationTime * 0.12f;
        float flightR = 65.0f;
        glm::vec3 flockCenter(std::sin(flightAngle) * flightR * 1.2f, 25.0f + 4.0f * std::sin(flightAngle * 2.0f), std::cos(flightAngle) * flightR);
        glm::vec3 flockVelocity(std::cos(flightAngle) * flightR * 1.2f, 8.0f * std::cos(flightAngle * 2.0f), -std::sin(flightAngle) * flightR);
        float birdYaw = glm::degrees(std::atan2(flockVelocity.x, flockVelocity.z));
        float birdBank = -std::sin(flightAngle) * 20.0f; // Aviation banking roll into the turn

        for (const auto& b : flock) {
            glm::vec3 birdPos = flockCenter + b.offset;
            glm::mat4 birdRoot = glm::translate(glm::mat4(1.0f), birdPos);
            birdRoot = glm::rotate(birdRoot, glm::radians(birdYaw), glm::vec3(0, 1, 0));
            birdRoot = glm::rotate(birdRoot, glm::radians(birdBank), glm::vec3(0, 0, 1));

            sceneShader.setMat4("uModel", birdRoot);
            birdBody.draw();

            float flapAngle = std::sin(simulationTime * 9.5f + b.flapPhase) * 28.0f;

            glm::mat4 lWingModel = glm::rotate(birdRoot, glm::radians(-flapAngle), glm::vec3(0, 0, 1));
            sceneShader.setMat4("uModel", lWingModel);
            birdLeftWing.draw();

            glm::mat4 rWingModel = glm::rotate(birdRoot, glm::radians(flapAngle), glm::vec3(0, 0, 1));
            sceneShader.setMat4("uModel", rWingModel);
            birdRightWing.draw();
        }

        // ==========================================
        // 7 & 8. Hot Air Balloon Fleet (Hierarchical Rig Engine)
        // ==========================================
        auto drawHotAirBalloon = [&](const glm::vec3& pos, float scale, float swayR, float swayP,
                                     Mesh& envMesh, bool flameOn, float flameIntensity) {
            glm::mat4 bRoot = glm::translate(glm::mat4(1.0f), pos);
            bRoot = glm::scale(bRoot, glm::vec3(scale));

            // A. Balloon Envelope
            glm::mat4 envModel = glm::translate(bRoot, glm::vec3(0.0f, 6.8f, 0.0f));
            sceneShader.setMat4("uModel", envModel);
            envMesh.draw();

            // B. Throat Skirt Collar
            glm::mat4 skirtModel = glm::translate(bRoot, glm::vec3(0.0f, 2.2f, 0.0f));
            sceneShader.setMat4("uModel", skirtModel);
            whiteSkirt.draw();

            // C. Burner Ring
            glm::mat4 burnerModel = glm::translate(bRoot, glm::vec3(0.0f, 1.80f, 0.0f));
            sceneShader.setMat4("uModel", burnerModel);
            burnerRing.draw();

            // D. Burner Flame
            if (flameOn) {
                // If in Basket POV on hero balloon, only draw flame when looking up at the envelope
                bool shouldDrawFlame = true;
                if (camera.mode == CAMERA_BASKET_POV && &envMesh == &rainbowEnvelope && camera.pitch < 20.0f) {
                    shouldDrawFlame = false;
                }

                if (shouldDrawFlame) {
                    float fScale = (0.95f + 0.30f * std::sin(simulationTime * 24.0f + pos.x)) * flameIntensity;
                    glm::mat4 fModel = glm::translate(bRoot, glm::vec3(0.0f, 2.50f, 0.0f));
                    fModel = glm::scale(fModel, glm::vec3(fScale * 0.70f, fScale * 1.0f, fScale * 0.70f));
                    sceneShader.setMat4("uModel", fModel);
                    sceneShader.setFloat("uEmissive", 1.0f);
                    burnerFlame.draw();
                    sceneShader.setFloat("uEmissive", 0.0f);
                }
            }

            // E. Swaying Woven Basket
            glm::mat4 bTransform = glm::rotate(bRoot, glm::radians(swayR), glm::vec3(0, 0, 1));
            bTransform = glm::rotate(bTransform, glm::radians(swayP), glm::vec3(1, 0, 0));

            glm::mat4 bModel = glm::translate(bTransform, glm::vec3(0.0f, -0.6f, 0.0f));
            sceneShader.setMat4("uModel", bModel);
            wovenBasket.draw();

            // F. 8 Suspension Rigging Cables
            float sRadius = 1.22f;
            float sY = 1.80f;
            float bRimY = 0.30f;
            float bAnchors[8][2] = {
                {-1.15f, -1.15f}, { 0.00f, -1.18f}, { 1.15f, -1.15f},
                { 1.18f,  0.00f}, { 1.15f,  1.15f}, { 0.00f,  1.18f},
                {-1.15f,  1.15f}, {-1.18f,  0.00f}
            };

            for (int i = 0; i < 8; ++i) {
                float angle = (float)i * (2.0f * (float)M_PI / 8.0f);
                glm::vec3 topA(std::cos(angle) * sRadius, sY, std::sin(angle) * sRadius);
                glm::vec3 botA(bAnchors[i][0], bRimY, bAnchors[i][1]);

                glm::vec3 cMid = (topA + botA) * 0.5f;
                glm::vec3 dir = topA - botA;
                float cLen = glm::length(dir);
                dir = glm::normalize(dir);

                glm::mat4 cModel = glm::translate(bTransform, cMid);
                glm::vec3 upV(0.0f, 1.0f, 0.0f);
                glm::vec3 axis = glm::cross(upV, dir);
                float cosA = glm::dot(upV, dir);
                if (glm::length(axis) > 0.001f) {
                    cModel = glm::rotate(cModel, std::acos(cosA), glm::normalize(axis));
                }
                cModel = glm::scale(cModel, glm::vec3(1.0f, cLen / 3.4f, 1.0f));

                sceneShader.setMat4("uModel", cModel);
                riggingCable.draw();
            }
        };

        // Render Background Balloons (Different scales, altitudes & decoupled trajectories)
        drawHotAirBalloon(bg1Pos, bg1Scale, bg1Roll, bg1Pitch, sunsetEnvelope, true, 0.45f);
        drawHotAirBalloon(bg2Pos, bg2Scale, bg2Roll, bg2Pitch, oceanEnvelope, false, 0.0f);
        drawHotAirBalloon(bg3Pos, bg3Scale, bg3Roll, bg3Pitch, emeraldEnvelope, false, 0.0f);

        // Render Main Hero Hot Air Balloon (Full physics & user burner control)
        drawHotAirBalloon(balloonPosition, 1.0f, basketSwayRoll, basketSwayPitch, rainbowEnvelope, burnerActive, 1.0f);

        // ==========================================
        // 8.5. Real-Time Screen-Space Ray Tracing Pass (Key T)
        // ==========================================
        if (isRayTracingActive) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbWidth, fbHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            rayTracingShader.use();
            rayTracingShader.setMat4("uView", view);
            rayTracingShader.setMat4("uProjection", projection);
            rayTracingShader.setVec3("uCamPos", camera.position);
            rayTracingShader.setVec3("uDirLightDir", curSunDir);
            rayTracingShader.setVec3("uDirLightColor", curSunColor);
            rayTracingShader.setVec3("uSkyColor", curSkyColor);
            rayTracingShader.setFloat("uTime", simulationTime);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gColor);
            rayTracingShader.setInt("uGColor", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            rayTracingShader.setInt("uGNormal", 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            rayTracingShader.setInt("uGPosition", 2);

            glBindVertexArray(screenQuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
        }

        // ==========================================
        // 9. Render 2D Orthographic Avionics HUD Overlay (Disabled by default per user request)
        // ==========================================
        if (showHud) {
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            hud.renderDashboard(
                fbWidth, fbHeight,
                balloonPosition.y,
                balloonVelocity.y,
                windSpeed,
                windHeadingDeg,
                burnerActive,
                static_cast<int>(camera.mode),
                static_cast<int>(currentLightMode),
                currentFps
            );
        }

        // ==========================================
        // 10. Swap Buffers & Poll Events
        // ==========================================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
