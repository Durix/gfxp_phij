#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// glfw and input functions
// ------------------------
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// global variables used for rendering
// -----------------------------------
Shader* shader;
Shader* phong_shading;
Shader* pbr_shading;
// @PHIJ
Shader* leaf_shading;
//-------
Shader* shadowMap_shader;
Model* carBodyModel;
Model* carPaintModel;
Model* carInteriorModel;
Model* carLightModel;
Model* carWindowsModel;
Model* carWheelModel;
Model* floorModel;
GLuint carBodyTexture;
GLuint carPaintTexture;
GLuint carLightTexture;
GLuint carWindowsTexture;
GLuint carWheelTexture;
GLuint floorTexture;
// @phij
unsigned int leaf_texture;
unsigned int leaf_texture_normal;
unsigned int leaf_texture_translusency;
unsigned int leaf_texture_roughness;
//------------
Camera camera(glm::vec3(0.0f, 1.6f, 5.0f));

Shader* skyboxShader;
unsigned int skyboxVAO; // skybox handle
unsigned int cubemapTexture; // skybox texture handle

unsigned int shadowMap, shadowMapFBO;
glm::mat4 lightSpaceMatrix;

// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
float deltaTime;
bool isPaused = false; // stop camera movement when GUI is open

float lightRotationSpeed = 1.0f;

// structure to hold lighting info
// -------------------------------
struct Light
{
    Light(glm::vec3 position, glm::vec3 color, float intensity, float radius)
        : position(position), color(color), intensity(intensity), radius(radius)
    {
    }

    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
};

// structure to hold config info
// -------------------------------
struct Config
{
    Config() : lights()
    {
        // Adding lights
        //lights.emplace_back(position, color, intensity, radius);

        // light 1
        lights.emplace_back(glm::vec3(-1.0f, 1.0f, -0.5f), glm::vec3(1.0f, 1.0f, 1.0f), 30.0f, 0.0f);

        // light 2
        lights.emplace_back(glm::vec3( 1.0f, 1.5f, 0.0f), glm::vec3(0.7f, 0.2f, 1.0f), 0.0f, 10.0f);
    }

    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 1.0f, 1.0f};
    float ambientLightIntensity = 0.25f;

    // material
    glm::vec3 reflectionColor = {0.9f, 0.9f, 0.2f};
    float ambientReflectance = 0.75f;
    float diffuseReflectance = 0.75f;
    float specularReflectance = 0.5f;
    float specularExponent = 10.0f;
    float roughness = 0.5f;
    float metalness = 0.0f;

    std::vector<Light> lights;

} config;


