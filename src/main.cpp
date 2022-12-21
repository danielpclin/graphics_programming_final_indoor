// include standard libraries
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

// include OpenGL
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// include glm for vector/matrix math
#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Common.h"
#include "Shader.h"
#include "Model.h"
#include "Camera.h"

const float FOV = 72.0;

Camera *camera;
Shader *shader;
Shader *shadowMapShader;
Model *gray_room;
Model *trice;
glm::mat4 model_matrix(1.0f);
glm::mat4 projection_matrix(1.0f);

// Mouse variables
float mouse_last_x = 0;
float mouse_last_y = 0;
bool firstMouse = true;
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
bool capture_mouse = false;

struct RenderConfig {
    bool blinn_phong = true;
    bool directional_light_shadow = true;
    bool deferred_shading = true;
    bool normal_mapping = true;
    bool bloom = true;
} renderConfig;

//imgui state
bool show_demo_window = false;
glm::vec3 cameraPosition;
glm::vec3 cameraLookat;

void init() {
    glClearColor(0.19, 0.19, 0.19, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

//    glEnable(GL_MULTISAMPLE);

    // setup camera, models, and shaders
    camera = new Camera(glm::vec3(4.0, 1.5, -2.0), -195, -15);
    gray_room = new Model("assets/indoor/Grey_White_Room.obj");
    trice = new Model("assets/indoor/trice.obj");
    shader = new Shader("shader/texture.vs.glsl", "shader/texture.fs.glsl");
    shader->use();
    shader->setVec3("light.position", glm::vec3(-2.845, 2.028, -1.293));
    shader->setVec3("light.ambient", glm::vec3(0.1));
    shader->setVec3("light.diffuse", glm::vec3(0.7));
    shader->setVec3("light.specular", glm::vec3(0.2));

    // directional light shadow
    GLuint directionalFBO = 0;
    glGenFramebuffers(1, &directionalFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, directionalFBO);

    GLuint directionalDepthTextureID;
    glActiveTexture(0);
    glGenTextures(1, &directionalDepthTextureID);
    glBindTexture(GL_TEXTURE_2D, directionalDepthTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT16, 1024, 1024, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, directionalDepthTextureID, 0);

    glDrawBuffer(GL_NONE);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not ok" << std::endl;

    // Compute the MVP matrix from the light's point of view
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10,10,-10,10,-10,20);
    glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(-2.845, 2.028, -1.293), glm::vec3(0.542, -0.141, -0.422), glm::vec3(0,1,0));
    glm::mat4 depthModelMatrix = glm::mat4(1.0);
    glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

    shadowMapShader->setMat4("MVP", depthMVP);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

void draw() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // projection & view matrix
    shader->use();
    shader->setBool("config.blinnPhong", renderConfig.blinn_phong);
    shader->setBool("config.directionalLightShadow", renderConfig.directional_light_shadow);
    shader->setBool("config.bloom", renderConfig.bloom);
    shader->setBool("config.deferredShading", renderConfig.deferred_shading);
    shader->setBool("config.normalMapping", renderConfig.normal_mapping);

    shader->setMat4("model", model_matrix);
    shader->setMat4("view", camera->getViewMatrix());
    shader->setMat4("projection", projection_matrix);
    for (auto &mesh: gray_room->meshes) {
        glBindVertexArray(mesh.vao);
        shader->setBool("hasTexture", gray_room->materials[mesh.materialID].hasTexture);
        shader->setInt("textureMap", gray_room->materials[mesh.materialID].textureID);
        shader->setVec3("material.ambient", gray_room->materials[mesh.materialID].ambientColor);
        shader->setVec3("material.diffuse", gray_room->materials[mesh.materialID].diffuseColor);
        shader->setVec3("material.specular", gray_room->materials[mesh.materialID].specularColor);
        shader->setFloat("material.shininess", gray_room->materials[mesh.materialID].shininess);
        glDrawElements(GL_TRIANGLES, (GLint) mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid *) nullptr);
    }

    shader->setMat4("model", glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(2.05, 0.628725, -1.9)), glm::vec3(0.001)));
    for (auto &mesh: trice->meshes) {
        glBindVertexArray(mesh.vao);
        shader->setBool("hasTexture", trice->materials[mesh.materialID].hasTexture);
        shader->setInt("textureMap", trice->materials[mesh.materialID].textureID);
        shader->setVec3("material.ambient", trice->materials[mesh.materialID].ambientColor);
        shader->setVec3("material.diffuse", trice->materials[mesh.materialID].diffuseColor);
        shader->setVec3("material.specular", trice->materials[mesh.materialID].specularColor);
        shader->setFloat("material.shininess", trice->materials[mesh.materialID].shininess);
        glDrawElements(GL_TRIANGLES, (GLint) mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid *) nullptr);
    }

}

