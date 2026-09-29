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
float balloonAltitude = 2.5f;
glm::vec3 balloonPosition(0.0f, 2.5f, 0.0f);
float simulationTime = 0.0f;
float windmillAngle = 0.0f;

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

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
        camera.setMode(CAMERA_OVERVIEW);
        std::cout << "[CAMERA] Switched to Mode 1: Overview (Scenic Landscape View)\n";
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
        std::cout << "[CAMERA] Switched to Mode 3: Free-fly (WASD + QE + Arrow Keys / Right Mouse)\n";
        key3Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) {
        key3Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !key4Pressed) {
        camera.setMode(CAMERA_BASKET_POV);
        std::cout << "[CAMERA] Switched to Mode 4: Basket POV (Passenger Horizon View)\n";
        key4Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) {
        key4Pressed = false;
    }

    // Pause / Resume
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        isPaused = !isPaused;
        std::cout << "[SIMULATION] " << (isPaused ? "PAUSED" : "RESUMED") << "\n";
        spacePressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spacePressed = false;
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

        // Arrow keys rotate / orbit
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hot Air Balloon 3D - Phase 2: Procedural Rural Landscape & Architecture", nullptr, nullptr);
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
    std::cout << "  Hot Air Balloon 3D: Phase 2 Operational\n";
    std::cout << "  OpenGL Version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << "\n";
    std::cout << "  Renderer:       " << glGetString(GL_RENDERER) << "\n";
    std::cout << "========================================================\n";
    std::cout << "Phase 2 Features Active:\n";
    std::cout << "  * Vast Rolling Rural Meadow Terrain & Surrounding Hills\n";
    std::cout << "  * Winding Curved Dirt Country Road\n";
    std::cout << "  * Countryside Red Barn with Gabled Roof & Grain Silo\n";
    std::cout << "  * Traditional Dutch Windmill with Rotating Lattice Sails\n";
    std::cout << "  * Farmhouse Country Cottage with Stone Chimney\n";
    std::cout << "  * Golden Cylindrical Hay Bales & Pasture Split-Rail Fences\n";
    std::cout << "  * Timber Launchpad with Fence, Gate & Striped Windsock\n";
    std::cout << "========================================================\n";

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    // 4. Build and Compile Shaders
    Shader sceneShader(SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER);

    // 5. Generate Procedural 3D Meshes for Rural Landscape
    // A. Terrain & Road
    Mesh rollingTerrain = ModelGenerator::createRollingTerrain(240.0f, 240.0f, 80);
    Mesh dirtRoad = ModelGenerator::createCurvedDirtRoad();

    // B. Wooden Launch Platform & Enclosure
    Mesh launchPlatform = ModelGenerator::createCube(16.0f, 0.4f, 16.0f, glm::vec3(0.56f, 0.38f, 0.24f));
    Mesh platformBorder = ModelGenerator::createCube(16.4f, 0.45f, 0.4f, glm::vec3(0.38f, 0.24f, 0.15f));
    Mesh fencePost = ModelGenerator::createCube(0.25f, 1.4f, 0.25f, glm::vec3(0.42f, 0.28f, 0.18f));
    Mesh fenceRail = ModelGenerator::createCube(3.8f, 0.12f, 0.12f, glm::vec3(0.48f, 0.32f, 0.20f));

    // Platform Mast & Windsock
    Mesh windsockPole = ModelGenerator::createCylinder(0.12f, 0.12f, 8.5f, 12, glm::vec3(0.72f, 0.72f, 0.76f));
    Mesh windsockCone = ModelGenerator::createStripedWindsock(0.50f, 0.22f, 2.4f, 16, 5);
    Mesh floodlightHead = ModelGenerator::createCone(0.35f, 0.5f, 12, glm::vec3(0.20f, 0.20f, 0.22f));

    // C. Countryside Red Barn
    Mesh barnWalls = ModelGenerator::createCube(14.0f, 6.5f, 18.0f, glm::vec3(0.68f, 0.18f, 0.14f));
    Mesh barnRoof = ModelGenerator::createPrism(15.2f, 4.2f, 18.8f, glm::vec3(0.25f, 0.25f, 0.28f));
    Mesh barnDoors = ModelGenerator::createCube(4.0f, 4.5f, 0.2f, glm::vec3(0.92f, 0.90f, 0.85f));
    Mesh barnDoorCross = ModelGenerator::createCube(3.6f, 0.25f, 0.25f, glm::vec3(0.68f, 0.18f, 0.14f));
    Mesh siloTower = ModelGenerator::createCylinder(2.4f, 2.4f, 10.0f, 18, glm::vec3(0.65f, 0.65f, 0.68f));
    Mesh siloCap = ModelGenerator::createSphere(2.45f, 12, 18, glm::vec3(0.45f, 0.45f, 0.48f));

    // D. Dutch Country Windmill
    Mesh windmillBase = ModelGenerator::createCylinder(4.6f, 3.2f, 13.5f, 16, glm::vec3(0.78f, 0.74f, 0.68f));
    Mesh windmillRoof = ModelGenerator::createCone(3.6f, 3.8f, 16, glm::vec3(0.32f, 0.22f, 0.16f));
    Mesh windmillBalcony = ModelGenerator::createCylinder(4.2f, 4.2f, 0.35f, 16, glm::vec3(0.42f, 0.28f, 0.18f));
    Mesh windmillHub = ModelGenerator::createSphere(0.75f, 12, 12, glm::vec3(0.28f, 0.24f, 0.20f));
    Mesh windmillBlade = ModelGenerator::createWindmillBlade(7.8f, 1.35f, glm::vec3(0.45f, 0.30f, 0.18f), glm::vec3(0.92f, 0.90f, 0.82f));

    // E. Farmhouse Country Cottage
    Mesh cottageWalls = ModelGenerator::createCube(9.0f, 4.5f, 7.5f, glm::vec3(0.85f, 0.80f, 0.70f));
    Mesh cottageRoof = ModelGenerator::createPrism(10.0f, 3.0f, 8.2f, glm::vec3(0.75f, 0.32f, 0.18f));
    Mesh cottageChimney = ModelGenerator::createCube(1.2f, 5.2f, 1.2f, glm::vec3(0.45f, 0.42f, 0.40f));
    Mesh cottageDoor = ModelGenerator::createCube(1.4f, 2.8f, 0.15f, glm::vec3(0.42f, 0.25f, 0.14f));

    // F. Rural Props (Hay Bales, Split-Rail Fences, Boulders)
    Mesh hayBale = ModelGenerator::createHayBale(1.1f, 2.2f);
    Mesh pastureFencePost = ModelGenerator::createCube(0.2f, 1.2f, 0.2f, glm::vec3(0.45f, 0.30f, 0.18f));
    Mesh pastureFenceRail = ModelGenerator::createCube(4.2f, 0.10f, 0.10f, glm::vec3(0.50f, 0.34f, 0.20f));
    Mesh boulder = ModelGenerator::createSphere(1.2f, 10, 10, glm::vec3(0.52f, 0.52f, 0.50f));

    // G. Flora (Evergreen Conifers & Deciduous Leafy Trees)
    Mesh treeTrunk = ModelGenerator::createCylinder(0.45f, 0.35f, 4.0f, 12, glm::vec3(0.42f, 0.25f, 0.14f));
    Mesh treeFoliagePine = ModelGenerator::createCone(2.4f, 4.5f, 12, glm::vec3(0.14f, 0.40f, 0.16f));
    Mesh treeFoliageLeafy = ModelGenerator::createSphere(2.2f, 16, 16, glm::vec3(0.22f, 0.52f, 0.18f));

    // Trees layout across rural valley
    struct TreeInstance {
        glm::vec3 pos;
        float scale;
        bool isPine;
    };
    std::vector<TreeInstance> trees = {
        // Group near Barn
        {{-42.0f, 0.0f,  18.0f}, 1.3f, false},
        {{-45.0f, 0.0f,  30.0f}, 1.5f, false},
        {{-22.0f, 0.0f,  32.0f}, 1.1f, false},
        {{-28.0f, 0.0f,  12.0f}, 1.2f, true},

        // Group along the Dirt Road
        {{ 10.0f, 0.0f,  20.0f}, 1.0f, false},
        {{ 14.0f, 0.0f,  38.0f}, 1.2f, false},
        {{-04.0f, 0.0f,  45.0f}, 1.1f, true},
        {{ 32.0f, 0.0f,  65.0f}, 1.4f, false},

        // Group around Windmill knoll
        {{ 45.0f, 1.2f, -18.0f}, 1.3f, true},
        {{ 42.0f, 1.0f, -34.0f}, 1.4f, true},
        {{ 26.0f, 0.2f, -32.0f}, 1.2f, false},

        // Periphery hill forests
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

    // Hay bales positions in harvested pasture
    std::vector<glm::vec3> hayBalePositions = {
        {-20.0f, 1.1f, 16.0f},
        {-16.0f, 1.1f, 22.0f},
        {-24.0f, 1.1f, 20.0f},
        {-22.0f, 1.1f, 26.0f},
        {-18.0f, 1.1f, 28.0f}
    };

    // Boulders positions
    std::vector<glm::vec3> boulderPositions = {
        { 16.0f, 0.6f, -10.0f},
        {-12.0f, 0.5f,  12.0f},
        { 24.0f, 0.7f,  16.0f},
        {-26.0f, 0.8f, -14.0f}
    };

    // H. Main Hot Air Balloon (from Phase 1)
    Mesh balloonEnvelope = ModelGenerator::createBalloonEnvelope(4.2f, 8.5f, 32, 48);
    Mesh basket = ModelGenerator::createCube(2.2f, 1.6f, 2.2f, glm::vec3(0.68f, 0.45f, 0.24f));
    Mesh basketRim = ModelGenerator::createCube(2.35f, 0.2f, 2.35f, glm::vec3(0.50f, 0.30f, 0.14f));
    Mesh supportRope = ModelGenerator::createCylinder(0.035f, 0.035f, 3.2f, 8, glm::vec3(0.30f, 0.25f, 0.20f));
    Mesh burnerRing = ModelGenerator::createCylinder(0.55f, 0.55f, 0.25f, 16, glm::vec3(0.25f, 0.25f, 0.28f));
    Mesh burnerFlame = ModelGenerator::createCone(0.45f, 1.2f, 16, glm::vec3(1.0f, 0.55f, 0.05f));

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
            // Balloon gentle hovering motion above launchpad
            balloonAltitude = 4.8f + std::sin(simulationTime * 0.9f) * 1.6f;
            balloonPosition = glm::vec3(0.0f, balloonAltitude, 0.0f);

            // Windmill sails continuous rotation
            windmillAngle += 45.0f * deltaTime;
            if (windmillAngle > 360.0f) windmillAngle -= 360.0f;
        }

        processInput(window);
        camera.update(deltaTime, balloonPosition);

        // Update Window Title with status
        frameCount++;
        if (currentFrame - lastTitleUpdate >= 0.25) {
            float fps = frameCount / static_cast<float>(currentFrame - lastTitleUpdate);
            std::ostringstream ss;
            ss << "Hot Air Balloon 3D | Phase 2 Rural Scene | " << camera.getModeName()
               << " | Alt: " << std::fixed << std::setprecision(1) << balloonPosition.y << "m"
               << " | FPS: " << static_cast<int>(fps);
            glfwSetWindowTitle(window, ss.str().c_str());
            frameCount = 0;
            lastTitleUpdate = currentFrame;
        }

        // Render Background (Atmospheric Sky Blue)
        glClearColor(0.46f, 0.74f, 0.98f, 1.0f);
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
        sceneShader.setVec3("uDirLightColor", glm::vec3(1.0f, 0.98f, 0.88f));
        sceneShader.setVec3("uAmbientColor", glm::vec3(0.36f, 0.44f, 0.56f));

        // Burner Point Light
        glm::vec3 burnerPos = balloonPosition + glm::vec3(0.0f, 1.2f, 0.0f);
        float flameFlicker = 1.0f + 0.15f * std::sin(simulationTime * 18.0f);
        sceneShader.setVec3("uPointLightPos", burnerPos);
        sceneShader.setVec3("uPointLightColor", glm::vec3(1.0f, 0.60f, 0.10f));
        sceneShader.setFloat("uPointLightIntensity", burnerActive ? flameFlicker : 0.0f);

        sceneShader.setFloat("uSpecularStrength", 0.35f);
        sceneShader.setFloat("uShininess", 32.0f);

        glm::mat4 model(1.0f);

        // ==========================================
        // 1. Draw Rolling Rural Meadow & Dirt Road
        // ==========================================
        model = glm::mat4(1.0f);
        sceneShader.setMat4("uModel", model);
        rollingTerrain.draw();
        dirtRoad.draw();

        // ==========================================
        // 2. Draw Launch Platform, Fences & Mast
        // ==========================================
        // Wooden platform
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f));
        sceneShader.setMat4("uModel", model);
        launchPlatform.draw();

        // Platform perimeter borders (leaving gap on south side for dirt road entry)
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.25f, -8.0f)); // North
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(8.0f, 0.25f, 0.0f)), glm::radians(90.0f), glm::vec3(0, 1, 0)); // East
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f, 0.25f, 0.0f)), glm::radians(90.0f), glm::vec3(0, 1, 0)); // West
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        // Split south border for entrance gate
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.5f, 0.25f, 8.0f));
        model = glm::scale(model, glm::vec3(0.35f, 1.0f, 1.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        model = glm::translate(glm::mat4(1.0f), glm::vec3(5.5f, 0.25f, 8.0f));
        model = glm::scale(model, glm::vec3(0.35f, 1.0f, 1.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        // Platform perimeter fence posts and rails
        float platformPosts[8][2] = {
            {-7.8f, -7.8f}, {0.0f, -7.8f}, {7.8f, -7.8f},
            {-7.8f,  0.0f},                {7.8f,  0.0f},
            {-7.8f,  7.8f}, {-2.5f, 7.8f}, {2.5f, 7.8f} // gap between -2.5 and +2.5
        };
        for (int i = 0; i < 8; ++i) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(platformPosts[i][0], 0.9f, platformPosts[i][1]));
            sceneShader.setMat4("uModel", model);
            fencePost.draw();
        }

        // Platform fence rails
        float platformRails[5][4] = {
            {-3.9f, -7.8f, 0.0f, 0.0f}, {3.9f, -7.8f, 0.0f, 0.0f}, // North rails
            {-7.8f, -3.9f, 90.0f, 0.0f}, {-7.8f, 3.9f, 90.0f, 0.0f}, // West rails
            { 7.8f, -3.9f, 90.0f, 0.0f}                               // East rail
        };
        for (int i = 0; i < 5; ++i) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(platformRails[i][0], 1.0f, platformRails[i][1]));
            if (platformRails[i][2] != 0.0f) model = glm::rotate(model, glm::radians(platformRails[i][2]), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            fenceRail.draw();

            // Lower rail
            model = glm::translate(glm::mat4(1.0f), glm::vec3(platformRails[i][0], 0.5f, platformRails[i][1]));
            if (platformRails[i][2] != 0.0f) model = glm::rotate(model, glm::radians(platformRails[i][2]), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            fenceRail.draw();
        }

        // Windsock mast on platform edge
        glm::vec3 mastPos(9.8f, 0.0f, -9.8f);
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 4.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windsockPole.draw();

        // Floodlight head on the mast
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(-0.35f, 8.2f, 0.35f));
        model = glm::rotate(model, glm::radians(135.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        floodlightHead.draw();

        // Dynamic fluttering striped windsock
        float windHeading = 48.0f + 8.0f * std::sin(simulationTime * 1.4f);
        float sockFlutter = 82.0f + 5.0f * std::sin(simulationTime * 4.5f);
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 8.3f, 0.0f));
        model = glm::rotate(model, glm::radians(windHeading), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(sockFlutter), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        windsockCone.draw();

        // ==========================================
        // 3. Draw Countryside Red Barn & Silo
        // ==========================================
        glm::vec3 barnPos(-32.0f, 0.0f, 25.0f);
        float barnYaw = 25.0f;

        glm::mat4 barnBase = glm::translate(glm::mat4(1.0f), barnPos);
        barnBase = glm::rotate(barnBase, glm::radians(barnYaw), glm::vec3(0, 1, 0));

        // Barn main walls
        model = glm::translate(barnBase, glm::vec3(0.0f, 3.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        barnWalls.draw();

        // Barn pitched gabled roof
        model = glm::translate(barnBase, glm::vec3(0.0f, 6.5f, 0.0f));
        sceneShader.setMat4("uModel", model);
        barnRoof.draw();

        // Barn double doors on the front (Z = +9.0)
        model = glm::translate(barnBase, glm::vec3(0.0f, 2.25f, 9.05f));
        sceneShader.setMat4("uModel", model);
        barnDoors.draw();

        // Barn door cross brace
        model = glm::translate(barnBase, glm::vec3(0.0f, 2.25f, 9.15f));
        sceneShader.setMat4("uModel", model);
        barnDoorCross.draw();

        // Silo tower beside the barn
        glm::vec3 siloPos = barnPos + glm::vec3(9.5f, 0.0f, -2.0f);
        model = glm::translate(glm::mat4(1.0f), siloPos + glm::vec3(0.0f, 5.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        siloTower.draw();

        model = glm::translate(glm::mat4(1.0f), siloPos + glm::vec3(0.0f, 10.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        siloCap.draw();

        // ==========================================
        // 4. Draw Dutch Country Windmill
        // ==========================================
        glm::vec3 windmillPos(38.0f, 0.5f, -28.0f);
        glm::mat4 windmillBaseTrans = glm::translate(glm::mat4(1.0f), windmillPos);

        // Stone tower
        model = glm::translate(windmillBaseTrans, glm::vec3(0.0f, 6.75f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windmillBase.draw();

        // Balcony gallery
        model = glm::translate(windmillBaseTrans, glm::vec3(0.0f, 9.2f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windmillBalcony.draw();

        // Conical roof cap
        model = glm::translate(windmillBaseTrans, glm::vec3(0.0f, 15.4f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windmillRoof.draw();

        // Rotor Hub (facing slightly south-west toward the launch area)
        glm::vec3 hubPos = windmillPos + glm::vec3(-2.8f, 13.8f, 0.8f);
        model = glm::translate(glm::mat4(1.0f), hubPos);
        model = glm::rotate(model, glm::radians(70.0f), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        windmillHub.draw();

        // 4 Rotating Sails
        for (int i = 0; i < 4; ++i) {
            float bladeAngle = windmillAngle + i * 90.0f;
            glm::mat4 bladeModel = glm::translate(glm::mat4(1.0f), hubPos);
            bladeModel = glm::rotate(bladeModel, glm::radians(70.0f), glm::vec3(0, 1, 0));
            bladeModel = glm::rotate(bladeModel, glm::radians(bladeAngle), glm::vec3(0, 0, 1));
            sceneShader.setMat4("uModel", bladeModel);
            windmillBlade.draw();
        }

        // ==========================================
        // 5. Draw Farmhouse Country Cottage
        // ==========================================
        glm::vec3 cottagePos(28.0f, 0.0f, 26.0f);
        glm::mat4 cottageTrans = glm::translate(glm::mat4(1.0f), cottagePos);
        cottageTrans = glm::rotate(cottageTrans, glm::radians(-35.0f), glm::vec3(0, 1, 0));

        // Cottage walls
        model = glm::translate(cottageTrans, glm::vec3(0.0f, 2.25f, 0.0f));
        sceneShader.setMat4("uModel", model);
        cottageWalls.draw();

        // Terracotta gabled roof
        model = glm::translate(cottageTrans, glm::vec3(0.0f, 4.5f, 0.0f));
        sceneShader.setMat4("uModel", model);
        cottageRoof.draw();

        // Stone chimney
        model = glm::translate(cottageTrans, glm::vec3(-3.2f, 4.0f, 2.2f));
        sceneShader.setMat4("uModel", model);
        cottageChimney.draw();

        // Front door
        model = glm::translate(cottageTrans, glm::vec3(0.0f, 1.4f, 3.8f));
        sceneShader.setMat4("uModel", model);
        cottageDoor.draw();

        // ==========================================
        // 6. Draw Hay Bales in the Harvested Pasture
        // ==========================================
        for (const auto& pos : hayBalePositions) {
            model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1)); // Lay horizontally
            model = glm::rotate(model, glm::radians(pos.x * 3.0f), glm::vec3(0, 1, 0));
            sceneShader.setMat4("uModel", model);
            hayBale.draw();
        }

        // ==========================================
        // 7. Draw Split-Rail Pasture Fences
        // ==========================================
        // Fence line separating barn pasture from road
        for (int i = 0; i < 7; ++i) {
            float fenceX = -38.0f + i * 4.0f;
            float fenceZ = 12.0f;
            model = glm::translate(glm::mat4(1.0f), glm::vec3(fenceX, 0.6f, fenceZ));
            sceneShader.setMat4("uModel", model);
            pastureFencePost.draw();

            if (i < 6) {
                // Top rail
                model = glm::translate(glm::mat4(1.0f), glm::vec3(fenceX + 2.0f, 0.9f, fenceZ));
                sceneShader.setMat4("uModel", model);
                pastureFenceRail.draw();
                // Bottom rail
                model = glm::translate(glm::mat4(1.0f), glm::vec3(fenceX + 2.0f, 0.45f, fenceZ));
                sceneShader.setMat4("uModel", model);
                pastureFenceRail.draw();
            }
        }

        // ==========================================
        // 8. Draw Decorative Boulders
        // ==========================================
        for (size_t i = 0; i < boulderPositions.size(); ++i) {
            float scale = 0.8f + (i % 3) * 0.25f;
            model = glm::translate(glm::mat4(1.0f), boulderPositions[i]);
            model = glm::scale(model, glm::vec3(scale * 1.3f, scale * 0.8f, scale));
            sceneShader.setMat4("uModel", model);
            boulder.draw();
        }

        // ==========================================
        // 9. Draw Rural Trees (Conifers & Leafy)
        // ==========================================
        for (const auto& t : trees) {
            // Trunk
            model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, 2.0f * t.scale, 0.0f));
            model = glm::scale(model, glm::vec3(t.scale, t.scale, t.scale));
            sceneShader.setMat4("uModel", model);
            treeTrunk.draw();

            if (t.isPine) {
                // Multi-tiered evergreen foliage
                for (int tier = 0; tier < 3; ++tier) {
                    float tierY = (3.5f + tier * 1.8f) * t.scale;
                    float tierScale = (1.0f - tier * 0.22f) * t.scale;
                    model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, tierY, 0.0f));
                    model = glm::scale(model, glm::vec3(tierScale, tierScale, tierScale));
                    sceneShader.setMat4("uModel", model);
                    treeFoliagePine.draw();
                }
            } else {
                // Clustered round foliage
                model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, 5.0f * t.scale, 0.0f));
                model = glm::scale(model, glm::vec3(t.scale, t.scale * 1.1f, t.scale));
                sceneShader.setMat4("uModel", model);
                treeFoliageLeafy.draw();
            }
        }

        // ==========================================
        // 10. Draw Hot Air Balloon (Hierarchical Rig)
        // ==========================================
        glm::mat4 balloonRoot = glm::translate(glm::mat4(1.0f), balloonPosition);

        // Gentle basket sway physics
        float swayAngle = 3.5f * std::sin(simulationTime * 2.2f);
        glm::mat4 basketTransform = glm::rotate(balloonRoot, glm::radians(swayAngle), glm::vec3(0, 0, 1));

        // A. Envelope (Teardrop balloon with colored gores)
        glm::mat4 envelopeModel = glm::translate(balloonRoot, glm::vec3(0.0f, 6.2f, 0.0f));
        sceneShader.setMat4("uModel", envelopeModel);
        balloonEnvelope.draw();

        // B. Burner Unit & Animated Flame
        glm::mat4 burnerModel = glm::translate(balloonRoot, glm::vec3(0.0f, 1.4f, 0.0f));
        sceneShader.setMat4("uModel", burnerModel);
        burnerRing.draw();

        float flameScale = 0.85f + 0.35f * std::sin(simulationTime * 24.0f);
        glm::mat4 flameModel = glm::translate(balloonRoot, glm::vec3(0.0f, 1.5f, 0.0f));
        flameModel = glm::scale(flameModel, glm::vec3(flameScale, flameScale * 1.25f, flameScale));
        sceneShader.setMat4("uModel", flameModel);
        burnerFlame.draw();

        // C. Woven Basket
        glm::mat4 basketModel = glm::translate(basketTransform, glm::vec3(0.0f, -1.0f, 0.0f));
        sceneShader.setMat4("uModel", basketModel);
        basket.draw();

        glm::mat4 rimModel = glm::translate(basketTransform, glm::vec3(0.0f, -0.1f, 0.0f));
        sceneShader.setMat4("uModel", rimModel);
        basketRim.draw();

        // D. 4 Connecting Rigging Ropes
        float ropeOffsets[4][2] = {{-0.95f, -0.95f}, {0.95f, -0.95f}, {0.95f, 0.95f}, {-0.95f, 0.95f}};
        for (int i = 0; i < 4; ++i) {
            glm::mat4 ropeModel = glm::translate(basketTransform, glm::vec3(ropeOffsets[i][0], 0.6f, ropeOffsets[i][1]));
            sceneShader.setMat4("uModel", ropeModel);
            supportRope.draw();
        }

        // ==========================================
        // 11. Swap Buffers & Poll Events
        // ==========================================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