// function declarations
// ---------------------
void setAmbientUniforms(glm::vec3 ambientLightColor);
void setLightUniforms(Light &light);
void setupForwardAdditionalPass();
void resetForwardAdditionalPass();
void drawSkybox();
void drawShadowMap();
void drawObjects();
void drawGui();
unsigned int initSkyboxBuffers();
unsigned int loadCubemap(vector<std::string> faces);
void createShadowMap();
void setShadowUniforms();
// == PHIJ ==
void drawQuad();
unsigned int loadTexture(string name);
unsigned int loadTextureNoAlpha(string name);
unsigned int loadTextureRED(string name);
void GenerateOffsets();
float epsilon = 1.0f;
float c = 1.0f;
float maxThickness = 5.0f;
float minThickness = 0.1f;;
glm::mat4 models[100];
int instanceCount = 1;
// ==========

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 8", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetKeyCallback(window, key_input_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // load the shaders and the 3D models
    // ----------------------------------
    phong_shading = new Shader("shaders/common_shading.vert", "shaders/phong_shading.frag");
    pbr_shading = new Shader("shaders/common_shading.vert", "shaders/pbr_shading.frag");
    leaf_shading = new Shader("shaders/common_shading.vert", "shaders/leaf_shading.frag");

    shader = leaf_shading;


    // - @PHIJ Texture Loading
    leaf_texture = loadTexture("leaf05_basecolor_transparent.png"); // loads the texture
    leaf_texture_normal = loadTexture("leaf05_normal.png");
    leaf_texture_translusency = loadTextureRED("leaf05_translucency.png");
    leaf_texture_roughness = loadTextureNoAlpha("leaf05_roughnessR.png");

    // init skybox
    vector<std::string> faces
    {
            "skybox/right.tga",
            "skybox/left.tga",
            "skybox/top.tga",
            "skybox/bottom.tga",
            "skybox/front.tga",
            "skybox/back.tga"
    };
    cubemapTexture = loadCubemap(faces);
    skyboxVAO = initSkyboxBuffers();
    skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");

    createShadowMap();
    shadowMap_shader = new Shader("shaders/shadowmap.vert", "shaders/shadowmap.frag");

    GenerateOffsets(); // @PHIJ - Generate the offsets and apply the values to the uniform.


    // set up the z-buffer
    // -------------------
    glDepthRange(-1,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC
    
    glEnable(GL_FRAMEBUFFER_SRGB);
    
    // Dear IMGUI init
    // ---------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        static float lastFrame = 0.0f;
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 viewProjection = projection * view;

        processInput(window);

        // Rotate light 2
        if (lightRotationSpeed > 0.0f)
        {   
            glm::vec4 rotatedLight = glm::rotate(glm::mat4(1.0f), lightRotationSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(config.lights[1].position, 1.0f);
            config.lights[1].position = glm::vec3(rotatedLight.x, rotatedLight.y, rotatedLight.z);
        }

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        drawSkybox();

        drawShadowMap();

        shader->use();
        
        // First light + ambient
        setAmbientUniforms(config.ambientLightColor * config.ambientLightIntensity);
        setLightUniforms(config.lights[0]);
        setShadowUniforms();

        // @PHIJ - set epsilonC float to some constant,
        // KEEP IN MIND: This is the result of Epsilon * C in Beers law.
        shader->setFloat("epsilonC", epsilon * c);
        shader->setFloat("minThickness", minThickness);
        shader->setFloat("maxThickness", maxThickness);

        drawObjects();
        
        // Additional additive lights
        setupForwardAdditionalPass();
        for (int i = 1; i < config.lights.size(); ++i)
        {
            setLightUniforms(config.lights[i]);
            drawObjects();
        }
        resetForwardAdditionalPass();

        if (isPaused) {
            drawGui();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    // -------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete phong_shading;
    delete pbr_shading;
    delete shadowMap_shader;
    delete leaf_shading;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void drawGui(){

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Settings");

        ImGui::Text("Ambient light: ");
        ImGui::ColorEdit3("ambient light color", (float*)&config.ambientLightColor);
        ImGui::SliderFloat("ambient light intensity", &config.ambientLightIntensity, 0.0f, 1.0f);
        ImGui::Separator();
        
        ImGui::Text("Light 1: ");
        ImGui::DragFloat3("light 1 direction", (float*)&config.lights[0].position, .1f, -20, 20);
        ImGui::ColorEdit3("light 1 color", (float*)&config.lights[0].color);
        ImGui::SliderFloat("light 1 intensity", &config.lights[0].intensity, 0.0f, 50.0f);
        ImGui::Separator();

        ImGui::Text("Light 2: ");
        ImGui::DragFloat3("light 2 position", (float*)&config.lights[1].position, .1f, -20, 20);
        ImGui::ColorEdit3("light 2 color", (float*)&config.lights[1].color);
        ImGui::SliderFloat("light 2 intensity", &config.lights[1].intensity, 0.0f, 50.0f);
        ImGui::SliderFloat("light 2 radius", &config.lights[1].radius, 0.01f, 50.0f);
        ImGui::SliderFloat("light 2 speed", &lightRotationSpeed, 0.0f, 2.0f);
        ImGui::Separator();

        ImGui::Text("Beer's Law constants");
        ImGui::SliderFloat("Epsilon", &epsilon, 0.01f, 1.0f);
        ImGui::SliderFloat("c-value", &c, 0.01f, 1.0f);
        ImGui::Separator();
        
        ImGui::Text("Instancing");
        ImGui::SliderInt("instance Count", &instanceCount, 1, 100);
        ImGui::Separator();
        
        
        ImGui::Text("Thickness Variables");
        ImGui::SliderFloat("Max", &maxThickness, 0.01f, 10.0f);
        ImGui::SliderFloat("Min", &minThickness, 0.01f, 10.0f);
        ImGui::Separator();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void setAmbientUniforms(glm::vec3 ambientLightColor)
{
    // ambient uniforms
    shader->setVec4("ambientLightColor", glm::vec4(ambientLightColor, glm::length(ambientLightColor) > 0.0f ? 1.0f : 0.0f));
}

void setLightUniforms(Light& light)
{
    glm::vec3 lightEnergy = light.color * light.intensity;

    // TODO 8.3 : if we are using the PBR shader, multiply the lightEnergy by PI to match the color of the previous setup
    if (shader == pbr_shading)
    {
        lightEnergy *= glm::pi<float>();
    }

    // light uniforms
    shader->setVec3("lightPosition", light.position);
    shader->setVec3("lightColor", lightEnergy);
    shader->setFloat("lightRadius", light.radius);
}

void setupForwardAdditionalPass()
{
    // Remove ambient from additional passes
    setAmbientUniforms(glm::vec3(0.0f));

    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // Set depth test to GL_EQUAL (only the fragments that match the depth buffer are rendered)
    glDepthFunc(GL_EQUAL);

    // Disable shadowmap
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void resetForwardAdditionalPass()
{
    // Restore ambient
    setAmbientUniforms(config.ambientLightColor * config.ambientLightIntensity);

    //Disable blend and restore default blend function
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);

    // Restore default depth test
    glDepthFunc(GL_LESS);
}

unsigned int loadTextureNoAlpha(string name)
{
    unsigned int id = -1;
    // taken directly from: https://learnopengl.com/Getting-started/Textures
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    // "LeafTex.png" <-- This is the cutout call.
    unsigned char* data = stbi_load(name.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return id;
}

unsigned int loadTexture(string name)
{
    unsigned int id = -1;
// taken directly from: https://learnopengl.com/Getting-started/Textures
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    // "LeafTex.png" <-- This is the cutout call.
    unsigned char* data = stbi_load(name.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data); 

    return id;
}

unsigned int loadTextureRED(string name)
{
    unsigned int id = -1;
    // taken directly from: https://learnopengl.com/Getting-started/Textures
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(name.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return id;
}
// PHIJ - Inspired from Excercise 9
void drawQuad() 
{
    // Vertex Array/buffer Object identification init
    static unsigned int quadVAO = 0, quadVBO;
    if (quadVAO == 0) { // ensures that the vertex arrays are only generated once, and kept in the buffer for future use.

        // Setup positions and Texture Coordiantes (Packed as 3 verticies, and 2 texture coords. strips of 5)
        float quadVerticies[] = {
                //pos   pos   pos | txtC  txtC | norm norm norm |  tangent (3) 
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -0.25f, -0.25f, 1.0f, 1.0f, 0.0f, 0.0f,
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, -0.25f, 0.25f, 1.0f, 1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.25f, -0.25f, 1.0f, 1.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.25f, 0.25f, 1.0f, 1.0f, 0.0f, 0.0f
        };
        
        // Setup Plane Vertex Array Object
        glGenVertexArrays(1, &quadVAO); // Generates a vertex array with an associated id
        glGenBuffers(1, &quadVBO); // generates a buffer object with an associated ID
        glBindVertexArray(quadVAO); // Binds Vertex Array, so we may work on it
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO); // Binds the buffer so we may work on it.
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticies), &quadVerticies, GL_STATIC_DRAW); // sets the verticies into the buffer object that is currently bound.
        
        // pbr/common_shading (vertex) attribute array pointers.
        glEnableVertexAttribArray(0); // - vertex
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); // creates a pointer in memory to read data in the buffer. techiang it how to navigate inidvidual pieces.
        glEnableVertexAttribArray(1); // - normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float) , (void*)(5 * sizeof(float))); // ?
        glEnableVertexAttribArray(2); // - texture coords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(3); // - tangent
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); // ?
    
    }
    // bind the vertex array... again?
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instanceCount); // draw verticies in the vertex array as a triangle strip (memory effecient)
    glBindVertexArray(0); // unbinds active VAO. presumably to avoid memory overflow.
}

