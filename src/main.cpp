#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <vector>
using namespace std;

const char* VERTEX_SHADER_SOURCE = R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
layout (location = 2) in float animated;
out vec3 vertexColor;
uniform float uTime;

void main() {
    float cloudOffset = mod(uTime * 0.12 * animated, 2.4);
    vec2 animatedPosition = position;
    animatedPosition.x = mod(position.x + cloudOffset + 1.2, 2.4) - 1.2;
    gl_Position = vec4(animatedPosition, 0.0, 1.0);
    vertexColor = color;
}
)";

const char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core
in vec3 vertexColor;
out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(vertexColor, 1.0);
}
)";

struct Vertex {
    float x;
    float y;
    float red;
    float green;
    float blue;
    float animated;
};

GLuint createShaderProgram() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &VERTEX_SHADER_SOURCE, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FRAGMENT_SHADER_SOURCE, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

void addTriangle(vector<Vertex>& scene, Vertex first, Vertex second, Vertex third) {
    scene.push_back(first);
    scene.push_back(second);
    scene.push_back(third);
}

void addRectangle(vector<Vertex>& scene, float left, float bottom, float right, float top,
                  float red, float green, float blue, float animated = 0.0f) {
    Vertex bottomLeft{left, bottom, red, green, blue, animated};
    Vertex bottomRight{right, bottom, red, green, blue, animated};
    Vertex topRight{right, top, red, green, blue, animated};
    Vertex topLeft{left, top, red, green, blue, animated};
    addTriangle(scene, bottomLeft, bottomRight, topRight);
    addTriangle(scene, bottomLeft, topRight, topLeft);
}

// Callback to adjust the viewport when the window size changes
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Process input (e.g., close window on ESC press)
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    // Configure GLFW: OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create Window
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Project", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Initialize GLAD (OpenGL loader)
    int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (!version) {
        cerr << "Failed to initialize GLAD" << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    cout << "OpenGL Version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << endl;
    cout << "GPU / Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "GLSL Version:   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    // Set initial viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Build a simple colorful scene in normalized device coordinates.
    vector<Vertex> scene;
    addRectangle(scene, -1.0f, -1.0f, 1.0f, 1.0f, 0.35f, 0.70f, 0.95f); // sky
    addRectangle(scene, -1.0f, -1.0f, 1.0f, -0.35f, 0.25f, 0.60f, 0.25f); // grass
    addRectangle(scene, -0.88f, 0.42f, -0.62f, 0.68f, 1.0f, 0.82f, 0.18f); // sun

    // House and roof.
    addRectangle(scene, -0.42f, -0.35f, 0.30f, 0.30f, 0.85f, 0.35f, 0.20f);
    addTriangle(scene,
                Vertex{-0.52f, 0.30f, 0.45f, 0.12f, 0.08f},
                Vertex{0.40f, 0.30f, 0.45f, 0.12f, 0.08f},
                Vertex{-0.06f, 0.72f, 0.45f, 0.12f, 0.08f});
    addRectangle(scene, -0.12f, -0.35f, 0.05f, 0.02f, 0.25f, 0.12f, 0.08f); // door
    addRectangle(scene, -0.34f, -0.02f, -0.16f, 0.16f, 0.65f, 0.90f, 1.0f); // window
    addRectangle(scene, 0.04f, -0.02f, 0.22f, 0.16f, 0.65f, 0.90f, 1.0f); // window

    // Tree.
    addRectangle(scene, 0.58f, -0.35f, 0.70f, 0.20f, 0.38f, 0.20f, 0.08f);
    addTriangle(scene,
                Vertex{0.38f, 0.02f, 0.08f, 0.40f, 0.16f},
                Vertex{0.90f, 0.02f, 0.08f, 0.40f, 0.16f},
                Vertex{0.64f, 0.58f, 0.08f, 0.40f, 0.16f});
    addTriangle(scene,
                Vertex{0.43f, 0.28f, 0.10f, 0.50f, 0.20f},
                Vertex{0.85f, 0.28f, 0.10f, 0.50f, 0.20f},
                Vertex{0.64f, 0.78f, 0.10f, 0.50f, 0.20f});

    // Clouds marked as animated drift from left to right and wrap around.
    addRectangle(scene, -0.95f, 0.55f, -0.65f, 0.65f, 0.95f, 0.98f, 1.0f, 1.0f);
    addRectangle(scene, -0.86f, 0.62f, -0.72f, 0.72f, 0.95f, 0.98f, 1.0f, 1.0f);
    addRectangle(scene, 0.18f, 0.72f, 0.52f, 0.82f, 0.95f, 0.98f, 1.0f, 1.0f);
    addRectangle(scene, 0.28f, 0.79f, 0.43f, 0.88f, 0.95f, 0.98f, 1.0f, 1.0f);

    // Car moving in front of the house. Its animation factor makes it faster than the clouds.
    const float carSpeed = 2.4f;
    addRectangle(scene, -0.78f, -0.82f, -0.22f, -0.62f, 0.82f, 0.08f, 0.06f, carSpeed); // body
    addTriangle(scene,
                Vertex{-0.66f, -0.62f, 0.82f, 0.08f, 0.06f, carSpeed},
                Vertex{-0.48f, -0.46f, 0.82f, 0.08f, 0.06f, carSpeed},
                Vertex{-0.30f, -0.62f, 0.82f, 0.08f, 0.06f, carSpeed});
    addRectangle(scene, -0.57f, -0.60f, -0.48f, -0.50f, 0.55f, 0.85f, 0.95f, carSpeed); // window
    addRectangle(scene, -0.46f, -0.60f, -0.35f, -0.50f, 0.55f, 0.85f, 0.95f, carSpeed); // window
    addRectangle(scene, -0.69f, -0.87f, -0.56f, -0.77f, 0.04f, 0.04f, 0.05f, carSpeed); // wheel
    addRectangle(scene, -0.38f, -0.87f, -0.25f, -0.77f, 0.04f, 0.04f, 0.05f, carSpeed); // wheel

    GLuint shaderProgram = createShaderProgram();
    GLint timeLocation = glGetUniformLocation(shaderProgram, "uTime");
    GLuint vertexArrayObject = 0;
    GLuint vertexBufferObject = 0;
    glGenVertexArrays(1, &vertexArrayObject);
    glGenBuffers(1, &vertexBufferObject);
    glBindVertexArray(vertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, scene.size() * sizeof(Vertex), scene.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // 4. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        // Input
        processInput(window);

        // Rendering commands
        glClearColor(0.1f, 0.13f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform1f(timeLocation, static_cast<float>(glfwGetTime()));
        glBindVertexArray(vertexArrayObject);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(scene.size()));

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 5. Clean up resources
    glDeleteBuffers(1, &vertexBufferObject);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
