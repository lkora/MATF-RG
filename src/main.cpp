#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

void setLights(Shader lightingShader, glm::vec3 pointLightPositions[]);

void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

// Saving program state
struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    int screenWidth;
    int screenHeight;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 islandPosition = glm::vec3(0.0f);
    float islandScale = 1.0f;
    PointLight pointLight;
    bool blinnLighting = true;
    bool wireframe = false;
    bool hdr = true;
    float hdrExposure = 1.0f;
    float hdrGamma = 2.2f;
    int effectSelected = 0;
    bool bloom = false;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
    void UpdateRatio(int width, int height);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
            << clearColor.b << '\n'
            << ImGuiEnabled << '\n'
            << camera.Position.x << '\n'
            << camera.Position.y << '\n'
            << camera.Position.z << '\n'
            << camera.Front.x << '\n'
            << camera.Front.y << '\n'
            << camera.Front.z << '\n'
            << screenWidth << '\n'
            << screenHeight << '\n'
            << blinnLighting << '\n'
            << hdr << '\n'
            << hdrExposure << '\n'
            << hdrGamma << '\n'
            << bloom << '\n'
            << effectSelected << '\n'
            << wireframe << '\n';
}
void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
                >> clearColor.b
                >> ImGuiEnabled
                >> camera.Position.x
                >> camera.Position.y
                >> camera.Position.z
                >> camera.Front.x
                >> camera.Front.y
                >> camera.Front.z
                >> screenWidth
                >> screenHeight
                >> blinnLighting
                >> hdr
                >> hdrExposure
                >> hdrGamma
                >> bloom
                >> effectSelected
                >> wireframe;
    }
}
void ProgramState::UpdateRatio(int width, int height){
    screenWidth = width;
    screenHeight = height;
}
ProgramState *programState;

void drawTrees(Shader modelShader, Model treeModel);
void drawSnail(Shader modelShader, Model snailModel);
void drawIsland(Shader modelShader, Model islandModel);

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Snail Island", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.FontGlobalScale = 1.5;
    (void) io;


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    // Culling
    glCullFace(GL_FRONT);
    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader screenShader("resources/shaders/framebuffer.vs", "resources/shaders/framebuffer.fs");
    Shader ourShader("resources/shaders/model_lighting_phong.vs", "resources/shaders/model_lighting_phong.fs");
    Shader skyboxShader("resources/shaders/skyboxShader.vs", "resources/shaders/skyboxShader.fs");
    Shader instanceShader("resources/shaders/instanceShader.vs", "resources/shaders/instanceShader.fs");
    Shader hdrShader("resources/shaders/hdrShader.vs", "resources/shaders/hdrShader.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    //    ourShader.setInt("numPointLights", 4);

    // load models
    // -----------
    Model islandModel("resources/objects/island/land/island.obj");
    islandModel.SetShaderTextureNamePrefix("material.");
    Model snailModel("resources/objects/island/snail/snail.obj");
    snailModel.SetShaderTextureNamePrefix("material.");
    Model treeModel("resources/objects/island/tree/tree.obj");
    treeModel.SetShaderTextureNamePrefix("material.");
    Model cloudModel("resources/objects/cloud/cloud.obj");
    cloudModel.SetShaderTextureNamePrefix("material.");

    float skyboxVertices[] = {
            // positions
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,

            1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,

            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,

            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f
    };
    glm::vec3 pointLightPositions[] = {
            glm::vec3(programState->islandPosition.x + 8.5f, programState->islandPosition.y + 3.2f, programState->islandPosition.z + 4.5f),
            glm::vec3(programState->islandPosition.x + 6.5f, programState->islandPosition.y + 3.7f, programState->islandPosition.z - 3.0f),
            glm::vec3(programState->islandPosition.x - 5.2f, programState->islandPosition.y + 3.7f, programState->islandPosition.z + 9.0f),
            glm::vec3(programState->islandPosition.x - 10.5f, programState->islandPosition.y + 3.7f, programState->islandPosition.z + 7.0f),
            glm::vec3(programState->islandPosition.x + 0.5f, programState->islandPosition.y + 6.0f, programState->islandPosition.z + 3.0f)
    };
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };
    // -------------- INSTANCING -------------
    // generate a large list of semi-random model transformation matrices
    // ------------------------------------------------------------------
    unsigned int amount = 1000;
    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(glfwGetTime()); // initialize random seed
    float radius = 20.0f;
    float offset = 25.0f;
    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: position the clouds to the right area
        if(i < amount/2)
            model = glm::translate(model, glm::vec3(rand() % 100 - 50, 6 + rand() % 10, rand() % 100 - 50));
        else
            model = glm::translate(model, glm::vec3(rand() % 100 - 50, -4 - rand() % 10, rand() % 100 - 50));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        model = glm::scale(model, glm::vec3(scale));
        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 60);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));
        // 4. now add to list of matrices
        modelMatrices[i] = model;
    }
    // configure instanced array
    // -------------------------
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < cloudModel.meshes.size(); i++)
    {
        unsigned int VAO = cloudModel.meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }
    // -------------- ----------- -------------

    // Culling
    glFrontFace(GL_CW);

    // Setting lights
    PointLight& pointLight = programState->pointLight;
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    // Quad VAO
    // screen quad VAO
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);

    // Load skybox textures
    vector<std::string> faces{
            FileSystem::getPath("resources/textures/skybox/space/right.png"),
            FileSystem::getPath("resources/textures/skybox/space/left.png"),
            FileSystem::getPath("resources/textures/skybox/space/top.png"),
            FileSystem::getPath("resources/textures/skybox/space/bottom.png"),
            FileSystem::getPath("resources/textures/skybox/space/front.png"),
            FileSystem::getPath("resources/textures/skybox/space/back.png")
    };
    // OLD
