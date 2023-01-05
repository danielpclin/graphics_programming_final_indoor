// include standard libraries
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>

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
int WIDTH = 1600;
int HEIGHT = 900;

Camera *camera;
Shader *shader;
Shader* shadowMapShader;
Shader *screenShader;
Shader* gbufferShader;
Model *gray_room;
Model *trice;
glm::mat4 model_matrix(1.0f);
glm::mat4 projection_matrix(1.0f);
GLuint depthMapFBO;
GLuint depthMap;
GLuint frameVAO;

/*----- G Buffer Begin ----- */
GLuint GBufferFBO;
GLuint GBufferTexture[6];
/*----- G Buffer End ----- */

// Point Light Shadow
GLuint pointShadowFBO;
GLuint pointShadowDepthMap;
Shader *pointLightShadowMapShader;
//

/*----- Post Process Parameters Begin ----- */
GLuint FBO;
GLuint depth_stencil_RBO;
GLuint FBODataTexture;
/*----- Post Process Parameters End ----- */

/*----- Bloom Effect Parameters Begin ----- */
Model* emissive_sphere;
glm::vec3 emissive_sphere_position = glm::vec3(1.87659, 0.4625, 0.103928);
GLuint BloomEffect_HDR_FBO;
GLuint BloomEffect_HDR_StencilBuffer;
GLuint BloomEffect_HDR_Texture;
glm::vec3 directionalLight_position = glm::vec3(-2.845, 2.028, -1.293);
Shader* BloomEffect_BlurShader;
GLuint BloomEffect_pingpongFBO[2];
GLuint BloomEffect_pingpongBuffer[2];
/*----- Bloom Effect Parameters End ----- */

/*----- SSAO Process Parameters Begin ----- */
Shader* ssaoEffectShader;
GLuint SSAO_VAO;
GLuint SSAO_FBO;
GLuint SSAO_Texture;
GLuint noiseMap;
GLuint uboSSAOkernel;

/*----- SSAO Process Parameters End ----- */


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
    bool deferred_shading = false;
    int  gbuffer = 0;
    bool normal_mapping = true;
    bool bloom = false;
    bool NPR = false;
    bool SSAO = false;
} renderConfig;

//imgui state
bool show_demo_window = false;
glm::vec3 cameraPosition;
glm::vec3 cameraLookat;