// init the VAO of the skybox
// --------------------------
unsigned int initSkyboxBuffers() {
    // triangles forming the six faces of a cube
    // note that the camera is placed inside of the cube, so the winding order
    // is selected to make the triangles visible from the inside
    float skyboxVertices[108]{
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    return skyboxVAO;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

void drawSkybox()
{
    // render skybox
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    skyboxShader->use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    skyboxShader->setMat4("projection", projection);
    skyboxShader->setMat4("view", view);
    skyboxShader->setInt("skybox", 0);

    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

void createShadowMap()
{
    // create depth texture
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // if you replace GL_LINEAR with GL_NEAREST you will see pixelation in the borders of the shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // if you replace GL_LINEAR with GL_NEAREST you will see pixelation in the borders of the shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture as FBO's depth buffer
    glGenFramebuffers(1, &shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawShadowMap()
{
    Shader* currShader = shader;
    shader = shadowMap_shader;

    // setup depth shader
    shader->use();

    // We use an ortographic projection since it is a directional light.
    // left, right, bottom, top, near and far values define the 3D volume relative to
    // the light position and direction that will be rendered to produce the depth texture.
    // Geometry outside of this range will not be considered when computing shadows.
    float near_plane = 1.0f;
    float shadowMapSize = 6.0f;
    float shadowMapDepthRange = 10.0f;
    float half = shadowMapSize / 2.0f;
    glm::mat4 lightProjection = glm::ortho(-half, half, -half, half, near_plane, near_plane + shadowMapDepthRange);
    glm::mat4 lightView = glm::lookAt(glm::normalize(config.lights[0].position) * shadowMapDepthRange * 0.5f, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // setup framebuffer size
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // bind our depth texture to the frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

    // clear the depth texture/depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // draw scene from the light's perspective into the depth texture
    drawObjects();

    // unbind the depth texture from the frame buffer, now we can render to the screen (frame buffer) again
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    shader = currShader;
}

void setShadowUniforms()
{
    // shadow uniforms
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader->setInt("shadowMap", 6);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    //shader->setFloat("shadowBias", config.shadowBias * 0.01f);
}

void drawObjects()
{
    // the typical transformation uniforms are already set for you, these are:
    // projection (perspective projection matrix)
    // view (to map world space coordinates to the camera space, so the camera position becomes the origin)
    // model (for each model part we draw) <-- @PHIJ -- omitted for this project --

    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;
    
    // camera position
    shader->setVec3("camPosition", camera.Position);
    // set viewProjection matrix uniform
    shader->setMat4("viewProjection", viewProjection);

    // set up skybox texture
    shader->setInt("skybox", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // @PHIJ -- Draw Quad --
    shader->use(); // applies current shader.
    
    shader->setMat4("model", glm::mat4(1)); // Sets the identity matrix to model (?)
    shader->setMat4("viewProjection", viewProjection); // applies the view projection matrix.

    glEnable(GL_DEPTH_TEST); 

    //-- Alpha blending (OGL stuff)
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    //-- Textre binding to shader uniforms. (Only leaf shader takes these)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, leaf_texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, leaf_texture_normal);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, leaf_texture_translusency);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, leaf_texture_roughness);

    
    shader->setInt("texture_diffuse1", 1);
    shader->setInt("texture_normal1", 2);
    shader->setInt("texture_translucency1", 7);
    shader->setInt("texture_roughness1", 8);

    drawQuad(); // draws the quad.

}

void GenerateOffsets() {
    int index = 0;
    float rLow = 0.6f;  
    float rHigh = 1.6f;
 
    for (int i = 0; i < 100; i++) {
        
        
        glm::mat4 baseMatrix = glm::mat4(1.0);
        // - translate to place in world
        // -- places leafs in a wall - did try a cloud implementation but since layering isn't implemented, the effect is somewhat jarring
        //float r = static_cast <float> (rand() / static_cast <float> (RAND_MAX / 10.0f));
        baseMatrix = glm::translate(baseMatrix, glm::vec3(i % 10, (int)i / 10, 0.0));

        // - rotate by a random float value. (RAND_MAX/6.28 is to randomize radians. 6.28 = full circle)
        
        float rx = static_cast <float> (rand() / static_cast <float> (RAND_MAX / 6.28f));
        float ry = static_cast <float> (rand() / static_cast <float> (RAND_MAX / 6.28f));
        float rz = static_cast <float> (rand() / static_cast <float> (RAND_MAX / 6.28f));
        baseMatrix = glm::rotate(baseMatrix, rz , glm::vec3(0.0f, 0.0f, 1.0f));
        baseMatrix = glm::rotate(baseMatrix, ry, glm::vec3(0.0f, 1.0f, 0.0f));
        baseMatrix = glm::rotate(baseMatrix, rx, glm::vec3(1.0f, 0.0f, 0.0f));

        // - scale up and or down. 
        float r = rLow + static_cast <float> (rand() / static_cast <float> (RAND_MAX/(rHigh - rLow)));
        baseMatrix = glm::scale(baseMatrix, glm::vec3(r, r, 1.0));
        models[index++] = baseMatrix;
        
    }   
    
    // send array to uniform in common_shading.vert
    shader->use(); // we can do this with any shader, seeing as they are all bound to the common_shading vertex shader.
    for (unsigned int i = 0; i < 100; i++)
    {
        //shader->setVec2((("offSets[" + std::to_string(i) + "]")), offsets[i]);
        shader->setMat4((("models[" + std::to_string(i) + "]")), models[i]);

    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (isPaused)
        return;

    // movement commands
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){

    // camera rotation
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = (float)posX;
        lastY = (float)posY;
        firstMouse = false;
    }

    float xoffset = (float)posX - lastX;
    float yoffset = lastY - (float)posY; // reversed since y-coordinates go from bottom to top

    lastX = (float)posX;
    lastY = (float)posY;

    if (isPaused)
        return;

    // we use the handy camera class from LearnOpenGL to handle our camera
    camera.ProcessMouseMovement(xoffset, yoffset);
}


void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods){
    // controls pause mode
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        isPaused = !isPaused;
        glfwSetInputMode(window, GLFW_CURSOR, isPaused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}