//    vector<std::string> faces{
//            FileSystem::getPath("resources/textures/skybox/sky/right.jpg"),
//            FileSystem::getPath("resources/textures/skybox/sky/left.jpg"),
//            FileSystem::getPath("resources/textures/skybox/sky/top.jpg"),
//            FileSystem::getPath("resources/textures/skybox/sky/bottom.jpg"),
//            FileSystem::getPath("resources/textures/skybox/sky/front.jpg"),
//            FileSystem::getPath("resources/textures/skybox/sky/back.jpg")
//    };
    unsigned int cubemapTexture = loadCubemap(faces);
    {
//    // ------- configure (floating point) framebuffers --------
//    // --------------------------------------------------------
//    unsigned int hdrFBO;
//    glGenFramebuffers(1, &hdrFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
//    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
//    unsigned int colorBuffers[2];
//    glGenTextures(2, colorBuffers);
//    for (unsigned int i = 0; i < 2; i++)
//    {
//        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//        // attach texture to framebuffer
//        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
//    }
//
//    // create and attach depth buffer (renderbuffer)
//    unsigned int rboDepth;
//    glGenRenderbuffers(1, &rboDepth);
//    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
//    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
//    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
//    glDrawBuffers(2, attachments);
//    // finally check if framebuffer is complete
//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//        std::cout << "ERROR::FRAMEBUFFER:: RBODepth Framebuffer not complete!" << std::endl;
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // framebuffer configuration
    // -------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
//    // create a color attachment texture
//    unsigned int textureColorbuffer;
//    glGenTextures(1, &textureColorbuffer);
//    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
//    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: RBO Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Pingpong Framebuffer not complete!" << std::endl;
    }
    // --------------------------------------------------------


    {
        // floating point framebuffer configuration
        // ------------------------------------
//    unsigned int hdrFBO;
//    glGenFramebuffers(1, &hdrFBO);
//    // create floating point color buffer
//    unsigned int colorBuffer;
//    glGenTextures(1, &colorBuffer);
//    glBindTexture(GL_TEXTURE_2D, colorBuffer);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    // create depth buffer (renderbuffer)
//    unsigned int rboDepth;
//    glGenRenderbuffers(1, &rboDepth);
//    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
//    // attach buffers
//    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//        std::cout << "ERROR::FRAMEBUFFER:: HDR Framebuffer not complete!" << std::endl;
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);

//    // create and attach depth buffer (renderbuffer)
//    unsigned int rboDepth;
//    glGenRenderbuffers(1, &rboDepth);
//    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
//    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
//    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
//    glDrawBuffers(2, attachments);
//    // finally check if framebuffer is complete
//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//        std::cout << "Framebuffer not complete!" << std::endl;
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    // Shader configuration
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    ourShader.use();
    ourShader.setInt("material.diffuse", 0);
    ourShader.setInt("material.specular", 1);