void init() {
    //Global Setting
    glClearColor(0.19, 0.19, 0.19, 1.0);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);

    // setup camera, models, and shaders
    camera = new Camera(glm::vec3(4.0, 1.5, -2.0), -195, -15);
    cameraPosition = camera->position;
    cameraLookat = camera->getLookAt();
    gray_room = new Model("assets/indoor/Grey_White_Room.obj");
    trice = new Model("assets/indoor/trice.obj");
    shader = new Shader("shader/texture.vert", "shader/texture.frag");
    shadowMapShader = new Shader("shader/shadowMap.vert", "shader/shadowMap.frag");
    pointLightShadowMapShader = new Shader("shader/pointShadowMap.vert", "shader/pointShadowMap.frag", "shader/pointShadowMap.geom");
    screenShader = new Shader("shader/screen.vert", "shader/screen.frag");
    gbufferShader = new Shader("shader/GBuffer.vert", "shader/GBuffer.frag");
    /*----- Bloom Effect Object/Shader Begin ----- */
    emissive_sphere = new Model("assets/indoor/sphere.obj");
    BloomEffect_BlurShader = new Shader("shader/BloomEffectBlur.vert", "shader/BloomEffectBlur.frag");
    /*----- Bloom Effect Object/Shader End ----- */

    /*----- SSAO Shader Begin -----*/
    ssaoEffectShader = new Shader("shader/SSAO.vert", "shader/SSAO.frag");
    /*----- SSAO Shader End -----*/

    // directional light shadow
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    glGenTextures(1, &depthMap);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT, 1024, 1024, 0,GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glActiveTexture(GL_TEXTURE0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not ok" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*----- SSAO Init. Begin ----- */
    // VAO Init.
    float vertices[] = {
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
    };
    glGenVertexArrays(1, &SSAO_VAO);

    GLuint SSAO_VBO;
    glGenBuffers(1, &SSAO_VBO);
    glBindVertexArray(SSAO_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, SSAO_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    /* ----- Kernel Generation ----- */
    
    ssaoEffectShader->setUniformBlockBinding("SSAOKernals", 0);

    const int KERNEL_SIZE = 64;
    
    glGenBuffers(1, &uboSSAOkernel);
    glBindBuffer(GL_UNIFORM_BUFFER, uboSSAOkernel);
    glm::vec4 uniformSSAOKernalPtr[KERNEL_SIZE];
    srand((unsigned int)time(0));

    for (int i = 0; i < KERNEL_SIZE; i++) {
        float scale = (float)i / (float)KERNEL_SIZE;
        scale = 0.1f + 0.9f * scale * scale;
        uniformSSAOKernalPtr[i] = glm::vec4(glm::normalize(glm::vec3(
            rand() / (float)RAND_MAX * 2.0f - 1.0f,
            rand() / (float)RAND_MAX * 2.0f - 1.0f,
            rand() / (float)RAND_MAX * 0.85f + 0.15f)) * scale,
            0.0f
        );
    }
    glBufferData(GL_UNIFORM_BUFFER, KERNEL_SIZE * sizeof(glm::vec4), uniformSSAOKernalPtr, GL_STATIC_DRAW);

    /* SSAO FBO init */
    
    glGenFramebuffers(1, &SSAO_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, SSAO_FBO);

    glGenTextures(1, &SSAO_Texture);
    glBindTexture(GL_TEXTURE_2D, SSAO_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAO_Texture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*----- Random Noise ----- */
    glGenTextures(1, &noiseMap);
    glBindTexture(GL_TEXTURE_2D, noiseMap);
    glm::vec3 noiseData[16];
    for (int i = 0; i < 16; i++) {
        noiseData[i] = glm::normalize(glm::vec3(
            rand() / (float)RAND_MAX * 2.0f - 1.0f,
            rand() / (float)RAND_MAX * 2.0f - 1.0f,
            0.0f
        ));
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGB, GL_FLOAT, noiseData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    /*----- SSAO Init. End ----- */
    /*----- Post Process FBO/Textures Init. Begin ----- */
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);

    glGenRenderbuffers(1, &depth_stencil_RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, WIDTH, HEIGHT);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_RBO);

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &FBODataTexture);
    glBindTexture(GL_TEXTURE_2D, FBODataTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    /*----- Post Process FBO/Textures Init. End ----- */

    /*----- Bloom Effect FBO/Blur Texture Init. Begin ----- */
    glGenFramebuffers(1, &BloomEffect_HDR_FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, BloomEffect_HDR_FBO);
    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &BloomEffect_HDR_Texture);
    glBindTexture(GL_TEXTURE_2D, BloomEffect_HDR_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BloomEffect_HDR_Texture, 0);

    glGenFramebuffers(2, BloomEffect_pingpongFBO);
    glGenTextures(2, BloomEffect_pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, BloomEffect_pingpongFBO[i]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, BloomEffect_pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BloomEffect_pingpongBuffer[i], 0);
    }
    /*----- Bloom Effect FBO/Blur Texture Init. End ----- */

    /*----- G Buffer Init. Begin ----- */
    glGenFramebuffers(1, &GBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, GBufferFBO);
    glGenTextures(6, GBufferTexture);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[0]); // For Diffuse
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[1]); // For Normal
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[2]); // For Coordinate
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[3]); // For Ambient
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[4]); // For Specular
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[5]); //For Depth
    // glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, WIDTH, HEIGHT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GBufferTexture[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, GBufferTexture[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, GBufferTexture[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, GBufferTexture[3], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, GBufferTexture[4], 0);
    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GBufferTexture[5], 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /*----- G Buffer Init. End ----- */

    // Point Light Shadow
    glGenFramebuffers(1, &pointShadowFBO);
    glGenTextures(1, &pointShadowDepthMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowDepthMap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointShadowDepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    

    // screen frame VAO
    GLuint frameVBO;
    float frameVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &frameVAO);
	glGenBuffers(1, &frameVBO);
	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), &frameVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

    // setup shaders
    shader->use();
    shader->setVec3("directionalLight.position", directionalLight_position);
    shader->setVec3("directionalLight.ambient", glm::vec3(0.1));
    shader->setVec3("directionalLight.diffuse", glm::vec3(0.7));
    shader->setVec3("directionalLight.specular", glm::vec3(0.2));
    shader->setInt("shadowMap", 4);
    shader->setInt("textureMap", 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void drawToScreen() {
    // draw to screen
    glViewport(0, 0, WIDTH, HEIGHT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClearColor(0.19, 0.19, 0.19, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    screenShader->use();
    screenShader->setBool("config.blinnPhong", renderConfig.blinn_phong);
    screenShader->setBool("config.directionalLightShadow", renderConfig.directional_light_shadow);
    screenShader->setBool("config.bloom", renderConfig.bloom);
    screenShader->setBool("config.deferredShading", renderConfig.deferred_shading);
    screenShader->setBool("config.normalMapping", renderConfig.normal_mapping);
    screenShader->setBool("config.NPR", renderConfig.NPR);
    glBindVertexArray(frameVAO);

    /*----- Post Process Textures Binding Begin ----- */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, FBODataTexture);
    screenShader->setInt("colorTexture", 1);

    /*----- Bloom Effect Textures Binding Begin ----- */
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, BloomEffect_HDR_Texture);
    screenShader->setInt("BloomEffect_HDR_Texture", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, BloomEffect_pingpongBuffer[0]);
    screenShader->setInt("BloomEffect_Blur_Texture", 3);
    /*----- Bloom Effect Textures Binding End ----- */

    /*----- Post Process Textures Binding End ----- */

    // Deferred Shading 
    int gbuffer_tex_idx[5];
    for (int i = 0; i < 5; i++)
        gbuffer_tex_idx[i] = i + 5;
    screenShader->setIntArray("gtex", gbuffer_tex_idx, 5);
    screenShader->setInt("gbufferidx", renderConfig.gbuffer);
    for (int i = 0; i < 5; i++)
    {
        glActiveTexture(GL_TEXTURE5 + i);
        glBindTexture(GL_TEXTURE_2D, GBufferTexture[i]);
    }

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw() {
    //Global Setting
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0x00);
    // Shadow
    // Compute the MVP matrix from the light's point of view
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-5, 5, -5, 5, 0.1, 10);
    glm::mat4 depthViewMatrix = glm::lookAt(directionalLight_position, glm::vec3(0.542, -0.141, -0.422), glm::vec3(0, 1, 0));
    glm::mat4 depthVP = depthProjectionMatrix * depthViewMatrix;

    glm::mat4 biasMatrix(
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0
    );
    glm::mat4 depthBiasVP = biasMatrix * depthVP;
    
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowMapShader->use();
    shadowMapShader->setMat4("VP", depthVP);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    shadowMapShader->setMat4("M", glm::mat4(1.0));
    for (auto &mesh: gray_room->meshes) {
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, (GLint) mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid *) nullptr);
    }
    shadowMapShader->setMat4("M", glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(2.05, 0.628725, -1.9)), glm::vec3(0.001)));
    for (auto &mesh: trice->meshes) {
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, (GLint) mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid *) nullptr);
    }

    // Point Light Shadow Pass
    float near_plane = 0.22f;
    float far_plane = 10.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
    glm::mat4 shadowTransforms[6];
    shadowTransforms[0] = shadowProj * glm::lookAt(emissive_sphere_position, emissive_sphere_position+ glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    shadowTransforms[1] = shadowProj * glm::lookAt(emissive_sphere_position, emissive_sphere_position+ glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    shadowTransforms[2] = shadowProj * glm::lookAt(emissive_sphere_position, emissive_sphere_position+ glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    shadowTransforms[3] = shadowProj * glm::lookAt(emissive_sphere_position, emissive_sphere_position+ glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    shadowTransforms[4] = shadowProj * glm::lookAt(emissive_sphere_position, emissive_sphere_position+ glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    shadowTransforms[5] = shadowProj * glm::lookAt(emissive_sphere_position, emissive_sphere_position+ glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    pointLightShadowMapShader->use();
    for (unsigned int i = 0; i < 6; ++i)
        pointLightShadowMapShader->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
    pointLightShadowMapShader->setFloat("far_plane", far_plane);
    pointLightShadowMapShader->setVec3("lightPos", emissive_sphere_position);
    pointLightShadowMapShader->setMat4("model", glm::mat4(1.0));
    for (auto& mesh : gray_room->meshes) {
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, (GLint)mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid*) nullptr);
    }
    pointLightShadowMapShader->setMat4("model", glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(2.05, 0.628725, -1.9)), glm::vec3(0.001)));
    for (auto& mesh : trice->meshes) {
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, (GLint)mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid*) nullptr);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glActiveTexture(GL_TEXTURE0);
    // Deferred  Shading
    glBindFramebuffer(GL_FRAMEBUFFER, GBufferFBO);
    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferShader->use();
    gbufferShader->setMat4("model", model_matrix);
    gbufferShader->setMat4("view", camera->getViewMatrix());
    gbufferShader->setMat4("projection", projection_matrix);
    for (auto& mesh : gray_room->meshes) {
        glBindVertexArray(mesh.vao);
        gbufferShader->setBool("hasTexture", gray_room->materials[mesh.materialID].hasTexture);
        gbufferShader->setBool("hasNormalMap", false);
        gbufferShader->setInt("tex_diffuse", 0);
        gbufferShader->setVec3("material.ambient", gray_room->materials[mesh.materialID].ambientColor);
        gbufferShader->setVec3("material.diffuse", gray_room->materials[mesh.materialID].diffuseColor);
        gbufferShader->setVec3("material.specular", gray_room->materials[mesh.materialID].specularColor);
        gbufferShader->setFloat("material.shininess", gray_room->materials[mesh.materialID].shininess);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gray_room->materials[mesh.materialID].textureID);
        glDrawElements(GL_TRIANGLES, (GLint)mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid*) nullptr);
    }

    gbufferShader->setMat4("model", glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(2.05, 0.628725, -1.9)), glm::vec3(0.001)));
    for (auto& mesh : trice->meshes) {
        glBindVertexArray(mesh.vao);
        gbufferShader->setBool("hasTexture", trice->materials[mesh.materialID].hasTexture);
        gbufferShader->setBool("hasNormalMap", trice->materials[mesh.materialID].hasNormalMap && renderConfig.normal_mapping);
        gbufferShader->setInt("tex_diffuse", 0);
        gbufferShader->setInt("NormalMap", 6);
        gbufferShader->setVec3("material.ambient", trice->materials[mesh.materialID].ambientColor);
        gbufferShader->setVec3("material.diffuse", trice->materials[mesh.materialID].diffuseColor);
        gbufferShader->setVec3("material.specular", trice->materials[mesh.materialID].specularColor);
        gbufferShader->setFloat("material.shininess", trice->materials[mesh.materialID].shininess);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, trice->materials[mesh.materialID].textureID);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, trice->materials[mesh.materialID].NormalMapID);
        glDrawElements(GL_TRIANGLES, (GLint)mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid*) nullptr);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 

    /*----- SSAO Effect Render Begin ----- */
    if (renderConfig.SSAO) {
        glBindFramebuffer(GL_FRAMEBUFFER, SSAO_FBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ssaoEffectShader->use();

        GLuint normal_tex = GBufferTexture[1];
        GLuint depth_tex = GBufferTexture[5];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normal_tex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depth_tex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseMap);

        glm::mat4 vp = projection_matrix * camera->getViewMatrix();
        ssaoEffectShader->setMat4("vp", vp);
        ssaoEffectShader->setMat4("invvp", glm::inverse(vp));
        ssaoEffectShader->setInt("normalMap", 0);
        ssaoEffectShader->setInt("depthMap", 1);
        ssaoEffectShader->setInt("noiseMap", 2);
        ssaoEffectShader->setVec2("noise_scale", glm::vec2(WIDTH / 4, HEIGHT / 4));


        glBindVertexArray(SSAO_VAO);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboSSAOkernel);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    /*----- SSAO Effect Render End ----- */

    /*----- Post Process Render Setting Begin ----- */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    /*----- Post Process Render Setting End ----- */
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, WIDTH, HEIGHT);
    // projection & view matrix
    shader->use();
    shader->setMat4("depthMVP", depthBiasVP);
    shader->setBool("config.blinnPhong", renderConfig.blinn_phong);
    shader->setBool("config.directionalLightShadow", renderConfig.directional_light_shadow);
    shader->setBool("config.bloom", renderConfig.bloom);
    shader->setBool("config.deferredShading", renderConfig.deferred_shading);
    shader->setBool("config.normalMapping", renderConfig.normal_mapping);
    shader->setBool("config.NPR", renderConfig.NPR);
    shader->setVec3("directionalLight.position", directionalLight_position);

    // point light shadow
    shader->setFloat("farPlane", far_plane);
    shader->setInt("pointShadowMap", 6);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowDepthMap);

    /*----- Bloom Effect Setting Begin ----- */
    if (renderConfig.bloom) shader->setVec3("emissive_sphere_position", emissive_sphere_position);
    /*----- Bloom Effect Setting End ----- */
    shader->setBool("config.SSAO", renderConfig.SSAO);
    shader->setBool("isLightObject", false);

    shader->setMat4("model", model_matrix);
    shader->setMat4("view", camera->getViewMatrix());
    shader->setMat4("projection", projection_matrix);

    if (renderConfig.SSAO) {
        shader->setInt("view.width", WIDTH);
        shader->setInt("view.height", HEIGHT);

        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, SSAO_Texture);
        shader->setInt("SSAO_Map", 10);
    }

    for (auto &mesh: gray_room->meshes) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gray_room->materials[mesh.materialID].textureID);
        glBindVertexArray(mesh.vao);
        shader->setBool("hasTexture", gray_room->materials[mesh.materialID].hasTexture);
        shader->setBool("hasNormalMap", false);
        shader->setVec3("material.ambient", gray_room->materials[mesh.materialID].ambientColor);
        shader->setVec3("material.diffuse", gray_room->materials[mesh.materialID].diffuseColor);
        shader->setVec3("material.specular", gray_room->materials[mesh.materialID].specularColor);
        shader->setFloat("material.shininess", gray_room->materials[mesh.materialID].shininess);
        glDrawElements(GL_TRIANGLES, (GLint) mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid *) nullptr);
    }

    shader->setMat4("model", glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(2.05, 0.628725, -1.9)), glm::vec3(0.001)));
    for (auto &mesh: trice->meshes) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, trice->materials[mesh.materialID].textureID);
        shader->setInt("NormalMap", 5);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, trice->materials[mesh.materialID].NormalMapID);
        glBindVertexArray(mesh.vao);
        shader->setBool("hasTexture", trice->materials[mesh.materialID].hasTexture);
        shader->setBool("hasNormalMap", trice->materials[mesh.materialID].hasNormalMap && renderConfig.normal_mapping);
        shader->setInt("textureMap", 0);
        shader->setVec3("material.ambient", trice->materials[mesh.materialID].ambientColor);
        shader->setVec3("material.diffuse", trice->materials[mesh.materialID].diffuseColor);
        shader->setVec3("material.specular", trice->materials[mesh.materialID].specularColor);
        shader->setFloat("material.shininess", trice->materials[mesh.materialID].shininess);
        glDrawElements(GL_TRIANGLES, (GLint) mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid *) nullptr);
    }

    /*----- Bloom Effect Render Begin ----- */
    if (renderConfig.bloom) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_RBO);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        shader->setMat4("model", glm::scale(glm::translate(glm::mat4(1.0), emissive_sphere_position), glm::vec3(0.22)));
        shader->setBool("isLightObject", true);
        for (auto& mesh : emissive_sphere->meshes) {
            glBindVertexArray(mesh.vao);
            shader->setBool("hasTexture", emissive_sphere->materials[mesh.materialID].hasTexture);
            shader->setInt("textureMap", emissive_sphere->materials[mesh.materialID].textureID);
            shader->setVec3("material.ambient", emissive_sphere->materials[mesh.materialID].ambientColor);
            shader->setVec3("material.diffuse", glm::vec3(1.0));
            shader->setVec3("material.specular", emissive_sphere->materials[mesh.materialID].specularColor);
            shader->setFloat("material.shininess", emissive_sphere->materials[mesh.materialID].shininess);
            glDrawElements(GL_TRIANGLES, (GLint)mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid*) nullptr);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, BloomEffect_HDR_FBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_RBO);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        shader->setMat4("model", glm::scale(glm::translate(glm::mat4(1.0), emissive_sphere_position), glm::vec3(0.22)));
        shader->setBool("isLightObject", true);
        for (auto& mesh : emissive_sphere->meshes) {
            glBindVertexArray(mesh.vao);
            shader->setBool("hasTexture", emissive_sphere->materials[mesh.materialID].hasTexture);
            shader->setInt("textureMap", emissive_sphere->materials[mesh.materialID].textureID);
            shader->setVec3("material.ambient", emissive_sphere->materials[mesh.materialID].ambientColor);
            shader->setVec3("material.diffuse", glm::vec3(1.0));
            shader->setVec3("material.specular", emissive_sphere->materials[mesh.materialID].specularColor);
            shader->setFloat("material.shininess", emissive_sphere->materials[mesh.materialID].shininess);
            glDrawElements(GL_TRIANGLES, (GLint)mesh.indicesCount, GL_UNSIGNED_INT, (GLvoid*) nullptr);
        }
        shader->setBool("isLightObject", false);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);

        bool BloomEffect_horizontal = true, BloomEffect_first_iteration = true;
        int BloomEffect_amount = 10;
        glViewport(0, 0, WIDTH, HEIGHT);
        BloomEffect_BlurShader->use();
        glBindVertexArray(frameVAO);
        for (unsigned int i = 0; i < BloomEffect_amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, BloomEffect_pingpongFBO[BloomEffect_horizontal]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            BloomEffect_BlurShader->setInt("horizontal", BloomEffect_horizontal);
            if (BloomEffect_first_iteration) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, BloomEffect_HDR_Texture);
                BloomEffect_BlurShader->setInt("image", 2);
            }
            else {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, BloomEffect_pingpongBuffer[!BloomEffect_horizontal]);
                BloomEffect_BlurShader->setInt("image", 3);
            }

            glDrawArrays(GL_TRIANGLES, 0, 6);

            BloomEffect_horizontal = !BloomEffect_horizontal;
            if (BloomEffect_first_iteration) BloomEffect_first_iteration = false;
        }
    }
    /*----- Bloom Effect Render End ----- */

    /*----- Post Process Render Begin ----- */
    drawToScreen();
    /*----- Post Process Render End ----- */
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
        if (renderConfig.directional_light_shadow) {
            ImGui::SliderFloat("X##Directional_light", &directionalLight_position.x, -4.0f, -2.0f);
            ImGui::SliderFloat("Y##Directional_light", &directionalLight_position.y, 1.0f, 4.0f);
            ImGui::SliderFloat("Z##Directional_light", &directionalLight_position.z, -4.0f, 1.0f);
        }

        ImGui::Checkbox("Deferred shading", &renderConfig.deferred_shading);
        if (renderConfig.deferred_shading) {
            ImGui::SliderInt("G Buffers", &renderConfig.gbuffer, 0, 4);
        }
        ImGui::Checkbox("Normal mapping", &renderConfig.normal_mapping);
        /*----- Bloom Effect ImGui Begin -----*/
        ImGui::Checkbox("Bloom", &renderConfig.bloom);
        if (renderConfig.bloom) {
            ImGui::SliderFloat("X##Point_Light", &emissive_sphere_position.x, 0.0f, 4.0f);
            ImGui::SliderFloat("Y##Point_Light", &emissive_sphere_position.y, 0.0f, 4.0f);
            ImGui::SliderFloat("Z##Point_Light", &emissive_sphere_position.z, -4.0f, 1.0f);
        }
        /*----- Bloom Effect ImGui End -----*/

        ImGui::Checkbox("SSAO", &renderConfig.SSAO);

        /*----- NPR ImGui Begin -----*/
        ImGui::Checkbox("NPR", &renderConfig.NPR);
        /*----- NPR ImGui End -----*/

        ImGui::Separator();
        ImGui::Text("Camera position %.2f, %.2f, %.2f", camera->position.x, camera->position.y, camera->position.z);
        ImGui::Text("Camera yaw: %.2f°, pitch: %.2f°", camera->yaw, camera->pitch);
        ImGui::InputFloat3("Position", (float *)&cameraPosition);
        ImGui::InputFloat3("Lookat", (float *)&cameraLookat);

        if (ImGui::Button("Set position and lookat")){
            camera->position = cameraPosition;
            camera->updateLootAt(cameraLookat);
        }


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
    WIDTH = width;
    HEIGHT = height;
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    projection_matrix = glm::perspective(glm::radians(FOV), (float) width / (float) height, 0.01f, 100.0f);

    // Deferred Render Begin
    glBindFramebuffer(GL_FRAMEBUFFER, GBufferFBO);
    glDeleteTextures(6, GBufferTexture);
    glGenTextures(6, GBufferTexture);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[0]); // For Diffuse
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[1]); // For Normal
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[2]); // For Coordinate
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[3]); // For Ambient
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[4]); // For Specular
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GBufferTexture[5]); //For Depth
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, WIDTH, HEIGHT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GBufferTexture[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, GBufferTexture[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, GBufferTexture[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, GBufferTexture[3], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, GBufferTexture[4], 0);
    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GBufferTexture[5], 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*----- SSAO -----*/
    glBindFramebuffer(GL_FRAMEBUFFER, SSAO_FBO);
    glDeleteTextures(1, &SSAO_Texture);

    glGenTextures(1, &SSAO_Texture);
    glBindTexture(GL_TEXTURE_2D, SSAO_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAO_Texture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*----- Post Process RBO/Textures Resize Begin ----- */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);

    glDeleteRenderbuffers(1, &depth_stencil_RBO);
    glGenRenderbuffers(1, &depth_stencil_RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_RBO);

    glActiveTexture(GL_TEXTURE1);
    glDeleteTextures(1, &FBODataTexture);
    glGenTextures(1, &FBODataTexture);
    glBindTexture(GL_TEXTURE_2D, FBODataTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);

    /*----- Bloom Effect Textures Resize Begin ----- */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, BloomEffect_HDR_FBO);

    glActiveTexture(GL_TEXTURE2);
    glDeleteTextures(1, &BloomEffect_HDR_Texture);
    glGenTextures(1, &BloomEffect_HDR_Texture);
    glBindTexture(GL_TEXTURE_2D, BloomEffect_HDR_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, BloomEffect_HDR_Texture, 0);

    glDeleteTextures(2, BloomEffect_pingpongBuffer);
    glGenTextures(2, BloomEffect_pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, BloomEffect_pingpongFBO[i]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, BloomEffect_pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BloomEffect_pingpongBuffer[i], 0);
    }
    /*----- Bloom Effect Textures Resize End ----- */

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /*----- Post Process RBO/Textures Resize End ----- */

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

    window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics programming final indoor", nullptr, nullptr);
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
