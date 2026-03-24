#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include "utils/Model.h"
#include "utils/Renderer.h"
#include "utils/Camera.h"
#include <vector>
#include <cmath>
#include <filesystem>

#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif

// Resource path handling
const std::filesystem::path RESOURCE_ROOT = "/Users/dodge/programs/avr/rtr/rtr-opengl/src/assignment4";
std::string Path(const std::string& subPath) { return (RESOURCE_ROOT / subPath).string(); }
#define RE(p) Path(p).c_str()

// Global state
int window_width = 1920, window_height = 1080;
Camera camera(glm::vec3(0.0f, 2.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -20.0f);
float deltaTime = 0.0f, lastFrame = 0.0f;
float rotationSpeed = 2.0f;

enum FilterMode { MODE_NEAREST = 0, MODE_LINEAR, MODE_MIPMAP_NEAREST, MODE_MIPMAP_LINEAR };

int current_mode = MODE_MIPMAP_LINEAR;

// Dynamic texture control variables
unsigned int g_TextureID = 0;
int g_SizeExponent = 12;
int g_GridDivisor = 4;

// Function to regenerate texture content
void RefreshTexture(unsigned int texID, int exponent, int divisor)
{
    int size = (int)std::pow(2, exponent);
    glBindTexture(GL_TEXTURE_2D, texID);

    std::vector<unsigned char> data(size * size * 4);
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int idx = (y * size + x) * 4;
            // Checkerboard logic: density determined by divisor
            unsigned char c = (((x >> divisor) ^ (y >> divisor)) & 1) ? 255 : 0;
            data[idx] = data[idx + 1] = data[idx + 2] = c;
            data[idx + 3] = 255;
        }
    }

    // Upload to GPU memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    // Critical: Mipmap levels must be regenerated after size changes
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Apply texture filtering parameters
void ApplyTextureParams(unsigned int texID, int mode)
{
    glBindTexture(GL_TEXTURE_2D, texID);
    // Force disable anisotropic filtering to observe Mipmap differences clearly
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.0f);

    switch (mode)
    {
    case MODE_NEAREST:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    case MODE_LINEAR:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    case MODE_MIPMAP_NEAREST: // Bilinear (Linear within level, Nearest between levels)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    case MODE_MIPMAP_LINEAR: // Trilinear (Linear within level, Linear between levels)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
}

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Real-time Rendering - Assignment4", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Renderer::Init();
    glEnable(GL_DEPTH_TEST);

    Model floor(RE("floor/floor.obj"), RE("floor/model.vs"), RE("floor/model.fs"));

    // Initialize Texture
    glGenTextures(1, &g_TextureID);
    glBindTexture(GL_TEXTURE_2D, g_TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    RefreshTexture(g_TextureID, g_SizeExponent, g_GridDivisor);
    ApplyTextureParams(g_TextureID, current_mode);

    // Bind texture to the model meshes
    if (!floor.meshes.empty())
    {
        if (floor.meshes[0].textures.empty()) floor.AddTexture((int)g_TextureID, "texture_diffuse");
        else floor.meshes[0].textures[0].id = g_TextureID;
    }

    while (!glfwWindowShouldClose(window))
    {
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Texture Control Panel");

            ImGui::Text("Resolution Settings");
            // Use exponent slider to ensure Power-of-Two (POT) dimensions
            if (ImGui::SliderInt("Size Exponent (2^n)", &g_SizeExponent, 4, 13))
            {
                RefreshTexture(g_TextureID, g_SizeExponent, g_GridDivisor);
            }
            if (ImGui::SliderInt("Grid Divisor (Density)", &g_GridDivisor, 1, 8))
            {
                RefreshTexture(g_TextureID, g_SizeExponent, g_GridDivisor);
            }

            int current_res = (int)std::pow(2, g_SizeExponent);
            ImGui::Text("Current Resolution: %d x %d", current_res, current_res);
            ImGui::Text("VRAM Usage: %.2f MB", (current_res * current_res * 4.0f) / (1024.0f * 1024.0f));

            ImGui::Separator();
            ImGui::Text("Filter Mode");
            bool modeChanged = false;
            if (ImGui::RadioButton("Nearest", &current_mode, MODE_NEAREST)) modeChanged = true;
            if (ImGui::RadioButton("Linear", &current_mode, MODE_LINEAR)) modeChanged = true;
            if (ImGui::RadioButton("Bilinear (Mipmap Nearest)", &current_mode, MODE_MIPMAP_NEAREST)) modeChanged = true;
            if (ImGui::RadioButton("Trilinear (Mipmap Linear)", &current_mode, MODE_MIPMAP_LINEAR)) modeChanged = true;

            if (modeChanged) ApplyTextureParams(g_TextureID, current_mode);

            ImGui::Separator();
            ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 10.0f);

            ImGui::End();
        }

        Renderer::BeginScene(camera, (float)window_width / (float)window_height);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(currentFrame * rotationSpeed * 10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Renderer::Submit(floor, model);
        Renderer::EndScene();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    Renderer::Shutdown();
    glfwTerminate();
    return 0;
}