//    hdrShader.use();
//    hdrShader.setInt("hdrBuffer", 0);
    blurShader.use();
    blurShader.setInt("image", 0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // draw in wireframe
        if(programState->wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // bind to framebuffer and draw scene as we normally would to color texture
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        // make sure we clear the framebuffer's content
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);

        // Skybox shader set
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        glm::mat4 view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // Draw skybox
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setBool("blinn", programState->blinnLighting);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 30.0f);
        // view/projection transformations
        ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        setLights(ourShader, pointLightPositions);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // ------------- Objects -------------
        drawIsland(ourShader, islandModel);
        drawSnail(ourShader, snailModel);
        drawTrees(ourShader, treeModel);
        // Set cloud shader
        instanceShader.use();
        instanceShader.setMat4("projection", projection);
        instanceShader.setMat4("view", view);
        // Draw clouds
        instanceShader.setInt("texture_diffuse", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cloudModel.textures_loaded[0].id); // note: we also made the textures_loaded vector public (instead of private) from the model class.
        for (unsigned int i = 0; i < cloudModel.meshes.size(); i++) {
            glBindVertexArray(cloudModel.meshes[i].VAO);
            glDrawElementsInstanced(GL_TRIANGLES, cloudModel.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }
        // -------------------------------------


        // Reset wireframe drawing so that it doesn't try to draw quads
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // Clear all relevant buffers
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the quad plane on default framebuffer
        screenShader.use();
        screenShader.setInt("bloom", programState->bloom);
        screenShader.setInt("option", programState->effectSelected);
        screenShader.setInt("hdr", programState->hdr);
        screenShader.setFloat("exposure", programState->hdrExposure);
        screenShader.setFloat("gamma", programState->hdrGamma);
        // Bind bloom and non bloom
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);

        renderQuad();
        //glBindVertexArray(quadVAO);
        //glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        //glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw imgui
        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");

    // Cleaning up
    delete programState;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &framebuffer);
    for(unsigned int i = 0; i < amount; i++){
        glDeleteVertexArrays(1, &(cloudModel.meshes[i].VAO));
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();

    return 0;
}

// Drawing functions
// -------------------------
void drawTrees(Shader modelShader, Model treeModel){
    // Tree 1
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3((programState->islandPosition.x + 8.0f) * programState->islandScale,
                                            (programState->islandPosition.x + 1.0f) * programState->islandScale,
                                            (programState->islandPosition.x + 4.0f) * programState->islandScale));
    model = glm::scale(model, glm::vec3(programState->islandScale));
    model = glm::rotate(model, 30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    modelShader.setMat4("model", model);
    // Tree 2
    treeModel.Draw(modelShader);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3((programState->islandPosition.x + 6.0f) * programState->islandScale,
                                            (programState->islandPosition.x + 0.5f) * programState->islandScale,
                                            (programState->islandPosition.x - 3.0f) * programState->islandScale));
    model = glm::scale(model, glm::vec3(programState->islandScale));
    model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    modelShader.setMat4("model", model);
    treeModel.Draw(modelShader);
    //Tree 3
    treeModel.Draw(modelShader);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3((programState->islandPosition.x - 1.8f) * programState->islandScale,
                                            (programState->islandPosition.x + 0.2f) * programState->islandScale,
                                            (programState->islandPosition.x + 1.0f) * programState->islandScale));
    model = glm::scale(model, glm::vec3(programState->islandScale));
    model = glm::rotate(model, AI_DEG_TO_RAD(220), glm::vec3(0.0f, 1.0f, 0.0f));
    modelShader.setMat4("model", model);
    treeModel.Draw(modelShader);

}
void drawSnail(Shader modelShader, Model snailModel){
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3((programState->islandPosition.x + 0.4f) * programState->islandScale,
                                            (programState->islandPosition.x + 0.6f) * programState->islandScale,
                                            (programState->islandPosition.x + 6.0f) * programState->islandScale)); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(programState->islandScale / 4));    // it's a bit too big for our scene, so scale it down
    model = glm::rotate(model, AI_DEG_TO_RAD(180), glm::vec3(0.2f, 1.0f, 1.0f));
    modelShader.setMat4("model", model);
    snailModel.Draw(modelShader);
}
void drawIsland(Shader modelShader, Model islandModel){
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, programState->islandPosition); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(programState->islandScale));    // it's a bit too big for our scene, so scale it down
    modelShader.setMat4("model", model);
    islandModel.Draw(modelShader);
}
// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Setup window");
        ImGui::Text("Base");
        ImGui::ColorEdit3("Background clear color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Island position", (float*)&programState->islandPosition);
        ImGui::DragFloat("Island scale", &programState->islandScale, 0.05, 0.1, 4.0);
        ImGui::Text("Lights");
        ImGui::Checkbox("Blinn-Phong lighting", &programState->blinnLighting);
        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::Text("HDR");
        ImGui::Checkbox("HDR", &programState->hdr);
        if(programState->hdr){
            ImGui::Checkbox("Bloom", &programState->bloom);
            ImGui::SliderFloat("HDR Exposure", &programState->hdrExposure, 0.0f, 5.0f);
            ImGui::SliderFloat("HDR Gamma", &programState->hdrGamma, 0.0f, 5.0f);
        }
        ImGui::Text("Effects");
        ImGui::Checkbox("Draw Wireframe", &programState->wireframe);
        ImGui::RadioButton("No Effect", &programState->effectSelected, 0);
        ImGui::RadioButton("Grayscale", &programState->effectSelected, 1);
        ImGui::RadioButton("Inversion", &programState->effectSelected, 2);
        ImGui::RadioButton("Sharpen", &programState->effectSelected, 3);
        ImGui::RadioButton("Blur", &programState->effectSelected, 4);
        ImGui::RadioButton("Edge detect", &programState->effectSelected, 5);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        programState->blinnLighting = !programState->blinnLighting;
        std::cout << "Blinn - " << (programState->blinnLighting ? "ON" : "OFF") << '\n';
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS){
        programState->hdr = !programState->hdr;
        std::cout << "HDR - " << (programState->hdr  ? "ON" : "OFF") << '\n';
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){
        programState->bloom = !programState->bloom;
        std::cout << "Bloom - " << (programState->bloom  ? "ON" : "OFF") << '\n';
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
        std::cout << "Camera lock - " << (programState->CameraMouseMovementUpdateEnabled ? "Disabled" : "Enabled");
    }
}

unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            // Load RGBA instead of RGB for .png cubemap textures
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void setLights(Shader lightingShader, glm::vec3 pointLightPositions[]) {
    //directional light
    lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
    // point light 1
    lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
    lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[0].diffuse", 0.94f, 0.98f, 0.78f);
    lightingShader.setVec3("pointLights[0].specular", 0.94f, 0.98f, 0.78f);
    lightingShader.setFloat("pointLights[0].constant", programState->pointLight.constant);
    lightingShader.setFloat("pointLights[0].linear", programState->pointLight.linear);
    lightingShader.setFloat("pointLights[0].quadratic", programState->pointLight.quadratic);
    // point light 2
    lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
    lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[1].diffuse", 0.94f, 0.98f, 0.78f);
    lightingShader.setVec3("pointLights[1].specular", 0.94f, 0.98f, 0.78f);
    lightingShader.setFloat("pointLights[1].constant", programState->pointLight.constant);
    lightingShader.setFloat("pointLights[1].linear", programState->pointLight.linear);
    lightingShader.setFloat("pointLights[1].quadratic", programState->pointLight.quadratic);
    // point light 3
    lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
    lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[2].diffuse", 0.94f, 0.98f, 0.78f);
    lightingShader.setVec3("pointLights[2].specular", 0.94f, 0.98f, 0.78f);
    lightingShader.setFloat("pointLights[2].constant", programState->pointLight.constant);
    lightingShader.setFloat("pointLights[2].linear", programState->pointLight.linear);
    lightingShader.setFloat("pointLights[2].quadratic", programState->pointLight.quadratic);
    // point light 4
    lightingShader.setVec3("pointLights[3].position", pointLightPositions[3]);
    lightingShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[3].diffuse", 0.94f, 0.98f, 0.78f);
    lightingShader.setVec3("pointLights[3].specular", 0.94f, 0.98f, 0.78f);
    lightingShader.setFloat("pointLights[3].constant", programState->pointLight.constant);
    lightingShader.setFloat("pointLights[3].linear", programState->pointLight.linear);
    lightingShader.setFloat("pointLights[3].quadratic", programState->pointLight.quadratic);
    // point light 5
    lightingShader.setVec3("pointLights[4].position", pointLightPositions[4]);
    lightingShader.setVec3("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[4].diffuse", 0.94f, 0.98f, 0.78f);
    lightingShader.setVec3("pointLights[4].specular", 0.94f, 0.98f, 0.78f);
    lightingShader.setFloat("pointLights[4].constant", programState->pointLight.constant);
    lightingShader.setFloat("pointLights[4].linear", programState->pointLight.linear);
    lightingShader.setFloat("pointLights[4].quadratic", programState->pointLight.quadratic);
}