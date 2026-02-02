#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "utils/Renderer.h"
#include "utils/Skybox.h"
#include "utils/Camera.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

Camera camera(glm::vec3(0.0f, 2.5f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -30.0f);
int window_width = 1920;
int window_height = 1080;

float cameraDistance = 10.0f;
float cameraHeightOffset = 2.0f;

float lastX = (float)window_width / 2.0;
float lastY = (float)window_height / 2.0;
bool firstMouse = true;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Real-time Rendering Assignment2",NULL,NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::vector<std::string> faces = {
        "miramar_lf.tga", "miramar_rt.tga",
        "miramar_up.tga", "miramar_dn.tga",
        "miramar_ft.tga", "miramar_bk.tga"
    };

    Renderer::Init();


    Skybox skybox(faces, "skybox.vs", "skybox.fs");

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer::BeginScene(camera, (float)window_width / (float)window_height);
        Renderer::SetSkybox(skybox);
        Renderer::EndScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraDistance -= static_cast<float>(yoffset) * 2.0f;
    if (cameraDistance < 5.0f) cameraDistance = 5.0f;
    if (cameraDistance > 100.0f) cameraDistance = 100.0f;
}
