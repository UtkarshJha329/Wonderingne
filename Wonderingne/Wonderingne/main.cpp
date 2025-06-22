
#include "VulkanHandlingFunctions.h"

#include "ft2build.h"
#include FT_FREETYPE_H

class HelloTriangleApplication {
public:
    void Run() {

        //InitFontLibrary();
        //LoadFont();

        InitWindow();
        InitVulkan(APPLICATION_NAME, *window, vertexShaderFilePath, fragmentShaderFilePath, allModelsFilePaths, allUIModelsFilePaths);
        MainLoop();
        Cleanup();
    }

private:

// Freetype Font Stuff
#pragma region Freetype Font Stuff

private:
    FT_Library freeTypelibrary;
    FT_Face fontFace;

private:
    void InitFontLibrary() {
        FT_Error result = FT_Init_FreeType(&freeTypelibrary);

        if (result != FT_Err_Ok) {
            throw std::runtime_error("ERROR::FREETYTPE: Failed to initialize Freetype font library. Error Code := " + std::to_string(result));
        }
    }

    void LoadFont() {

        FT_Error result = FT_New_Face(freeTypelibrary, "Assets/Fonts/cour.ttf", 0, &fontFace);
        if (result != FT_Err_Ok) {
            if (result == FT_Err_Unknown_File_Format) {
                throw std::runtime_error("ERROR::FREETYTPE: Failed to initialize font face becuase of unknown file format error. Error Code := " + std::to_string(result));
            }
            else {
                throw std::runtime_error("ERROR::FREETYTPE: Failed to initialize font face. Error Code := " + std::to_string(result));
            }
        }

        //std::cout << "Number of faces in font file := " << fontFace->num_faces << std::endl;

        FT_Set_Pixel_Sizes(fontFace, 0, 48);
    }


#pragma endregion

// Mesh stuff
private:

    std::string vertexShaderFilePath = "Assets/Shaders/CompiledShaders/vert.spv";
    std::string fragmentShaderFilePath = "Assets/Shaders/CompiledShaders/frag.spv";


    const std::string texturedTruckOBJModelFilePath = "Assets/Models/Truck/Truck.obj";
    const std::string texturedUtahTeapotOBJModelFilePath = "Assets/Models/UtahTeapot/UtahTeapot.obj";
    const std::string texturedSuzanneOBJModelFilePath = "Assets/Models/TexturedSuzanne/TexturedSuzanne.obj";
    const std::string texturedLowPolyForestTerrainOBJModelFilePath = "Assets/Models/LowPolyForestTerrain/LowPolyForestTerrain.obj";
    const std::string texturedPlaneOBJModelFilePath = "Assets/Models/TexturedPlane/TexturedPlane.obj";

    std::vector<std::string> allModelsFilePaths = { texturedSuzanneOBJModelFilePath, texturedTruckOBJModelFilePath, texturedLowPolyForestTerrainOBJModelFilePath, texturedUtahTeapotOBJModelFilePath };
    //std::vector<std::string> allModelsFilePaths = { texturedSuzanneOBJModelFilePath, texturedTruckOBJModelFilePath, texturedLowPolyForestTerrainOBJModelFilePath };
    //std::vector<std::string> allModelsFilePaths = { texturedPlaneOBJModelFilePath };
    //std::vector<std::string> allModelsFilePaths = {};
    //std::vector<std::string> allModelsFilePaths = { texturedUtahTeapotOBJModelFilePath };
    //std::vector<Model> allModelsThatNeedToBeLoadedAndRendered = {};


    std::vector<std::string> allUIModelsFilePaths = { texturedPlaneOBJModelFilePath };
    //std::vector<Model> allUIModelsThatNeedToBeLoadedAndRendered = {};

// APPLICATION PRIVATE FUNCTIONS
private:

    void MainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            DrawFrame(*window);
        }
    }

    void Cleanup() {
        VulkanCleanup();
        GlfwCleanup();
    }

    // GLFW PRIVATE FUNCTIONS
private:

    void InitWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE.c_str(), nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
    }

    //static void FramebufferResizeCallback() {
    //    framebufferResized = true;
    //}

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
        //auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        //app->framebufferResized = true;
        framebufferResized = true;
    }

    void GlfwCleanup() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

// APPLICATION PUBLIC VARIABLES
public:
    inline static const std::string APPLICATION_NAME = "Hello Triangle Application";

// APPLICATION PRIVATE VARIABLES
private:
    GLFWwindow* window;

};

int main() {
    HelloTriangleApplication app;

    try {
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}