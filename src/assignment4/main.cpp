#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include "utils/Model.h"
#include "utils/Renderer.h"
#include "utils/Camera.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#endif

const std::filesystem::path RESOURCE_ROOT = "/Users/dodge/programs/avr/rtr/rtr-opengl/src/assignment4";

std::string Path(const std::string& subPath)
{
    return (RESOURCE_ROOT / subPath).string();
}

#define RE(p) Path(p).c_str()


int window_width = 1920, window_height = 1080;
Camera camera(glm::vec3(0.0f, 2.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -20.0f);

enum FilterMode { NEAREST = 0, LINEAR, MIPMAP };

int current_mode = MIPMAP;

unsigned int CreateTexture(GLint minFilter, bool useMipmap)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    const int size = 1024;
    std::vector<unsigned char> data(size * size * 4);
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int idx = (y * size + x) * 4;
            unsigned char c = (((x >> 4) ^ (y >> 4)) & 1) ? 255 : 35;
            data[idx] = data[idx + 1] = data[idx + 2] = c;
            data[idx + 3] = 255;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (useMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        float maxAniso;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        if (maxAniso > 0.0f)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(8.0f, maxAniso));
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Real-time Rendering - Mipmap Study", NULL,
                                          NULL);
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

    Model floor(RE("floor/floor.obj"), RE("floor/model.vs"), RE("floor/model.fs"));


    unsigned int texNearest = CreateTexture(GL_NEAREST, false);
    unsigned int texLinear = CreateTexture(GL_LINEAR, false);
    unsigned int texMipmap = CreateTexture(GL_LINEAR_MIPMAP_LINEAR, true);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Controller Panel");
            ImGui::RadioButton("Nearest", &current_mode, NEAREST);
            ImGui::RadioButton("Linear", &current_mode, LINEAR);
            ImGui::RadioButton("Mipmap", &current_mode, MIPMAP);
            ImGui::Separator();
            ImGui::End();
        }

        unsigned int activeTex = (current_mode == NEAREST)
                                     ? texNearest
                                     : (current_mode == LINEAR ? texLinear : texMipmap);
        if (!floor.meshes.empty())
        {
            if (floor.meshes[0].textures.empty())
            {
                floor.AddTexture((int)activeTex, "texture_diffuse");
            }
            else
            {
                floor.meshes[0].textures[0].id = activeTex;
            }
        }

        Renderer::BeginScene(camera, (float)window_width / (float)window_height);

        Renderer::Submit(floor, glm::mat4(1.0f));

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
