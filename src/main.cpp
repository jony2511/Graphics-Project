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

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Simulation State
bool isPaused = false;
bool burnerActive = true;
bool showHud = false; // Set to false per user request: "scene er modde thaka lekha gula bad daw"
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

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
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

    if (camera.mode == CAMERA_FREE_FLY && rightMousePressed) {
        camera.processMouseMovement(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window;
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightMousePressed = (action == GLFW_PRESS);
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

    // R: Reset Scene
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        balloonAltitude = 4.2f;
        balloonPosition = glm::vec3(0.0f, 4.2f, 0.0f);
        balloonVelocity = glm::vec3(0.0f);
        simulationTime = 0.0f;
        std::cout << "[SCENE] Reset to Launch Position\n";
        rPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rPressed = false;
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
    std::cout << "  * 1, 2, 3, 4               : Switch Camera Modes (Overview, Follow, Free-fly, Basket)\n";
    std::cout << "  * L                        : Cycle Day / Sunset / Night / Dawn Lighting\n";
    std::cout << "  * R                        : Reset Balloon to Launch Position\n";
    std::cout << "  * H                        : Toggle HUD Dashboard\n";
    std::cout << "========================================================\n";

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 4. Build and Compile Shaders & HUD System
    Shader sceneShader(SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER);
    HUD hud;
    hud.init();

    // 5. Generate Procedural 3D Meshes
    // A. Rural Terrain & Dirt Road
    Mesh rollingTerrain = ModelGenerator::createRollingTerrain(260.0f, 260.0f, 80);
    Mesh dirtRoad = ModelGenerator::createCurvedDirtRoad();

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
    Mesh groundShadow = ModelGenerator::createShadowDisc(5.2f, 32);

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
        {{-26.0f, 0.0f, -18.0f}, 1.10f,  80.0f}  // Near launch platform edge
    };

    // Traditional Bengali Banana Tree Clusters (কলা বাগান)
    struct BananaInstance {
        glm::vec3 pos;
        float scale;
        float rotY;
    };
    std::vector<BananaInstance> bananaTrees = {
        // Cluster behind and to the right of the cottage
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
        {{-16.0f, 0.0f,  30.2f}, 1.20f, -65.0f}
    };

    // Dense Green Bush & Shrub Clumps (গ্রামের সবুজ ঝোপঝাড়)
    struct BushInstance {
        glm::vec3 pos;
        float scale;
    };
    std::vector<BushInstance> villageBushes = {
        // Cottage perimeter & garden
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
        {{ 40.0f, 0.0f,  36.0f}, 1.20f}
    };

    // Horizon Coconut Palms (Dense silhouette along the horizon)
    struct HorizonPalm {
        glm::vec3 pos;
        float scale;
        float rotY;
        bool isTall;
    };
    std::vector<HorizonPalm> horizonPalms = {
        {{-85.0f, 0.0f,  75.0f}, 1.15f,  20.0f, true},
        {{-72.0f, 0.0f,  78.0f}, 0.95f, -45.0f, false},
        {{-60.0f, 0.0f,  82.0f}, 1.25f,  60.0f, true},
        {{-48.0f, 0.0f,  85.0f}, 1.05f, -15.0f, false},
        {{-35.0f, 0.0f,  88.0f}, 1.20f,  40.0f, true},
        {{-22.0f, 0.0f,  86.0f}, 0.90f, -80.0f, false},
        {{-10.0f, 0.0f,  89.0f}, 1.30f,  15.0f, true},
        {{  2.0f, 0.0f,  92.0f}, 1.10f, -30.0f, false},
        {{ 14.0f, 0.0f,  90.0f}, 1.25f,  50.0f, true},
        {{ 26.0f, 0.0f,  88.0f}, 0.95f, -65.0f, false},
        {{ 38.0f, 0.0f,  86.0f}, 1.20f,  25.0f, true},
        {{ 50.0f, 0.0f,  84.0f}, 1.05f, -40.0f, false},
        {{ 62.0f, 0.0f,  82.0f}, 1.15f,  70.0f, true},
        {{ 75.0f, 0.0f,  80.0f}, 0.90f, -20.0f, false},
        {{ 88.0f, 0.0f,  78.0f}, 1.25f,  35.0f, true},
        {{100.0f, 0.0f,  76.0f}, 1.00f, -50.0f, false},
        // Mid-distance groves
        {{-52.0f, 0.0f,  55.0f}, 1.10f,  10.0f, true},
        {{-38.0f, 0.0f,  48.0f}, 0.85f, -70.0f, false},
        {{ 58.0f, 0.0f,  52.0f}, 1.05f,  45.0f, true},
        {{ 70.0f, 0.0f,  58.0f}, 0.90f, -35.0f, false},
        // Opposite side groves
        {{-65.0f, 0.0f, -45.0f}, 1.15f,  30.0f, true},
        {{-45.0f, 0.0f, -55.0f}, 1.00f, -25.0f, false},
        {{ 45.0f, 0.0f, -50.0f}, 1.20f,  55.0f, true},
        {{ 65.0f, 0.0f, -42.0f}, 0.95f, -15.0f, false}
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

    // Frame counter
    double lastTitleUpdate = 0.0;
    int frameCount = 0;
    float currentFps = 60.0f;

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
            // 2. Aerodynamic Balloon Ascent & Drift Physics
            // ==========================================
            if (burnerActive) {
                // Buoyancy lift force minus drag
                float targetAscentRate = 2.2f;
                balloonVelocity.y += (targetAscentRate - balloonVelocity.y) * 0.8f * deltaTime;
                balloonAltitude += balloonVelocity.y * deltaTime;
                if (balloonAltitude > 38.0f) {
                    balloonAltitude = 38.0f;
                    balloonVelocity.y = 0.0f;
                }
            } else {
                // Cooling descent with terminal velocity
                float targetDescentRate = -1.8f;
                balloonVelocity.y += (targetDescentRate - balloonVelocity.y) * 0.65f * deltaTime;
                balloonAltitude += balloonVelocity.y * deltaTime;
                if (balloonAltitude < 4.2f) {
                    balloonAltitude = 4.2f;
                    balloonVelocity.y = 0.0f;
                }
            }

            // Horizontal wind drag and interactive flight steering (Arrow Keys)
            float maxSteerSpeed = 8.5f; // m/s (~30.6 km/h)
            float driftTargetX = (windVector.x * (balloonAltitude / 14.0f)) + (userSteerX * maxSteerSpeed);
            float driftTargetZ = (windVector.z * (balloonAltitude / 14.0f)) + (userSteerZ * maxSteerSpeed);

            // Responsive steering acceleration when user is actively steering; smooth aerodynamic inertia when coasting
            bool isSteeringActive = (std::abs(userSteerX) > 0.01f || std::abs(userSteerZ) > 0.01f);
            float steerResponse = isSteeringActive ? 2.4f : 0.65f;

            balloonVelocity.x += (driftTargetX - balloonVelocity.x) * steerResponse * deltaTime;
            balloonVelocity.z += (driftTargetZ - balloonVelocity.z) * steerResponse * deltaTime;

            balloonPosition.x += balloonVelocity.x * deltaTime;
            balloonPosition.z += balloonVelocity.z * deltaTime;
            balloonPosition.y = balloonAltitude;

            // Soft terrain boundaries (keeps balloon inside the scenic world)
            balloonPosition.x = glm::clamp(balloonPosition.x, -85.0f, 85.0f);
            balloonPosition.z = glm::clamp(balloonPosition.z, -85.0f, 85.0f);

            // Multi-Axis Basket Pendulum Sway Physics (dynamically tilts with steering forces)
            float swayFreq = 2.3f;
            float naturalDamping = 0.95f;
            float windGustForce = (windSpeed / 15.0f);
            basketSwayRoll = glm::clamp((3.4f * std::sin(simulationTime * swayFreq) + balloonVelocity.x * 2.2f) * windGustForce * naturalDamping, -18.0f, 18.0f);
            basketSwayPitch = glm::clamp((2.6f * std::cos(simulationTime * (swayFreq * 0.88f)) + balloonVelocity.z * 2.2f) * windGustForce * naturalDamping, -18.0f, 18.0f);

            // ==========================================
            // 3. Environmental Rotations & Drifts
            // ==========================================
            // Windmill rotation speed directly driven by wind speed
            windmillAngle += (windSpeed * 3.4f) * deltaTime;
            if (windmillAngle > 360.0f) windmillAngle -= 360.0f;

            // Clouds drift with wind vector
            for (auto& c : clouds) {
                c.position += windVector * (c.speedMultiplier * 0.4f) * deltaTime;
                if (c.position.x > 120.0f) c.position.x = -120.0f;
                if (c.position.z > 120.0f) c.position.z = -120.0f;
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
        camera.update(deltaTime, balloonPosition);

        // Update Window Title with Live Telemetry
        frameCount++;
        if (currentFrame - lastTitleUpdate >= 0.25) {
            float fps = frameCount / static_cast<float>(currentFrame - lastTitleUpdate);
            currentFps = fps;
            std::ostringstream ss;
            ss << "Hot Air Balloon 3D | Alt: " << std::fixed << std::setprecision(1) << balloonPosition.y << "m"
               << " | Steer: [Arrows]"
               << " | Burner: [" << (burnerActive ? "FIRE (F)" : "OFF (F)") << "]"
               << " | Cam: " << camera.getModeName()
               << " | " << getLightingModeName(currentLightMode) << " [L]"
               << " | FPS: " << static_cast<int>(fps);
            glfwSetWindowTitle(window, ss.str().c_str());
            frameCount = 0;
            lastTitleUpdate = currentFrame;
        }

        // Dynamic Sky Color Clear
        glClearColor(curSkyColor.r, curSkyColor.g, curSkyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use();

        float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbHeight > 0) aspectRatio = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

        glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 view = camera.getViewMatrix();
        sceneShader.setMat4("uProjection", projection);
        sceneShader.setMat4("uView", view);
        sceneShader.setVec3("uViewPos", camera.position);

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
        // 2. Draw Rural Meadow & Dirt Road
        // ==========================================
        model = glm::mat4(1.0f);
        sceneShader.setMat4("uModel", model);
        rollingTerrain.draw();
        dirtRoad.draw();

        // Compute Decoupled Background Balloon Trajectories
        float bg1Alt = 28.0f + 2.2f * std::sin(simulationTime * 0.40f + 1.2f);
        float bg1X   = 34.0f + 3.2f * std::sin(simulationTime * 0.16f);
        float bg1Z   = 24.0f + 2.0f * std::cos(simulationTime * 0.14f);
        glm::vec3 bg1Pos(bg1X, bg1Alt, bg1Z);
        float bg1Scale = 0.65f;
        float bg1Roll  = 2.5f * std::sin(simulationTime * 1.8f);
        float bg1Pitch = 1.8f * std::cos(simulationTime * 1.5f);

        float bg2Alt = 36.0f + 2.8f * std::cos(simulationTime * 0.35f + 0.8f);
        float bg2X   = -30.0f + 3.8f * std::cos(simulationTime * 0.14f + 1.2f);
        float bg2Z   = 36.0f + 2.8f * std::sin(simulationTime * 0.12f + 0.4f);
        glm::vec3 bg2Pos(bg2X, bg2Alt, bg2Z);
        float bg2Scale = 0.52f;
        float bg2Roll  = -2.2f * std::cos(simulationTime * 1.6f);
        float bg2Pitch = 1.6f * std::sin(simulationTime * 1.4f);

        float bg3Alt = 48.0f + 2.2f * std::sin(simulationTime * 0.28f + 2.4f);
        float bg3X   = 12.0f + 4.2f * std::sin(simulationTime * 0.11f + 1.8f);
        float bg3Z   = 68.0f + 3.0f * std::cos(simulationTime * 0.10f + 1.0f);
        glm::vec3 bg3Pos(bg3X, bg3Alt, bg3Z);
        float bg3Scale = 0.40f;
        float bg3Roll  = 1.6f * std::sin(simulationTime * 1.4f);
        float bg3Pitch = 1.2f * std::cos(simulationTime * 1.2f);

        // ==========================================
        // 3. Draw Dynamic Ground Shadows (All Balloons)
        // ==========================================
        auto drawBalloonShadow = [&](const glm::vec3& bPos, float bScale) {
            if (curSunDir.y < -0.1f) {
                float tShadow = -(bPos.y - 0.06f) / curSunDir.y;
                float shadowX = bPos.x + tShadow * curSunDir.x;
                float shadowZ = bPos.z + tShadow * curSunDir.z;

                float shadowScale = bScale / (1.0f + 0.032f * bPos.y);
                float shadowAlpha = glm::clamp((0.55f - 0.012f * bPos.y) * bScale, 0.06f, 0.45f);

                if (currentLightMode == LIGHT_NIGHT) {
                    shadowAlpha *= 0.45f;
                }

                glm::mat4 sModel = glm::translate(glm::mat4(1.0f), glm::vec3(shadowX, 0.06f, shadowZ));
                sModel = glm::scale(sModel, glm::vec3(shadowScale, 1.0f, shadowScale * 1.15f));
                sceneShader.setMat4("uModel", sModel);
                sceneShader.setFloat("uAlpha", shadowAlpha);
                sceneShader.setFloat("uSpecularStrength", 0.0f);
                groundShadow.draw();
            }
        };

        drawBalloonShadow(balloonPosition, 1.0f);
        drawBalloonShadow(bg1Pos, bg1Scale);
        drawBalloonShadow(bg2Pos, bg2Scale);
        drawBalloonShadow(bg3Pos, bg3Scale);

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
        // A. Traditional Village Cottage / Hut with Terracotta Hip Roof
        glm::vec3 hutPos(21.0f, 0.0f, 31.0f);
        model = glm::translate(glm::mat4(1.0f), hutPos);
        model = glm::rotate(model, glm::radians(-28.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        villageHut.draw();

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
        wetlandWater.draw();
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
            glm::mat4 skirtModel = glm::translate(bRoot, glm::vec3(0.0f, 1.9f, 0.0f));
            sceneShader.setMat4("uModel", skirtModel);
            whiteSkirt.draw();

            // C. Burner Ring
            glm::mat4 burnerModel = glm::translate(bRoot, glm::vec3(0.0f, 1.25f, 0.0f));
            sceneShader.setMat4("uModel", burnerModel);
            burnerRing.draw();

            // D. Burner Flame
            if (flameOn) {
                float fScale = (0.95f + 0.35f * std::sin(simulationTime * 24.0f + pos.x)) * flameIntensity;
                glm::mat4 fModel = glm::translate(bRoot, glm::vec3(0.0f, 1.4f, 0.0f));
                fModel = glm::scale(fModel, glm::vec3(fScale, fScale * 1.35f, fScale));
                sceneShader.setMat4("uModel", fModel);
                sceneShader.setFloat("uEmissive", 1.0f);
                burnerFlame.draw();
                sceneShader.setFloat("uEmissive", 0.0f);
            }

            // E. Swaying Woven Basket
            glm::mat4 bTransform = glm::rotate(bRoot, glm::radians(swayR), glm::vec3(0, 0, 1));
            bTransform = glm::rotate(bTransform, glm::radians(swayP), glm::vec3(1, 0, 0));

            glm::mat4 bModel = glm::translate(bTransform, glm::vec3(0.0f, -0.6f, 0.0f));
            sceneShader.setMat4("uModel", bModel);
            wovenBasket.draw();

            // F. 8 Suspension Rigging Cables
            float sRadius = 1.22f;
            float sY = 1.35f;
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