void prepare_imgui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    {
        ImGui::Begin("Render config"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);


        ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
        ImGui::Checkbox("Blinn Phong", &renderConfig.blinn_phong);
        ImGui::Checkbox("Directional light shadow", &renderConfig.directional_light_shadow);
        ImGui::Checkbox("Deferred shading", &renderConfig.deferred_shading);
        ImGui::Checkbox("Normal mapping", &renderConfig.normal_mapping);
        ImGui::Checkbox("Bloom", &renderConfig.bloom);

        ImGui::Text("Camera position %.2f, %.2f, %.2f", camera->position.x, camera->position.y, camera->position.z);
        ImGui::Text("Camera yaw: %.2f°, pitch: %.2f°", camera->yaw, camera->pitch);
        ImGui::InputFloat3("Position", (float *)&cameraPosition, "%.2f");
        ImGui::InputFloat3("Lookat", (float *)&cameraLookat, "%.2f");

        if (ImGui::Button("Update position"))
            cameraPosition = camera->position;
        ImGui::SameLine();
        if (ImGui::Button("Update lookat"))
            cameraLookat = camera->getLookAt();
        if (ImGui::Button("Set position"))
            camera->position = cameraPosition;
        ImGui::SameLine();
        if (ImGui::Button("Set lookat"))
            camera->updateLootAt(cameraLookat);

        ImGui::End();
    }

    // Render
    ImGui::Render();

}

void toggle_mouse(GLFWwindow* window) {
    if (!capture_mouse){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    capture_mouse = !capture_mouse;
}

void mouse_callback(GLFWwindow* window, double x_pos_in, double y_pos_in) {
    if (!capture_mouse)
        return;
    auto x_pos = static_cast<float>(x_pos_in);
    auto y_pos = static_cast<float>(y_pos_in);

    if (firstMouse)
    {
        mouse_last_x = x_pos;
        mouse_last_y = y_pos;
        firstMouse = false;
    }

    float x_offset = x_pos - mouse_last_x;
    float y_offset = mouse_last_y - y_pos;

    mouse_last_x = x_pos;
    mouse_last_y = y_pos;

    camera->processMouseMovement(x_offset, y_offset);
}

void mouse_button_callback(GLFWwindow* window) {
    toggle_mouse(window);
}

void process_input(GLFWwindow* window) {
    // wasd / ctrl/ space to move camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->processKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->processKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->processKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->processKeyboard(Camera::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera->processKeyboard(Camera::DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera->processKeyboard(Camera::UP, deltaTime);
}

// whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    if (width == 0 && height == 0)
        return;
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    projection_matrix = glm::perspective(glm::radians(FOV), (float) width / (float) height, 0.01f, 100.0f);

    // Re-render the scene because the current frame was drawn for the old resolution
    draw();
    glfwSwapBuffers(window);
}

static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                            const GLchar *message, const void *userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
    if (type == GL_DEBUG_TYPE_ERROR)
        system("pause");
}

int main(int argc, char *argv[]) {
    GLFWwindow *window;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(1600, 900, "Graphics programming final indoor", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSwapInterval(1);

    glEnable(GL_DEBUG_OUTPUT);
    if (glDebugMessageCallback)
        glDebugMessageCallback(debugMessageCallback, nullptr);

    printGLContextInfo();

    // setup viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);
    projection_matrix = glm::perspective(glm::radians(FOV), (float) width / (float) height, 0.01f, 100.0f);

    init();

    // setup imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // calculate frame time
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        process_input(window);

        prepare_imgui();
        // left click to lock screen for camera movement
        if (!io.WantCaptureMouse && ImGui::IsMouseClicked(0)) {
            mouse_button_callback(window);
        }
        draw();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
