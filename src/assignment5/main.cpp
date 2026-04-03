#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "utils/Renderer.h"
#include "utils/Model.h"
#include "utils/Mesh.h"
#include "utils/Shader.h"
#include "utils/Camera.h"
#include "generator.hpp"
#include "utils/Skybox.h"

const std::filesystem::path RESOURCE_ROOT = "/Users/dodge/programs/avr/rtr/rtr-opengl/src/assignment5";

std::string Path(const std::string& subPath)
{
    return (RESOURCE_ROOT / subPath).string();
}

#define RE(p) Path(p).c_str()

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 3.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

Mesh createOceanGrid(int resolution, float size)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    float halfSize = size / 2.0f;
    float step = size / (float)resolution;

    for (int i = 0; i <= resolution; ++i)
    {
        for (int j = 0; j <= resolution; ++j)
        {
            Vertex v;
            v.Position = glm::vec3(-halfSize + j * step, 0.0f, -halfSize + i * step);
            v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.TexCoords = glm::vec2((float)j / resolution, (float)i / resolution);

            v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            v.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
            for (int k = 0; k < 4; ++k)
            {
                v.m_BoneIDs[k] = 0;
                v.m_Weights[k] = 0.0f;
            }
            vertices.push_back(v);
        }
    }

    for (int i = 0; i < resolution; ++i)
    {
        for (int j = 0; j < resolution; ++j)
        {
            int topLeft = i * (resolution + 1) + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * (resolution + 1) + j;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    return Mesh(vertices, indices, textures);
}


float windSpeed = 10.0f;
int numWaves = 60;
float minWave = 0.05f;
float maxWave = 50.0f;
float amplitudeAmplify = 1.0f;
float timespec = 1.2f;


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Ocean Rendering V2 (Dynamic LOD)", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

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

    Model oceanModel("dummy", "ocean-BRDF.vs", "ocean-BRDF.fs", false);

    oceanModel.meshes.push_back(createOceanGrid(512, 200.0f));

    auto waveParams = OceanWaveGenerator::generateWaves(numWaves, windSpeed, minWave, maxWave);

    oceanModel.modelShader->use();

    oceanModel.modelShader->setInt("u_ActiveWaves", numWaves);

    int uploadCount = std::min(numWaves, 60);

    for (int i = 0; i < uploadCount; ++i)
    {
        float visualAmplitude = waveParams[i].amplitude * amplitudeAmplify;

        std::string waveName = "u_Waves[" + std::to_string(i) + "]";
        glUniform4f(glGetUniformLocation(oceanModel.modelShader->ID, waveName.c_str()),
                    waveParams[i].k_x, waveParams[i].k_y, visualAmplitude, waveParams[i].omega);

        std::string paramName = "u_WaveParams[" + std::to_string(i) + "]";
        glUniform4f(glGetUniformLocation(oceanModel.modelShader->ID, paramName.c_str()),
                    waveParams[i].lambda, 0.0f, 0.0f, 0.0f);
    }

    oceanModel.modelShader->setVec3("u_SunDir", glm::normalize(glm::vec3(0.5f, 0.15f, -1.0f)));
    oceanModel.modelShader->setVec3("u_SunColor", glm::vec3(1.0f, 0.85f, 0.6f) * 12.0f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float oceanTime = currentFrame * timespec;

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.7f, 0.85f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        Renderer::BeginScene(camera, aspectRatio);

        Renderer::SetSkybox(skybox);

        glm::mat4 modelMatrix = glm::mat4(1.0f);

        Renderer::Submit(oceanModel, modelMatrix, [&](Shader* shader)
        {
            shader->setFloat("u_Time", oceanTime);
            shader->setVec3("u_CameraPos", camera.Position);
        });

        Renderer::EndScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Renderer::Shutdown();
    glfwTerminate();
    return 0;
}
