#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "../utils/Shader.h"
#include "../utils/Camera.h"
#include "../utils/Model.h"
#include "../utils/Renderer.h"

#include <iostream>
#include <string>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

Camera camera(glm::vec3(0.0f, 2.5f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -30.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float bp_pow = 16.0f;
float bp_ks = 1.0f;
float toon_pow = 16.0f;
float toon_edge_threshold = 0.2f;
float cook_roughness = 0.3f;
float cook_f0 = 0.6f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Real-time Rendering Assignment1", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    stbi_set_flip_vertically_on_load(true);

    Renderer::Init();

    std::string modelPath = "teacup.obj";
    Model bpModel(modelPath, "teacup-blinn-phong.vs", "teacup-blinn-phong.fs");
    Model toonModel(modelPath, "teacup-toon.vs", "teacup-toon.fs");
    Model cookModel(modelPath, "teacup-cook.vs", "teacup-cook.fs");

    glm::vec3 relativeLightPos(5.0f, 6.0f, 10.0f);
    glm::vec3 leftPos(-0.5f, 1.0f, 0.0f);
    glm::vec3 middlePos(0.0f, 1.0f, 0.0f);
    glm::vec3 rightPos(0.5f, 1.0f, 0.0f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(380, 280), ImGuiCond_Always);

            ImGui::Begin("Shading Parameters");
            ImGui::Text("Adjust real-time lighting parameters:");

            ImGui::SeparatorText("Blinn-Phong");
            ImGui::SliderFloat("BP Pow", &bp_pow, 1.0f, 256.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("Ks Value", &bp_ks, 0.1f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);

            ImGui::SeparatorText("Toon Shading");
            ImGui::SliderFloat("Toon Pow", &toon_pow, 1.0f, 128.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("Edge Threshold", &toon_edge_threshold, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);

            ImGui::SeparatorText("Cook-Torrance");
            ImGui::SliderFloat("Roughness", &cook_roughness, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("F0", &cook_f0, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);

            ImGui::Separator();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer::BeginScene(camera, (float)SCR_WIDTH / (float)SCR_HEIGHT);

        float angle = (float)glfwGetTime() * 0.5f;

        // Blinn-Phong
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), leftPos);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(bpModel, model, [&](Shader* s) {
                s->setVec3("viewPos", camera.Position);
                s->setVec3("lightPos", relativeLightPos + leftPos);
                s->setFloat("powValue", bp_pow);
                s->setFloat("ks", bp_ks);
            });
        }

        // Toon
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), middlePos);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(toonModel, model, [&](Shader* s) {
                s->setVec3("viewPos", camera.Position);
                s->setVec3("lightPos", relativeLightPos + middlePos);
                s->setVec3("uBaseColor", glm::vec3(1.0f, 0.0f, 0.0f));
                s->setFloat("uNumSteps", 3.0f);
                s->setFloat("powValue", toon_pow);
                s->setFloat("uEdgeThreshold", toon_edge_threshold);
            });
        }

        // Cook-Torrance
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), rightPos);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(cookModel, model, [&](Shader* s) {
                s->setVec3("viewPos", camera.Position);
                s->setVec3("lightPos", relativeLightPos + rightPos);
                s->setVec3("uBaseColor", glm::vec3(1.0f, 0.0f, 0.0f));
                s->setFloat("uRoughness", cook_roughness);
                s->setFloat("uF0", cook_f0);
            });
        }

        Renderer::EndScene();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Renderer::Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!ImGui::GetIO().WantCaptureMouse) {
        camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
}