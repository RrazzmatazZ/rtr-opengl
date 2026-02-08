#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm> // for std::clamp
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

const std::filesystem::path RESOURCE_ROOT = "/Users/dodge/programs/avr/rtr/rtr-opengl/src/assignment2";

std::string Path(const std::string& subPath)
{
    return (RESOURCE_ROOT / subPath).string();
}

#define RE(p) Path(p).c_str()


Camera camera(glm::vec3(0.0f, 10.0f, 5.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int window_width = 1920;
int window_height = 1080;
float lastX = (float)window_width / 2.0;
float lastY = (float)window_height / 2.0;
bool firstMouse = true;
float cameraDistance = 10.0f;

// 1: Reflect, 2: Refract, 3: Fresnel, 4: Chromatic
int renderMode = 1;

std::vector<std::string> skybox_dirs = {
    "ForbiddenCity", "GamlaStan", "GamlaStan2",
    "Medborgarplatsen", "Parliament", "Roundabout", "UnionSquare",
    "SaintLazarusChurch", "SaintLazarusChurch2", "SaintLazarusChurch3",
    "Sodermalmsallen", "CNTower", "Sodermalmsallen2"
};

//manage parameters
float refraction = 0.75f;
float fresnel_ior = 1.5f;
float chromatic_ior = 1.5f;
float chromatic_dispersion = 0.5f;

//manage rendering Skybox
int currentSkybox = 0;

template <typename... Args>
std::vector<std::string> create_skybox_paths(const std::string& dirName, Args&&... filenames)
{
    std::filesystem::path dirPath = RESOURCE_ROOT / "urban-skyboxes" / dirName;
    return {(dirPath / filenames).string()...};
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        glm::vec3 pivot = camera.Position + camera.Front * cameraDistance;

        glm::vec3 offset = camera.Position - pivot;

        float angularSpeed = 1.0f * deltaTime;

        float cosTheta = cos(angularSpeed);
        float sinTheta = sin(angularSpeed);

        float newX = offset.x * cosTheta - offset.z * sinTheta;
        float newZ = offset.x * sinTheta + offset.z * cosTheta;
        camera.Position = pivot + glm::vec3(newX, offset.y, newZ);
        glm::vec3 newDirection = glm::normalize(pivot - camera.Position);

        camera.Pitch = glm::degrees(asin(newDirection.y));
        camera.Yaw = glm::degrees(atan2(newDirection.z, newDirection.x));
        camera.ProcessMouseMovement(0.0f, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) renderMode = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) renderMode = 2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) renderMode = 3;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) renderMode = 4;

    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed)
    {
        currentSkybox = (currentSkybox + 1) % skybox_dirs.size();
        tabPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) tabPressed = false;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraDistance -= static_cast<float>(yoffset) * 2.0f;
    cameraDistance = std::clamp(cameraDistance, 5.0f, 100.0f);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Real-time Rendering Assignment2", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
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

    Renderer::Init();

    Model cubeReflectModel(RE("cube/cube.obj"), RE("cube/cube.vs"), RE("cube/reflect.fs"));
    Model cubeRefractModel(RE("cube/cube.obj"), RE("cube/cube.vs"), RE("cube/refract.fs"));
    Model cubeFresnelModel(RE("cube/cube.obj"), RE("cube/cube.vs"), RE("cube/fresnel.fs"));
    Model cubeChromaticModel(RE("cube/cube.obj"), RE("cube/cube.vs"), RE("cube/chromatic.fs"));

    Model ringReflectModel(RE("ring/ring.obj"), RE("ring/ring.vs"), RE("ring/reflect.fs"));
    Model ringRefractModel(RE("ring/ring.obj"), RE("ring/ring.vs"), RE("ring/refract.fs"));
    Model ringFresnelModel(RE("ring/ring.obj"), RE("ring/ring.vs"), RE("ring/fresnel.fs"));
    Model ringChromaticModel(RE("ring/ring.obj"), RE("ring/ring.vs"), RE("ring/chromatic.fs"));

    Model discoReflectModel(RE("discoball/discoball.obj"), RE("discoball/discoball.vs"),RE("discoball/reflect.fs"));
    Model discoRefractModel(RE("discoball/discoball.obj"), RE("discoball/discoball.vs"),RE("discoball/refract.fs"));
    Model discoFresnelModel(RE("discoball/discoball.obj"), RE("discoball/discoball.vs"),RE("discoball/fresnel.fs"));
    Model discoChromaticModel(RE("discoball/discoball.obj"), RE("discoball/discoball.vs"),RE("discoball/chromatic.fs"));

    Model diamondReflectModel(RE("diamond/diamond.obj"), RE("diamond/diamond.vs"), RE("diamond/reflect.fs"));
    Model diamondRefractModel(RE("diamond/diamond.obj"), RE("diamond/diamond.vs"), RE("diamond/refract.fs"));
    Model diamondFresnelModel(RE("diamond/diamond.obj"), RE("diamond/diamond.vs"), RE("diamond/fresnel.fs"));
    Model diamondChromaticModel(RE("diamond/diamond.obj"), RE("diamond/diamond.vs"),RE("diamond/chromatic.fs"));

    std::vector<Skybox> skyboxes;
    skyboxes.reserve(skybox_dirs.size());
    for (const auto& dirName : skybox_dirs)
    {
        auto faces = create_skybox_paths(dirName, "posx.jpg", "negx.jpg", "posy.jpg", "negy.jpg", "posz.jpg",
                                         "negz.jpg");
        skyboxes.emplace_back(faces, RE("urban-skyboxes/skybox.vs"), RE("urban-skyboxes/skybox.fs"));
    }

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(380, 280), ImGuiCond_Always);
            ImGui::Begin("Shading Parameters");
            ImGui::Text("Shading Parameters:");

            // 1: Reflect, 2: Refract, 3: Fresnel, 4: Chromatic
            ImGui::Text("Rendering Mode:");
            ImGui::RadioButton("Reflection", &renderMode, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Refraction", &renderMode, 2);
            ImGui::SameLine();
            ImGui::RadioButton("Fresnel", &renderMode, 3);
            ImGui::SameLine();
            ImGui::RadioButton("Chromatic", &renderMode, 4);

            if (renderMode == 2)
            {
                ImGui::SliderFloat("Refraction Value", &refraction, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);
            }
            if (renderMode == 3)
            {
                ImGui::SliderFloat("IOR Value", &fresnel_ior, 1.0f, 1.5f, "%.3f", ImGuiSliderFlags_NoInput);
            }

            if (renderMode == 4)
            {
                ImGui::SliderFloat("IOR Value", &chromatic_ior, 1.0f, 1.5f, "%.3f", ImGuiSliderFlags_NoInput);
                ImGui::SliderFloat("Dispersion Value", &chromatic_dispersion, .0f, 1.f, "%.3f",
                                   ImGuiSliderFlags_NoInput);
            }
            ImGui::End();
        }

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Skybox& skybox = skyboxes[currentSkybox % skyboxes.size()];

        Renderer::BeginScene(camera, (float)window_width / (float)window_height);
        Renderer::SetSkybox(skybox);

        float angle = (float)glfwGetTime() * 4.0f;

        {
            // Cube Row (Remains unchanged as baseline)
            {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 10.0f, -5.0f));
                Renderer::Submit(cubeReflectModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 1);
                });
            }
            {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 10.0f, -5.0f));
                Renderer::Submit(cubeRefractModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 2);
                });
            }
            {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 10.0f, -5.0f));
                Renderer::Submit(cubeFresnelModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 3);
                });
            }
            {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, -5.0f));
                Renderer::Submit(cubeChromaticModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 4);
                });
            }
        }
        {
            // Ring Row
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 10.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            if (renderMode == 1)
            {
                Renderer::Submit(ringReflectModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 5);
                });
            }
            else if (renderMode == 2)
            {
                Renderer::Submit(ringRefractModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 5);
                    s->setFloat("refraction", refraction);
                });
            }
            else if (renderMode == 3)
            {
                Renderer::Submit(ringFresnelModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 5);
                    s->setFloat("IOR", fresnel_ior);
                });
            }
            else if (renderMode == 4)
            {
                Renderer::Submit(ringChromaticModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 5);
                    s->setFloat("IOR", chromatic_ior);
                    s->setFloat("dispersion", chromatic_dispersion);
                });
            }
        }

        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(8.0f, 10.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            // Disco Row
            if (renderMode == 1)
            {
                Renderer::Submit(discoReflectModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 6);
                });
            }
            else if (renderMode == 2)
            {
                Renderer::Submit(discoRefractModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 6);
                    s->setFloat("refraction", refraction);
                });
            }
            else if (renderMode == 3)
            {
                Renderer::Submit(discoFresnelModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 6);
                    s->setFloat("IOR", fresnel_ior);
                });
            }
            else if (renderMode == 4)
            {
                Renderer::Submit(discoChromaticModel, model, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 6);
                    s->setFloat("IOR", chromatic_ior);
                    s->setFloat("dispersion", chromatic_dispersion);
                });
            }
        }

        {
            glm::mat4 diamondModel = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, -5.0f));
            diamondModel = glm::rotate(diamondModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            diamondModel = glm::scale(diamondModel, glm::vec3(0.01f));
            if (renderMode == 1)
            {
                Renderer::Submit(diamondReflectModel, diamondModel, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 7);
                });
            }
            else if (renderMode == 2)
            {
                Renderer::Submit(diamondRefractModel, diamondModel, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 7);
                    s->setFloat("refraction", refraction);
                });
            }
            else if (renderMode == 3)
            {
                Renderer::Submit(diamondFresnelModel, diamondModel, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 7);
                    s->setFloat("IOR", fresnel_ior);
                });
            }
            else if (renderMode == 4)
            {
                Renderer::Submit(diamondChromaticModel, diamondModel, [&](Shader* s)
                {
                    s->setVec3("cameraPos", camera.Position);
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                    s->setInt("skybox", 7);
                    s->setFloat("IOR", chromatic_ior);
                    s->setFloat("dispersion", chromatic_dispersion);
                });
            }
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
