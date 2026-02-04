#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm> // for std::clamp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"
#include "utils/Renderer.h"
#include "utils/Skybox.h"
#include "utils/Camera.h"
#include "utils/Model.h"
#include "utils/filesystem.h"

const std::filesystem::path RESOURCE_ROOT = "/Users/dodge/programs/avr/rtr/rtr-opengl/src/assignment2";

std::string Path(const std::string& subPath) {
    return (RESOURCE_ROOT / subPath).string();
}

#define RE(p) Path(p).c_str()

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

Camera camera(glm::vec3(0.0f, 2.5f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -30.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int window_width = 1920;
int window_height = 1080;
float lastX = (float)window_width / 2.0;
float lastY = (float)window_height / 2.0;
bool firstMouse = true;
float cameraDistance = 10.0f;

std::vector<std::string> skybox_dirs = {
    "CNTower", "ForbiddenCity", "GamlaStan", "GamlaStan2",
    "Medborgarplatsen", "Parliament", "Roundabout", "UnionSquare",
    "SaintLazarusChurch", "SaintLazarusChurch2", "SaintLazarusChurch3",
    "Sodermalmsallen", "Sodermalmsallen2"
};

int currentSkybox = 2;

template <typename... Args>
std::vector<std::string> create_skybox_paths(const std::string& dirName, Args&&... filenames)
{
    std::filesystem::path dirPath = RESOURCE_ROOT / "urban-skyboxes" / dirName;
    return { (dirPath / filenames).string()... };
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
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    Renderer::Init();

    Model cubeReflectModel(RE("cube/cube.obj"),   RE("cube/cube.vs"), RE("cube/cube_reflect.fs"));
    Model cubeRefractModel(RE("cube/cube.obj"),   RE("cube/cube.vs"), RE("cube/cube_refract.fs"));
    Model cubeFresnelModel(RE("cube/cube.obj"),   RE("cube/cube.vs"), RE("cube/cube_fresnel.fs"));

    Model ringReflectModel(RE("ring/ring.obj"),   RE("ring/ring.vs"), RE("ring/ring_reflect.fs"));
    Model ringRefractModel(RE("ring/ring.obj"),   RE("ring/ring.vs"), RE("ring/ring_refract.fs"));
    Model ringFresnelModel(RE("ring/ring.obj"),   RE("ring/ring.vs"), RE("ring/ring_fresnel.fs"));
    Model ringChromaticModel(RE("ring/ring.obj"), RE("ring/ring.vs"), RE("ring/ring_chromatic.fs"));

    Model discoReflectModel(RE("discoball/discoball.obj"),   RE("discoball/discoball.vs"), RE("discoball/disco_reflect.fs"));
    Model discoRefractModel(RE("discoball/discoball.obj"),   RE("discoball/discoball.vs"), RE("discoball/disco_refract.fs"));
    Model discoFresnelModel(RE("discoball/discoball.obj"),   RE("discoball/discoball.vs"), RE("discoball/disco_fresnel.fs"));
    Model discoChromaticModel(RE("discoball/discoball.obj"), RE("discoball/discoball.vs"), RE("discoball/disco_chromatic.fs"));

    Model diamondChromaticModel(RE("diamond/diamond.obj"), RE("diamond/diamond.vs"), RE("diamond/diamond_chromatic.fs"));

    std::vector<Skybox> skyboxes;
    skyboxes.reserve(skybox_dirs.size());
    for (const auto& dirName : skybox_dirs)
    {
        auto faces = create_skybox_paths(dirName, "posx.jpg", "negx.jpg", "posy.jpg", "negy.jpg", "posz.jpg", "negz.jpg");
        skyboxes.emplace_back(faces, RE("urban-skyboxes/skybox.vs"), RE("urban-skyboxes/skybox.fs"));
    }

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Skybox& skybox = skyboxes[currentSkybox % skyboxes.size()];

        Renderer::BeginScene(camera, (float)window_width / (float)window_height);
        Renderer::SetSkybox(skybox);

        float angle = (float)glfwGetTime() * 4.0f;

        // Cube Row
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 2.0f, -5.0f));
            Renderer::Submit(cubeReflectModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 1);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 2.0f, -5.0f));
            Renderer::Submit(cubeRefractModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 2);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 2.0f, -5.0f));
            Renderer::Submit(cubeFresnelModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 3);
            });
        }

        // Ring Row
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(ringReflectModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 4);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(ringRefractModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 5);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 2.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(ringFresnelModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 6);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 2.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(ringChromaticModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 7);
            });
        }

        // Disco Row
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, -1.0f, -5.0f));
            Renderer::Submit(discoReflectModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 8);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, -1.0f, -5.0f));
            Renderer::Submit(discoRefractModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE9);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 9);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -5.0f));
            Renderer::Submit(discoFresnelModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 10);
            });
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, -1.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            Renderer::Submit(discoChromaticModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE11);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 11);
            });
        }

        // Diamond (Special scaling)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, -1.0f, -5.0f));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.01f));
            Renderer::Submit(diamondChromaticModel, model, [&](Shader* s) {
                s->setVec3("cameraPos", camera.Position);
                glActiveTexture(GL_TEXTURE12);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
                s->setInt("skybox", 12);
            });
        }

        Renderer::EndScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        currentSkybox = (currentSkybox + 1) % skybox_dirs.size();
        tabPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) tabPressed = false;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDistance -= static_cast<float>(yoffset) * 2.0f;
    cameraDistance = std::clamp(cameraDistance, 5.0f, 100.0f);
}