#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Mesh.h"
#include "ModelGenerator.h"
#include "Shader.h"
#include "ShaderSources.h"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>

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
float balloonAltitude = 4.2f;
glm::vec3 balloonPosition(0.0f, 4.2f, 0.0f);
float simulationTime = 0.0f;
float windmillAngle = 0.0f;

// Cloud data structure for drifting sky
struct Cloud {
    glm::vec3 position;
    float scale;
    float speed;
};

// Bird in flock data structure
struct Bird {
    glm::vec3 offset; // Relative to flock center
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
        std::cout << "[BURNER] " << (burnerActive ? "FIRE ON (Ascending)" : "OFF (Idle)") << "\n";
        fPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fPressed = false;
    }

    // R: Reset Scene
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        balloonAltitude = 4.2f;
        balloonPosition = glm::vec3(0.0f, 4.2f, 0.0f);
        simulationTime = 0.0f;
        std::cout << "[SCENE] Reset to Launch Position\n";
        rPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rPressed = false;
    }

    // Free-fly Camera Movement
    if (camera.mode == CAMERA_FREE_FLY) {
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

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            camera.processKeyboard(CAM_YAW_LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            camera.processKeyboard(CAM_YAW_RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            camera.processKeyboard(CAM_PITCH_UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            camera.processKeyboard(CAM_PITCH_DOWN, deltaTime);
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hot Air Balloon 3D - Phase 3: High-Fidelity Rig & Background Balloons", nullptr, nullptr);
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
    std::cout << "  Hot Air Balloon 3D: Phase 3 Operational\n";
    std::cout << "  OpenGL Version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << "\n";
    std::cout << "  Renderer:       " << glGetString(GL_RENDERER) << "\n";
    std::cout << "========================================================\n";
    std::cout << "Phase 3 Features Active:\n";
    std::cout << "  * Vivid 7-Color Rainbow Envelope with 3D Puffy Gores\n";
    std::cout << "  * White Horizontal Equator Belt with Scalloped Swags\n";
    std::cout << "  * White Throat Skirt Collar & 8 Suspension Rigging Cables\n";
    std::cout << "  * Woven Wicker Basket with Corner Posts & Handrail\n";
    std::cout << "  * 2 Independent Background Balloons at Varying Heights\n";
    std::cout << "  * Volumetric Drifting Cumulus Cloud Clusters\n";
    std::cout << "  * Flock of Flapping Birds Flying in V-Formation\n";
    std::cout << "  * Controls: [1-4] Cameras | [F] Burner | [Space] Pause | [R] Reset\n";
    std::cout << "========================================================\n";

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    // 4. Build and Compile Shaders
    Shader sceneShader(SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER);

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
    Mesh floodlightHead = ModelGenerator::createCone(0.35f, 0.5f, 12, glm::vec3(0.20f, 0.20f, 0.22f));

    // C. Rural Architecture (Barn, Windmill, Cottage)
    Mesh barnWalls = ModelGenerator::createCube(14.0f, 6.5f, 18.0f, glm::vec3(0.68f, 0.18f, 0.14f));
    Mesh barnRoof = ModelGenerator::createPrism(15.2f, 4.2f, 18.8f, glm::vec3(0.25f, 0.25f, 0.28f));
    Mesh barnDoors = ModelGenerator::createCube(4.0f, 4.5f, 0.2f, glm::vec3(0.92f, 0.90f, 0.85f));
    Mesh barnDoorCross = ModelGenerator::createCube(3.6f, 0.25f, 0.25f, glm::vec3(0.68f, 0.18f, 0.14f));
    Mesh siloTower = ModelGenerator::createCylinder(2.4f, 2.4f, 10.0f, 18, glm::vec3(0.65f, 0.65f, 0.68f));
    Mesh siloCap = ModelGenerator::createSphere(2.45f, 12, 18, glm::vec3(0.45f, 0.45f, 0.48f));

    Mesh windmillBase = ModelGenerator::createCylinder(4.6f, 3.2f, 13.5f, 16, glm::vec3(0.78f, 0.74f, 0.68f));
    Mesh windmillRoof = ModelGenerator::createCone(3.6f, 3.8f, 16, glm::vec3(0.32f, 0.22f, 0.16f));
    Mesh windmillBalcony = ModelGenerator::createCylinder(4.2f, 4.2f, 0.35f, 16, glm::vec3(0.42f, 0.28f, 0.18f));
    Mesh windmillHub = ModelGenerator::createSphere(0.75f, 12, 12, glm::vec3(0.28f, 0.24f, 0.20f));
    Mesh windmillBlade = ModelGenerator::createWindmillBlade(7.8f, 1.35f, glm::vec3(0.45f, 0.30f, 0.18f), glm::vec3(0.92f, 0.90f, 0.82f));

    Mesh cottageWalls = ModelGenerator::createCube(9.0f, 4.5f, 7.5f, glm::vec3(0.85f, 0.80f, 0.70f));
    Mesh cottageRoof = ModelGenerator::createPrism(10.0f, 3.0f, 8.2f, glm::vec3(0.75f, 0.32f, 0.18f));
    Mesh cottageChimney = ModelGenerator::createCube(1.2f, 5.2f, 1.2f, glm::vec3(0.45f, 0.42f, 0.40f));
    Mesh cottageDoor = ModelGenerator::createCube(1.4f, 2.8f, 0.15f, glm::vec3(0.42f, 0.25f, 0.14f));

    Mesh hayBale = ModelGenerator::createHayBale(1.1f, 2.2f);
    Mesh pastureFencePost = ModelGenerator::createCube(0.2f, 1.2f, 0.2f, glm::vec3(0.45f, 0.30f, 0.18f));
    Mesh pastureFenceRail = ModelGenerator::createCube(4.2f, 0.10f, 0.10f, glm::vec3(0.50f, 0.34f, 0.20f));
    Mesh boulder = ModelGenerator::createSphere(1.2f, 10, 10, glm::vec3(0.52f, 0.52f, 0.50f));

    Mesh treeTrunk = ModelGenerator::createCylinder(0.45f, 0.35f, 4.0f, 12, glm::vec3(0.42f, 0.25f, 0.14f));
    Mesh treeFoliagePine = ModelGenerator::createCone(2.4f, 4.5f, 12, glm::vec3(0.14f, 0.40f, 0.16f));
    Mesh treeFoliageLeafy = ModelGenerator::createSphere(2.2f, 16, 16, glm::vec3(0.22f, 0.52f, 0.18f));

    // D. Main Hot Air Balloon (Matching User's Reference Image)
    // 14 gores, 7 vibrant rainbow colors, 3D puffy curvature
    Mesh rainbowEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 36, 14, 0);
    Mesh equatorBelt = ModelGenerator::createBalloonEquatorBelt(4.72f, 0.08f);
    Mesh whiteSkirt = ModelGenerator::createBalloonWhiteSkirt(1.55f, 1.25f, 1.35f, 32);
    Mesh wovenBasket = ModelGenerator::createWovenBasket(2.4f, 1.8f, 2.4f);
    Mesh burnerRing = ModelGenerator::createCylinder(0.65f, 0.65f, 0.25f, 16, glm::vec3(0.28f, 0.28f, 0.32f));
    Mesh burnerFlame = ModelGenerator::createCone(0.55f, 1.4f, 16, glm::vec3(1.0f, 0.55f, 0.05f));
    Mesh riggingCable = ModelGenerator::createCylinder(0.025f, 0.025f, 3.4f, 8, glm::vec3(0.20f, 0.20f, 0.22f));

    // E. Two Secondary Background Balloons
    // Balloon 1: Sunset Fire (scale 0.55x)
    Mesh sunsetEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 28, 12, 1);
    // Balloon 2: Ocean Teal (scale 0.38x)
    Mesh oceanEnvelope = ModelGenerator::createRainbowBalloonEnvelope(4.8f, 9.6f, 28, 12, 2);

    // F. Sky Atmosphere: Volumetric Clouds & Flapping Birds
    Mesh cloudCluster = ModelGenerator::createCloudCluster();
    Mesh birdBody = ModelGenerator::createBirdBody();
    Mesh birdLeftWing = ModelGenerator::createBirdWing(true);
    Mesh birdRightWing = ModelGenerator::createBirdWing(false);

    // Initial Cloud positions drifting in the sky
    std::vector<Cloud> clouds = {
        {{-40.0f, 32.0f, -30.0f}, 1.3f, 1.8f},
        {{ 10.0f, 38.0f, -50.0f}, 1.6f, 2.2f},
        {{ 55.0f, 28.0f, -20.0f}, 1.1f, 1.5f},
        {{-20.0f, 42.0f,  30.0f}, 1.4f, 2.0f},
        {{ 35.0f, 35.0f,  45.0f}, 1.2f, 1.6f},
        {{-60.0f, 30.0f,  15.0f}, 1.5f, 1.9f}
    };

    // Flock of Birds in V-Formation
    std::vector<Bird> flock = {
        {{ 0.0f,  0.0f,   0.0f}, 0.0f},  // Leader
        {{-2.2f, -0.4f,  -2.0f}, 0.5f},  // Left wing 1
        {{ 2.2f, -0.3f,  -2.0f}, 0.8f},  // Right wing 1
        {{-4.4f, -0.8f,  -4.0f}, 1.1f},  // Left wing 2
        {{ 4.4f, -0.7f,  -4.0f}, 1.4f}   // Right wing 2
    };
    glm::vec3 flockBasePos(-50.0f, 24.0f, -10.0f);

    // Trees layout
    struct TreeInstance {
        glm::vec3 pos;
        float scale;
        bool isPine;
    };
    std::vector<TreeInstance> trees = {
        {{-42.0f, 0.0f,  18.0f}, 1.3f, false},
        {{-45.0f, 0.0f,  30.0f}, 1.5f, false},
        {{-22.0f, 0.0f,  32.0f}, 1.1f, false},
        {{-28.0f, 0.0f,  12.0f}, 1.2f, true},
        {{ 10.0f, 0.0f,  20.0f}, 1.0f, false},
        {{ 14.0f, 0.0f,  38.0f}, 1.2f, false},
        {{-04.0f, 0.0f,  45.0f}, 1.1f, true},
        {{ 32.0f, 0.0f,  65.0f}, 1.4f, false},
        {{ 45.0f, 1.2f, -18.0f}, 1.3f, true},
        {{ 42.0f, 1.0f, -34.0f}, 1.4f, true},
        {{ 26.0f, 0.2f, -32.0f}, 1.2f, false},
        {{-55.0f, 3.5f, -25.0f}, 1.6f, true},
        {{-60.0f, 4.2f, -10.0f}, 1.5f, true},
        {{-50.0f, 2.8f, -42.0f}, 1.7f, true},
        {{ 55.0f, 3.0f,  15.0f}, 1.5f, true},
        {{ 62.0f, 4.0f,  30.0f}, 1.6f, true},
        {{-15.0f, 0.0f, -30.0f}, 1.2f, true},
        {{ 12.0f, 0.0f, -28.0f}, 1.1f, true},
        {{-20.0f, 0.0f, -18.0f}, 1.3f, true},
        {{ 22.0f, 0.0f, -16.0f}, 1.2f, false}
    };

    std::vector<glm::vec3> hayBalePositions = {
        {-20.0f, 1.1f, 16.0f}, {-16.0f, 1.1f, 22.0f},
        {-24.0f, 1.1f, 20.0f}, {-22.0f, 1.1f, 26.0f},
        {-18.0f, 1.1f, 28.0f}
    };

    std::vector<glm::vec3> boulderPositions = {
        { 16.0f, 0.6f, -10.0f}, {-12.0f, 0.5f,  12.0f},
        { 24.0f, 0.7f,  16.0f}, {-26.0f, 0.8f, -14.0f}
    };

    // Frame counter
    double lastTitleUpdate = 0.0;
    int frameCount = 0;

    // 6. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (!isPaused) {
            simulationTime += deltaTime;

            // Main Balloon Ascent & Hover Physics
            if (burnerActive) {
                // Rising up slowly to cruising altitude (e.g. 18m) and gently floating
                if (balloonAltitude < 18.0f) {
                    balloonAltitude += 1.4f * deltaTime;
                } else {
                    balloonAltitude = 18.0f + 0.8f * std::sin(simulationTime * 0.8f);
                }
            } else {
                // Descending slowly back towards pad
                if (balloonAltitude > 4.2f) {
                    balloonAltitude -= 1.6f * deltaTime;
                } else {
                    balloonAltitude = 4.2f;
                }
            }
            // Gentle horizontal wind drift
            float driftX = 2.5f * std::sin(simulationTime * 0.25f);
            balloonPosition = glm::vec3(driftX, balloonAltitude, 0.0f);

            // Windmill rotation
            windmillAngle += 45.0f * deltaTime;
            if (windmillAngle > 360.0f) windmillAngle -= 360.0f;

            // Clouds drift across the sky with wind (wrapping around boundary)
            for (auto& c : clouds) {
                c.position.x += c.speed * deltaTime;
                if (c.position.x > 110.0f) c.position.x = -110.0f;
            }

            // Bird flock flying across the sky
            flockBasePos.x += 6.5f * deltaTime;
            flockBasePos.z += 1.8f * deltaTime;
            flockBasePos.y = 24.0f + 1.2f * std::sin(simulationTime * 0.6f);
            if (flockBasePos.x > 110.0f) {
                flockBasePos.x = -110.0f;
                flockBasePos.z = -35.0f;
            }
        }

        processInput(window);
        camera.update(deltaTime, balloonPosition);

        // Update Window Title
        frameCount++;
        if (currentFrame - lastTitleUpdate >= 0.25) {
            float fps = frameCount / static_cast<float>(currentFrame - lastTitleUpdate);
            std::ostringstream ss;
            ss << "Hot Air Balloon 3D | Phase 3 High-Fidelity Rig | " << camera.getModeName()
               << " | Alt: " << std::fixed << std::setprecision(1) << balloonPosition.y << "m"
               << " | Burner: [" << (burnerActive ? "ACTIVE (F)" : "OFF (F)") << "]"
               << " | FPS: " << static_cast<int>(fps);
            glfwSetWindowTitle(window, ss.str().c_str());
            frameCount = 0;
            lastTitleUpdate = currentFrame;
        }

        // Sky Color (Matching user's vibrant blue sky)
        glClearColor(0.50f, 0.76f, 0.94f, 1.0f);
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

        // Warm Sunlight & Atmospheric Sky Ambient
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.55f, -0.80f, -0.45f));
        sceneShader.setVec3("uDirLightDir", sunDir);
        sceneShader.setVec3("uDirLightColor", glm::vec3(1.0f, 0.98f, 0.90f));
        sceneShader.setVec3("uAmbientColor", glm::vec3(0.40f, 0.46f, 0.58f));

        // Burner Point Light
        glm::vec3 burnerPos = balloonPosition + glm::vec3(0.0f, 1.3f, 0.0f);
        float flameFlicker = 1.0f + 0.20f * std::sin(simulationTime * 22.0f);
        sceneShader.setVec3("uPointLightPos", burnerPos);
        sceneShader.setVec3("uPointLightColor", glm::vec3(1.0f, 0.65f, 0.12f));
        sceneShader.setFloat("uPointLightIntensity", burnerActive ? flameFlicker : 0.15f);

        sceneShader.setFloat("uSpecularStrength", 0.40f);
        sceneShader.setFloat("uShininess", 32.0f);

        glm::mat4 model(1.0f);

        // ==========================================
        // 1. Draw Rural Meadow & Dirt Road
        // ==========================================
        model = glm::mat4(1.0f);
        sceneShader.setMat4("uModel", model);
        rollingTerrain.draw();
        dirtRoad.draw();

        // ==========================================
        // 2. Draw Launch Platform, Fences & Mast
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

        glm::vec3 mastPos(9.8f, 0.0f, -9.8f);
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 4.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windsockPole.draw();

        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(-0.35f, 8.2f, 0.35f));
        model = glm::rotate(model, glm::radians(135.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        floodlightHead.draw();

        float windHeading = 48.0f + 8.0f * std::sin(simulationTime * 1.4f);
        float sockFlutter = 82.0f + 5.0f * std::sin(simulationTime * 4.5f);
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 8.3f, 0.0f));
        model = glm::rotate(model, glm::radians(windHeading), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(sockFlutter), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        windsockCone.draw();

        // ==========================================
        // 3. Draw Countryside Barn, Windmill, Cottage
        // ==========================================
        glm::vec3 barnPos(-32.0f, 0.0f, 25.0f);
        glm::mat4 barnBase = glm::rotate(glm::translate(glm::mat4(1.0f), barnPos), glm::radians(25.0f), glm::vec3(0, 1, 0));

        model = glm::translate(barnBase, glm::vec3(0.0f, 3.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        barnWalls.draw();

        model = glm::translate(barnBase, glm::vec3(0.0f, 6.5f, 0.0f));
        sceneShader.setMat4("uModel", model);
        barnRoof.draw();

        model = glm::translate(barnBase, glm::vec3(0.0f, 2.25f, 9.05f));
        sceneShader.setMat4("uModel", model);
        barnDoors.draw();

        model = glm::translate(barnBase, glm::vec3(0.0f, 2.25f, 9.15f));
        sceneShader.setMat4("uModel", model);
        barnDoorCross.draw();

        glm::vec3 siloPos = barnPos + glm::vec3(9.5f, 0.0f, -2.0f);
        model = glm::translate(glm::mat4(1.0f), siloPos + glm::vec3(0.0f, 5.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        siloTower.draw();

        model = glm::translate(glm::mat4(1.0f), siloPos + glm::vec3(0.0f, 10.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        siloCap.draw();

        // Windmill
        glm::vec3 windmillPos(38.0f, 0.5f, -28.0f);
        glm::mat4 windmillBaseTrans = glm::translate(glm::mat4(1.0f), windmillPos);

        model = glm::translate(windmillBaseTrans, glm::vec3(0.0f, 6.75f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windmillBase.draw();

        model = glm::translate(windmillBaseTrans, glm::vec3(0.0f, 9.2f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windmillBalcony.draw();

        model = glm::translate(windmillBaseTrans, glm::vec3(0.0f, 15.4f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windmillRoof.draw();

        glm::vec3 hubPos = windmillPos + glm::vec3(-2.8f, 13.8f, 0.8f);
        model = glm::translate(glm::mat4(1.0f), hubPos);
        model = glm::rotate(model, glm::radians(70.0f), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        windmillHub.draw();

        for (int i = 0; i < 4; ++i) {
            float bladeAngle = windmillAngle + i * 90.0f;
            glm::mat4 bladeModel = glm::translate(glm::mat4(1.0f), hubPos);
            bladeModel = glm::rotate(bladeModel, glm::radians(70.0f), glm::vec3(0, 1, 0));
            bladeModel = glm::rotate(bladeModel, glm::radians(bladeAngle), glm::vec3(0, 0, 1));
            sceneShader.setMat4("uModel", bladeModel);
            windmillBlade.draw();
        }

        // Cottage
        glm::vec3 cottagePos(28.0f, 0.0f, 26.0f);
        glm::mat4 cottageTrans = glm::rotate(glm::translate(glm::mat4(1.0f), cottagePos), glm::radians(-35.0f), glm::vec3(0, 1, 0));

        model = glm::translate(cottageTrans, glm::vec3(0.0f, 2.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        cottageWalls.draw();

        model = glm::translate(cottageTrans, glm::vec3(0.0f, 4.5f, 0.0f));
        sceneShader.setMat4("uModel", model);
        cottageRoof.draw();

        model = glm::translate(cottageTrans, glm::vec3(-3.2f, 4.0f, 2.2f));
        sceneShader.setMat4("uModel", model);
        cottageChimney.draw();

        model = glm::translate(cottageTrans, glm::vec3(0.0f, 1.4f, 3.8f));
        sceneShader.setMat4("uModel", model);
        cottageDoor.draw();

        // Hay bales
        for (const auto& pos : hayBalePositions) {
            model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1));
            model = glm::rotate(model, glm::radians(pos.x * 3.0f), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            hayBale.draw();
        }

        // Fences
        for (int i = 0; i < 7; ++i) {
            float fenceX = -38.0f + i * 4.0f;
            float fenceZ = 12.0f;
            model = glm::translate(glm::mat4(1.0f), glm::vec3(fenceX, 0.6f, fenceZ));
            sceneShader.setMat4("uModel", model);
            pastureFencePost.draw();

            if (i < 6) {
                model = glm::translate(glm::mat4(1.0f), glm::vec3(fenceX + 2.0f, 0.9f, fenceZ));
                sceneShader.setMat4("uModel", model);
                pastureFenceRail.draw();
                model = glm::translate(glm::mat4(1.0f), glm::vec3(fenceX + 2.0f, 0.45f, fenceZ));
                sceneShader.setMat4("uModel", model);
                pastureFenceRail.draw();
            }
        }

        // Boulders
        for (size_t i = 0; i < boulderPositions.size(); ++i) {
            float scale = 0.8f + (i % 3) * 0.25f;
            model = glm::translate(glm::mat4(1.0f), boulderPositions[i]);
            model = glm::scale(model, glm::vec3(scale * 1.3f, scale * 0.8f, scale));
            sceneShader.setMat4("uModel", model);
            boulder.draw();
        }

        // Trees
        for (const auto& t : trees) {
            model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, 2.0f * t.scale, 0.0f));
            model = glm::scale(model, glm::vec3(t.scale, t.scale, t.scale));
            sceneShader.setMat4("uModel", model);
            treeTrunk.draw();

            if (t.isPine) {
                for (int tier = 0; tier < 3; ++tier) {
                    float tierY = (3.5f + tier * 1.8f) * t.scale;
                    float tierScale = (1.0f - tier * 0.22f) * t.scale;
                    model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, tierY, 0.0f));
                    model = glm::scale(model, glm::vec3(tierScale, tierScale, tierScale));
                    sceneShader.setMat4("uModel", model);
                    treeFoliagePine.draw();
                }
            } else {
                model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, 5.0f * t.scale, 0.0f));
                model = glm::scale(model, glm::vec3(t.scale, t.scale * 1.1f, t.scale));
                sceneShader.setMat4("uModel", model);
                treeFoliageLeafy.draw();
            }
        }

        // ==========================================
        // 4. Draw Atmospheric Clouds & Flapping Birds
        // ==========================================
        // Clouds
        for (const auto& c : clouds) {
            model = glm::translate(glm::mat4(1.0f), c.position);
            model = glm::scale(model, glm::vec3(c.scale, c.scale, c.scale));
            sceneShader.setMat4("uModel", model);
            cloudCluster.draw();
        }

        // Birds Flying in V-Formation
        for (const auto& b : flock) {
            glm::vec3 birdPos = flockBasePos + b.offset;
            glm::mat4 birdRoot = glm::translate(glm::mat4(1.0f), birdPos);
            // Orient bird towards flight direction (+X with slight +Z)
            birdRoot = glm::rotate(birdRoot, glm::radians(75.0f), glm::vec3(0, 1, 0));

            // Bird Body
            sceneShader.setMat4("uModel", birdRoot);
            birdBody.draw();

            // Flapping Wing Hinges
            float flapAngle = std::sin(simulationTime * 9.5f + b.flapPhase) * 26.0f;

            // Left Wing
            glm::mat4 lWingModel = glm::rotate(birdRoot, glm::radians(-flapAngle), glm::vec3(0, 0, 1));
            sceneShader.setMat4("uModel", lWingModel);
            birdLeftWing.draw();

            // Right Wing
            glm::mat4 rWingModel = glm::rotate(birdRoot, glm::radians(flapAngle), glm::vec3(0, 0, 1));
            sceneShader.setMat4("uModel", rWingModel);
            birdRightWing.draw();
        }

        // ==========================================
        // 5. Draw Two Secondary Background Balloons
        // ==========================================
        // Background Balloon 1 (Sunset Fire, 0.55x scale)
        {
            float bg1Alt = 26.0f + 1.8f * std::sin(simulationTime * 0.45f + 1.2f);
            float bg1DriftX = 36.0f + 3.0f * std::sin(simulationTime * 0.18f);
            glm::vec3 bg1Pos(bg1DriftX, bg1Alt, -42.0f);

            glm::mat4 bg1Root = glm::translate(glm::mat4(1.0f), bg1Pos);
            bg1Root = glm::scale(bg1Root, glm::vec3(0.55f, 0.55f, 0.55f));

            // Envelope
            glm::mat4 envModel = glm::translate(bg1Root, glm::vec3(0.0f, 6.8f, 0.0f));
            sceneShader.setMat4("uModel", envModel);
            sunsetEnvelope.draw();

            // Equator belt
            glm::mat4 beltModel = glm::translate(bg1Root, glm::vec3(0.0f, 5.0f, 0.0f));
            sceneShader.setMat4("uModel", beltModel);
            equatorBelt.draw();

            // Basket
            glm::mat4 basketModel = glm::translate(bg1Root, glm::vec3(0.0f, -0.6f, 0.0f));
            sceneShader.setMat4("uModel", basketModel);
            wovenBasket.draw();
        }

        // Background Balloon 2 (Ocean Teal, 0.38x scale)
        {
            float bg2Alt = 22.0f + 1.4f * std::cos(simulationTime * 0.35f + 0.6f);
            float bg2DriftX = -42.0f + 2.5f * std::cos(simulationTime * 0.15f);
            glm::vec3 bg2Pos(bg2DriftX, bg2Alt, -58.0f);

            glm::mat4 bg2Root = glm::translate(glm::mat4(1.0f), bg2Pos);
            bg2Root = glm::scale(bg2Root, glm::vec3(0.38f, 0.38f, 0.38f));

            // Envelope
            glm::mat4 envModel = glm::translate(bg2Root, glm::vec3(0.0f, 6.8f, 0.0f));
            sceneShader.setMat4("uModel", envModel);
            oceanEnvelope.draw();

            // Equator belt
            glm::mat4 beltModel = glm::translate(bg2Root, glm::vec3(0.0f, 5.0f, 0.0f));
            sceneShader.setMat4("uModel", beltModel);
            equatorBelt.draw();

            // Basket
            glm::mat4 basketModel = glm::translate(bg2Root, glm::vec3(0.0f, -0.6f, 0.0f));
            sceneShader.setMat4("uModel", basketModel);
            wovenBasket.draw();
        }

        // ==========================================
        // 6. Draw Main Hot Air Balloon (Hierarchical Rig)
        // ==========================================
        // World position of balloon
        glm::mat4 balloonRoot = glm::translate(glm::mat4(1.0f), balloonPosition);

        // A. Envelope (Vivid 7-Color Rainbow Gores)
        glm::mat4 envelopeModel = glm::translate(balloonRoot, glm::vec3(0.0f, 6.8f, 0.0f));
        sceneShader.setMat4("uModel", envelopeModel);
        rainbowEnvelope.draw();

        // B. White Horizontal Equator Belt with Scalloped Swags
        glm::mat4 beltModel = glm::translate(balloonRoot, glm::vec3(0.0f, 5.0f, 0.0f));
        sceneShader.setMat4("uModel", beltModel);
        equatorBelt.draw();

        // C. White Skirt Collar at the Throat
        glm::mat4 skirtModel = glm::translate(balloonRoot, glm::vec3(0.0f, 1.9f, 0.0f));
        sceneShader.setMat4("uModel", skirtModel);
        whiteSkirt.draw();

        // D. Burner Unit & Flickering Animated Flame
        glm::mat4 burnerModel = glm::translate(balloonRoot, glm::vec3(0.0f, 1.25f, 0.0f));
        sceneShader.setMat4("uModel", burnerModel);
        burnerRing.draw();

        if (burnerActive) {
            float flameScale = 0.95f + 0.35f * std::sin(simulationTime * 24.0f);
            glm::mat4 flameModel = glm::translate(balloonRoot, glm::vec3(0.0f, 1.4f, 0.0f));
            flameModel = glm::scale(flameModel, glm::vec3(flameScale, flameScale * 1.35f, flameScale));
            sceneShader.setMat4("uModel", flameModel);
            burnerFlame.draw();
        }

        // E. Woven Basket with Pendulum Sway
        float swayAngle = 3.6f * std::sin(simulationTime * 2.2f);
        glm::mat4 basketTransform = glm::rotate(balloonRoot, glm::radians(swayAngle), glm::vec3(0, 0, 1));

        glm::mat4 basketModel = glm::translate(basketTransform, glm::vec3(0.0f, -0.6f, 0.0f));
        sceneShader.setMat4("uModel", basketModel);
        wovenBasket.draw();

        // F. 8 Suspension Rigging Cables (Connecting Skirt Collar to Basket Rim)
        float skirtRadius = 1.22f;
        float skirtY = 1.35f;
        float basketRimY = 0.30f;

        float basketAnchors[8][2] = {
            {-1.15f, -1.15f}, { 0.00f, -1.18f}, { 1.15f, -1.15f},
            { 1.18f,  0.00f}, { 1.15f,  1.15f}, { 0.00f,  1.18f},
            {-1.15f,  1.15f}, {-1.18f,  0.00f}
        };

        for (int i = 0; i < 8; ++i) {
            float angle = (float)i * (2.0f * (float)M_PI / 8.0f);
            glm::vec3 topAnchor(std::cos(angle) * skirtRadius, skirtY, std::sin(angle) * skirtRadius);
            glm::vec3 botAnchor(basketAnchors[i][0], basketRimY, basketAnchors[i][1]);

            glm::vec3 cableMid = (topAnchor + botAnchor) * 0.5f;
            glm::vec3 dir = topAnchor - botAnchor;
            float cableLen = glm::length(dir);
            dir = glm::normalize(dir);

            glm::mat4 cableModel = glm::translate(basketTransform, cableMid);
            // Orient cylinder along dir vector
            glm::vec3 upVec(0.0f, 1.0f, 0.0f);
            glm::vec3 axis = glm::cross(upVec, dir);
            float cosA = glm::dot(upVec, dir);
            if (glm::length(axis) > 0.001f) {
                cableModel = glm::rotate(cableModel, std::acos(cosA), glm::normalize(axis));
            }
            cableModel = glm::scale(cableModel, glm::vec3(1.0f, cableLen / 3.4f, 1.0f));

            sceneShader.setMat4("uModel", cableModel);
            riggingCable.draw();
        }

        // ==========================================
        // 7. Swap Buffers & Poll Events
        // ==========================================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
