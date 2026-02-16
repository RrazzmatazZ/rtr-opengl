#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "stb_image.h"
#include "utils/Renderer.h"
#include "utils/Skybox.h"
#include "utils/Camera.h"
#include "utils/Model.h"


int window_width = 1920, window_height = 1080;

const std::filesystem::path RESOURCE_ROOT = "/Users/dodge/programs/avr/rtr/rtr-opengl/src/assignment3";

std::string Path(const std::string& subPath)
{
    return (RESOURCE_ROOT / subPath).string();
}

#define RE(p) Path(p).c_str()


Camera camera(glm::vec3(0.0f, 2.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -30.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;


glm::vec3 lightPos = glm::vec3(5.0f, 2.0f, 1.0f);
glm::vec3 leftPos = glm::vec3(-1.5f, 0.0f, 0.0f);
glm::vec3 rightPos = glm::vec3(1.5f, 0.0f, 0.0f);
glm::vec3 midPos = glm::vec3(0.0f, 0.0f, 0.0f);


int mappingMode = 0; // 0: None, 1: Bump, 2: Normal
float bumpScale = 1.0f;
int rotateMode = 0;

int shadingMode = 0; // 0: Blinn-Phong, 1: Cook-Torrance
float roughness = 0.5f;
float metallic = 0.1f;
float kS_Blinn = 0.5f;
float shininess = 32.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Real-time Rendering - Assignment3",NULL,NULL);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


#pragma region imgui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion imgui


    Model metalCup = Model(RE("metal/metal_plate_2k.gltf"),RE("metal/shader.vs"),RE("metal/shader.fs"));
    metalCup.AddTexture(RE("metal/textures/metal_plate_nor_gl_2k.jpg"), "texture_normal");
    metalCup.AddTexture(RE("metal/textures/metal_plate_diff_2k.jpg"), "texture_diffuse");

    Model rockCup = Model(RE("teacup.obj"),RE("rock/shader.vs"),RE("rock/shader.fs"));
    rockCup.AddTexture(RE("rock/textures/rock_tile_floor_diff_2k.jpg"), "texture_diffuse");
    rockCup.AddTexture(RE("rock/textures/rock_tile_floor_nor_gl_2k.jpg"), "texture_normal");

    Model woodCup = Model(RE("teacup.obj"),RE("wood/shader.vs"),RE("wood/shader.fs"));
    woodCup.AddTexture(RE("wood/textures/wood_floor_deck_diff_2k.jpg"), "texture_diffuse");
    woodCup.AddTexture(RE("wood/textures/wood_floor_deck_nor_gl_2k.jpg"), "texture_normal");

    std::vector<std::string> skybox_paths = {
        RE("skybox/miramar_lf.tga"),
        RE("skybox/miramar_rt.tga"),
        RE("skybox/miramar_up.tga"),
        RE("skybox/miramar_dn.tga"),
        RE("skybox/miramar_ft.tga"),
        RE("skybox/miramar_bk.tga"),

    };
    Skybox skybox = Skybox(skybox_paths,RE("skybox/skybox.vs"), RE("skybox/skybox.fs"));

    Renderer::Init();

    while (!glfwWindowShouldClose(window))
    {
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (rotateMode == 1)
        {
            float orbitAngle = .2f;
            float orbitRadius = 5.0f;
            camera.Orbit(orbitAngle, orbitRadius);
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(400, 280), ImGuiCond_Always);
            ImGui::Begin("Shading Parameters");
            ImGui::Text("Global Configuration:");

            ImGui::Separator();
            ImGui::Text("Lighting Model:");
            ImGui::RadioButton("Blinn-Phong", &shadingMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Cook-Torrance (PBR)", &shadingMode, 1);

            if (shadingMode != 0)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "PBR Settings:");
                ImGui::SliderFloat("Roughness", &roughness, 0.05f, 10.0f);
                ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
            }


            ImGui::Separator();
            ImGui::Text("Detail Mapping:");
            ImGui::RadioButton("None", &mappingMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Bump", &mappingMode, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Normal", &mappingMode, 2);

            if (mappingMode == 1)
            {
                ImGui::SliderFloat("Bump Scale", &bumpScale, 0.0f, 50.0f, "%.3f");
            }

            ImGui::Separator();
            ImGui::Text("Rotation:");
            ImGui::RadioButton("Object Rotation", &rotateMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Camera Orbit", &rotateMode, 1);

            ImGui::End();
        }


        Renderer::BeginScene(camera, (float)window_width / (float)window_height);
        Renderer::SetSkybox(skybox);

        float angle = static_cast<float>(glfwGetTime()) * 8.0f;

        auto setupShader = [&](Shader* s)
        {
            s->setVec3("lightPos", lightPos + midPos);
            s->setInt("mappingMode", mappingMode);
            s->setFloat("bumpScale", bumpScale);
            s->setInt("shadingMode", shadingMode);
            s->setFloat("roughness", roughness);
            s->setFloat("metallic", metallic);
            s->setFloat("specularStrength", kS_Blinn);
            s->setFloat("shininess", shininess);
        };

        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, midPos);
            if (rotateMode == 0) model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

            Renderer::Submit(metalCup, model, [&](Shader* s)
            {
                s->setVec3("lightPos", lightPos + midPos);
                setupShader(s);
            });
        }
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, leftPos);
            if (rotateMode == 0) model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

            Renderer::Submit(rockCup, model, [&](Shader* s)
            {
                s->setVec3("lightPos", lightPos + leftPos);
                setupShader(s);
            });
        }
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, rightPos);
            if (rotateMode == 0) model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

            Renderer::Submit(woodCup, model, [&](Shader* s)
            {
                s->setVec3("lightPos", lightPos + rightPos);
                setupShader(s);
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
