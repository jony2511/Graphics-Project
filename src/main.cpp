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

// Window settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 16.0f, 38.0f));
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
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Only orbit/look if Free-fly mode is active AND right mouse button is held
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

// Input handler for discrete actions and continuous movement
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
        std::cout << "[CAMERA] Switched to Mode 3: Free-fly (Use WASD / Arrow Keys)\n";
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
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x Multisampling

    // 2. Create Window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hot Air Balloon 3D - Phase 1 Architecture & Camera System", nullptr, nullptr);
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
    std::cout << "  Hot Air Balloon 3D: Phase 1 Operational\n";
    std::cout << "  OpenGL Version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << "\n";
    std::cout << "  Renderer:       " << glGetString(GL_RENDERER) << "\n";
    std::cout << "========================================================\n";
    std::cout << "Controls:\n";
    std::cout << "  [1] Overview Camera Mode\n";
    std::cout << "  [2] Follow Balloon Camera Mode\n";
    std::cout << "  [3] Free-fly Mode (WASD + QE + Arrow Keys / Right Mouse)\n";
    std::cout << "  [4] Basket POV Camera Mode\n";
    std::cout << "  [Space] Pause / Resume simulation\n";
    std::cout << "  [Esc] Exit Application\n";
    std::cout << "========================================================\n";

    // Configure Global OpenGL State
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    // 4. Build and Compile Shaders
    Shader sceneShader(SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER);

    // 5. Generate Procedural 3D Meshes
    // A. Rural Meadow Ground Plane
    Mesh meadowPlane = ModelGenerator::createPlane(180.0f, 180.0f, 40, glm::vec3(0.28f, 0.58f, 0.22f));

    // B. Wooden Launch Platform
    Mesh launchPlatform = ModelGenerator::createCube(16.0f, 0.4f, 16.0f, glm::vec3(0.55f, 0.38f, 0.24f));
    Mesh platformBorder = ModelGenerator::createCube(16.4f, 0.45f, 0.4f, glm::vec3(0.38f, 0.24f, 0.15f));
    Mesh fencePost = ModelGenerator::createCube(0.25f, 1.4f, 0.25f, glm::vec3(0.42f, 0.28f, 0.18f));
    Mesh fenceRail = ModelGenerator::createCube(3.8f, 0.12f, 0.12f, glm::vec3(0.48f, 0.32f, 0.20f));

    // C. Main Hot Air Balloon
    Mesh balloonEnvelope = ModelGenerator::createBalloonEnvelope(4.2f, 8.5f, 32, 48);
    Mesh basket = ModelGenerator::createCube(2.2f, 1.6f, 2.2f, glm::vec3(0.68f, 0.45f, 0.24f));
    Mesh basketRim = ModelGenerator::createCube(2.35f, 0.2f, 2.35f, glm::vec3(0.50f, 0.30f, 0.14f));
    Mesh supportRope = ModelGenerator::createCylinder(0.035f, 0.035f, 3.2f, 8, glm::vec3(0.30f, 0.25f, 0.20f));
    Mesh burnerRing = ModelGenerator::createCylinder(0.55f, 0.55f, 0.25f, 16, glm::vec3(0.25f, 0.25f, 0.28f));
    Mesh burnerFlame = ModelGenerator::createCone(0.45f, 1.2f, 16, glm::vec3(1.0f, 0.55f, 0.05f));

    // D. Surrounding Trees & Rural Props
    Mesh treeTrunk = ModelGenerator::createCylinder(0.45f, 0.35f, 4.0f, 12, glm::vec3(0.42f, 0.25f, 0.14f));
    Mesh treeFoliagePine = ModelGenerator::createCone(2.4f, 4.5f, 12, glm::vec3(0.15f, 0.42f, 0.18f));
    Mesh treeFoliageLeafy = ModelGenerator::createSphere(2.2f, 16, 16, glm::vec3(0.22f, 0.52f, 0.18f));
    Mesh windsockPole = ModelGenerator::createCylinder(0.12f, 0.12f, 8.0f, 12, glm::vec3(0.70f, 0.70f, 0.75f));
    Mesh windsockCloth = ModelGenerator::createCylinder(0.45f, 0.20f, 2.2f, 12, glm::vec3(0.95f, 0.30f, 0.10f));

    // Tree positions in the surrounding rural meadow
    struct TreeInstance {
        glm::vec3 pos;
        float scale;
        bool isPine;
    };
    std::vector<TreeInstance> trees = {
        {{-18.0f, 0.0f, -14.0f}, 1.2f, true},
        {{-24.0f, 0.0f,  10.0f}, 1.0f, false},
        {{ 20.0f, 0.0f, -18.0f}, 1.3f, true},
        {{ 25.0f, 0.0f,  12.0f}, 1.1f, false},
        {{-32.0f, 0.0f, -25.0f}, 1.4f, true},
        {{ 35.0f, 0.0f, -28.0f}, 1.2f, false},
        {{-14.0f, 0.0f,  28.0f}, 1.0f, false},
        {{ 18.0f, 0.0f,  30.0f}, 1.2f, true}
    };

    // Frame counter for FPS and title updates
    double lastTitleUpdate = 0.0;
    int frameCount = 0;

    // 6. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        // Delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (!isPaused) {
            simulationTime += deltaTime;
            // Phase 1 preview: balloon slowly elevates and gently hovers above pad
            balloonAltitude = 4.5f + std::sin(simulationTime * 0.9f) * 1.5f;
            balloonPosition = glm::vec3(0.0f, balloonAltitude, 0.0f);
        }

        // Process inputs
        processInput(window);

        // Update camera
        camera.update(deltaTime, balloonPosition);

        // Update Window Title with status
        frameCount++;
        if (currentFrame - lastTitleUpdate >= 0.25) {
            float fps = frameCount / static_cast<float>(currentFrame - lastTitleUpdate);
            std::ostringstream ss;
            ss << "Hot Air Balloon 3D | Mode: " << camera.getModeName()
               << " | Alt: " << std::fixed << std::setprecision(1) << balloonPosition.y << "m"
               << " | FPS: " << static_cast<int>(fps);
            glfwSetWindowTitle(window, ss.str().c_str());
            frameCount = 0;
            lastTitleUpdate = currentFrame;
        }

        // Render Background (Atmospheric Sky Blue)
        glClearColor(0.45f, 0.72f, 0.96f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate Shader
        sceneShader.use();

        // Setup Projection and View Matrices
        float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbHeight > 0) aspectRatio = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

        glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        glm::mat4 view = camera.getViewMatrix();
        sceneShader.setMat4("uProjection", projection);
        sceneShader.setMat4("uView", view);
        sceneShader.setVec3("uViewPos", camera.position);

        // Setup Lighting (Sun / Celestial Directional Light)
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.6f, -0.8f, -0.4f));
        sceneShader.setVec3("uDirLightDir", sunDir);
        sceneShader.setVec3("uDirLightColor", glm::vec3(1.0f, 0.98f, 0.88f)); // Warm daylight
        sceneShader.setVec3("uAmbientColor", glm::vec3(0.35f, 0.42f, 0.55f));

        // Burner Point Light
        glm::vec3 burnerPos = balloonPosition + glm::vec3(0.0f, 1.2f, 0.0f);
        float flameFlicker = 1.0f + 0.15f * std::sin(simulationTime * 18.0f);
        sceneShader.setVec3("uPointLightPos", burnerPos);
        sceneShader.setVec3("uPointLightColor", glm::vec3(1.0f, 0.60f, 0.10f));
        sceneShader.setFloat("uPointLightIntensity", burnerActive ? flameFlicker : 0.0f);

        // Material defaults
        sceneShader.setFloat("uSpecularStrength", 0.35f);
        sceneShader.setFloat("uShininess", 32.0f);

        // ==========================================
        // 1. Draw Rural Meadow Ground Plane
        // ==========================================
        glm::mat4 model = glm::mat4(1.0f);
        sceneShader.setMat4("uModel", model);
        meadowPlane.draw();

        // ==========================================
        // 2. Draw Wooden Launch Platform & Fences
        // ==========================================
        // Base platform
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f));
        sceneShader.setMat4("uModel", model);
        launchPlatform.draw();

        // Platform perimeter borders
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.25f, 8.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.25f, -8.0f));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(8.0f, 0.25f, 0.0f)), glm::radians(90.0f), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();
        model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f, 0.25f, 0.0f)), glm::radians(90.0f), glm::vec3(0, 1, 0));
        sceneShader.setMat4("uModel", model);
        platformBorder.draw();

        // Wooden fence posts at 4 corners
        float postCoords[4][2] = {{-7.8f, -7.8f}, {7.8f, -7.8f}, {7.8f, 7.8f}, {-7.8f, 7.8f}};
        for (int i = 0; i < 4; ++i) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(postCoords[i][0], 0.9f, postCoords[i][1]));
            sceneShader.setMat4("uModel", model);
            fencePost.draw();
        }

        // ==========================================
        // 3. Draw Windsock at the Launchpad
        // ==========================================
        glm::vec3 mastPos(9.5f, 0.0f, -9.5f);
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 4.0f, 0.0f));
        sceneShader.setMat4("uModel", model);
        windsockPole.draw();

        // Windsock cloth rotating with breeze
        float sockYaw = 40.0f + 10.0f * std::sin(simulationTime * 1.2f);
        model = glm::translate(glm::mat4(1.0f), mastPos + glm::vec3(0.0f, 7.8f, 0.0f));
        model = glm::rotate(model, glm::radians(sockYaw), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(75.0f), glm::vec3(1, 0, 0));
        sceneShader.setMat4("uModel", model);
        windsockCloth.draw();

        // ==========================================
        // 4. Draw Scattered Trees in the Meadow
        // ==========================================
        for (const auto& t : trees) {
            // Trunk
            model = glm::translate(glm::mat4(1.0f), t.pos + glm::vec3(0.0f, 2.0f * t.scale, 0.0f));
            model = glm::scale(model, glm::vec3(t.scale, t.scale, t.scale));
            sceneShader.setMat4("uModel", model);
            treeTrunk.draw();

            // Foliage
            if (t.isPine) {
                // Multi-tiered pine foliage
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
        // 5. Draw Hot Air Balloon (Hierarchical Rig)
        // ==========================================
        // Balloon root translation
        glm::mat4 balloonRoot = glm::translate(glm::mat4(1.0f), balloonPosition);

        // Gentle basket sway physics (pendulum oscillation)
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

        // Flickering flame inside throat
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
        // 6. Swap Buffers & Poll Events
        // ==========================================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
