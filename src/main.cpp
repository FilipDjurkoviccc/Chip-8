#include "Chip8.hpp"
#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr int WindowWidth = 800;
constexpr int WindowHeight = 600;

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

class GlfwWindow {
  public:
    GlfwWindow(int width, int height, const char* title) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (window_ == nullptr) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window_);
        glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }

        glViewport(0, 0, width, height);
    }

    ~GlfwWindow() {
        glfwTerminate();
    }

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    GLFWwindow* get() const {
        return window_;
    }

    bool shouldClose() const {
        return glfwWindowShouldClose(window_);
    }

    void close() {
        glfwSetWindowShouldClose(window_, true);
    }

    void swapBuffers() {
        glfwSwapBuffers(window_);
    }

    void pollEvents() {
        glfwPollEvents();
    }

  private:
    GLFWwindow* window_{nullptr};
};

class Chip8Input {
  public:
    void update(GLFWwindow* window, Chip8& chip8) const {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        for (const KeyBinding& binding : keyBindings_) {
            if (glfwGetKey(window, binding.glfwKey) == GLFW_PRESS) {
                chip8.setKey(binding.chip8Key);
            } else {
                chip8.unsetKey(binding.chip8Key);
            }
        }
    }

  private:
    struct KeyBinding {
        int glfwKey;
        std::size_t chip8Key;
    };

    const std::array<KeyBinding, Chip8::KeyCount> keyBindings_{{
        {GLFW_KEY_X, 0x0},
        {GLFW_KEY_1, 0x1},
        {GLFW_KEY_2, 0x2},
        {GLFW_KEY_3, 0x3},
        {GLFW_KEY_Q, 0x4},
        {GLFW_KEY_W, 0x5},
        {GLFW_KEY_E, 0x6},
        {GLFW_KEY_A, 0x7},
        {GLFW_KEY_S, 0x8},
        {GLFW_KEY_D, 0x9},
        {GLFW_KEY_Z, 0xA},
        {GLFW_KEY_C, 0xB},
        {GLFW_KEY_4, 0xC},
        {GLFW_KEY_R, 0xD},
        {GLFW_KEY_F, 0xE},
        {GLFW_KEY_V, 0xF},
    }};
};

class ScreenRenderer {
  public:
    ScreenRenderer()
        : shader_("shaders/vertexshader.vs", "shaders/fragmentshader.fs") {
        setupQuad();
    }

    ~ScreenRenderer() {
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ebo_);
    }

    ScreenRenderer(const ScreenRenderer&) = delete;
    ScreenRenderer& operator=(const ScreenRenderer&) = delete;

    void render(const Chip8& chip8) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader_.use();
        shader_.setMat4("view", glm::mat4(1.0f));
        shader_.setMat4(
            "projection",
            glm::ortho(0.0f, static_cast<float>(Chip8::ScreenWidth),
                       static_cast<float>(Chip8::ScreenHeight), 0.0f));

        glBindVertexArray(vao_);

        const Chip8::DisplayBuffer& display = chip8.display();
        for (std::size_t y = 0; y < Chip8::ScreenHeight; ++y) {
            for (std::size_t x = 0; x < Chip8::ScreenWidth; ++x) {
                if (display[y * Chip8::ScreenWidth + x] == 0) {
                    continue;
                }

                glm::mat4 model(1.0f);
                model = glm::translate(model, glm::vec3(x, y, 0.0f));
                shader_.setMat4("model", model);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
        }
    }

  private:
    void setupQuad() {
        const float vertices[] = {
            1.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
        };
        const unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3,
        };

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    Shader shader_;
    unsigned int vao_{0};
    unsigned int vbo_{0};
    unsigned int ebo_{0};
};

class Chip8Application {
  public:
    explicit Chip8Application(const std::string& romPath)
        : window_(WindowWidth, WindowHeight, "Chip-8") {
        chip8_.loadRom(romPath);
    }

    void run() {
        while (!window_.shouldClose()) {
            input_.update(window_.get(), chip8_);
            chip8_.cycle();
            renderer_.render(chip8_);
            window_.swapBuffers();
            window_.pollEvents();
        }
    }

  private:
    Chip8 chip8_;
    GlfwWindow window_;
    ScreenRenderer renderer_;
    Chip8Input input_;
};
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: chip8 <path-to-rom>\n";
        return 1;
    }

    try {
        Chip8Application app(argv[1]);
        app.run();
    } catch (const std::exception& error) {
        std::cout << error.what() << '\n';
        return 1;
    }

    return 0;
}
