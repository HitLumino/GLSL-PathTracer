/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this softwareand associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright noticeand this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _USE_MATH_DEFINES


#include <time.h>
#include <math.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

#include "Scene.h"
#include "TiledRenderer.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include "Loader.h"
#include "boyTestScene.h"
#include "ajaxTestScene.h"
#include "cornellTestScene.h"
#include "ImGuizmo.h"
#include "tinydir.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

using namespace std;
using namespace GLSLPT;

Scene* scene       = nullptr;
Renderer* renderer = nullptr;
RenderOptions	renderOptions;

std::vector<string> sceneFiles;

float mouseSensitivity = 0.01f;
bool keyPressed        = false;
int sampleSceneIndex   = 0;
int selectedInstance   = 0;
double lastTime        = glfwGetTime();
bool done = false;

// Decide GL+GLSL versions
// GL 3.2 + GLSL 150
const char* glsl_version = "#version 150";

std::string shadersDir = "../src/shaders/";
std::string assetsDir = "../assets/";

void GetSceneFiles()
{
    tinydir_dir dir;
    int i;
    tinydir_open_sorted(&dir, assetsDir.c_str());

    for (i = 0; i < dir.n_files; i++)
    {
        tinydir_file file;
        tinydir_readfile_n(&dir, &file, i);

        if (std::string(file.extension) == "scene")
        {
            sceneFiles.push_back(assetsDir + std::string(file.name));
        }
    }

    tinydir_close(&dir);
}

void LoadScene(std::string sceneName)
{
    delete scene;
    scene = new Scene();
    LoadSceneFromFile(sceneName, scene, renderOptions);
    //loadCornellTestScene(scene, renderOptions);
    selectedInstance = 0;
    scene->renderOptions = renderOptions;
}

bool InitRenderer()
{
    delete renderer;
    renderer = new TiledRenderer(scene, shadersDir);
    renderer->Init();
    return true;
}

// TODO: Fix occassional crashes when saving screenshot
void SaveFrame(const std::string filename)
{
    unsigned char* data = nullptr;
    int w, h;
    renderer->GetOutputBuffer(&data, w, h);
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filename.c_str(), w, h, 3, data, w*3);
    delete data;
}

void Render()
{
    auto io = ImGui::GetIO();
    renderer->Render();
    //const glm::ivec2 screenSize = renderer->GetScreenSize();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    renderer->Present();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Update(float secondsElapsed)
{
    keyPressed = false;

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) && ImGui::IsAnyMouseDown() && !ImGuizmo::IsOver() )
    {
        if (ImGui::IsMouseDown(0))
        {
            ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0, 0);
            scene->camera->OffsetOrientation(mouseDelta.x, mouseDelta.y);
            ImGui::ResetMouseDragDelta(0);
        }
        else if (ImGui::IsMouseDown(1))
        {
            ImVec2 mouseDelta = ImGui::GetMouseDragDelta(1, 0);
            scene->camera->SetRadius(mouseSensitivity * mouseDelta.y);
            ImGui::ResetMouseDragDelta(1);
        }
        else if (ImGui::IsMouseDown(2))
        {
            ImVec2 mouseDelta = ImGui::GetMouseDragDelta(2, 0);
            scene->camera->Strafe(mouseSensitivity * mouseDelta.x, mouseSensitivity * mouseDelta.y);
            ImGui::ResetMouseDragDelta(2);
        }
        scene->camera->isMoving = true;
    }

    renderer->Update(secondsElapsed);
}

void EditTransform(const float* view, const float* projection, float* matrix)
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

    if (ImGui::IsKeyPressed(90))
    {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }

    if (ImGui::IsKeyPressed(69))
    {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }

    if (ImGui::IsKeyPressed(82))
    {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    }

    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    }

    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation);
    ImGui::InputFloat3("Rt", matrixRotation);
    ImGui::InputFloat3("Sc", matrixScale);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        {
            mCurrentGizmoMode = ImGuizmo::LOCAL;
        }

        ImGui::SameLine();
        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        {
            mCurrentGizmoMode = ImGuizmo::WORLD;
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(view, projection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, NULL);
}

void MainLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::SetOrthographic(false);

        ImGuizmo::BeginFrame();
        {
            ImGui::Begin("Settings");

            ImGui::Text("Samples: %d ", renderer->GetSampleCount());

            ImGui::BulletText("LMB + drag to rotate");
            ImGui::BulletText("MMB + drag to pan");
            ImGui::BulletText("RMB + drag to zoom in/out");

            if (ImGui::Button("Save Screenshot")) {
                SaveFrame("./img_" + to_string(renderer->GetSampleCount()) + ".png");
            }

            std::vector<const char *> scenes;
            for (int i = 0; i < sceneFiles.size(); ++i) {
                scenes.push_back(sceneFiles[i].c_str());
            }

            if (ImGui::Combo("Scene", &sampleSceneIndex, scenes.data(), scenes.size())) {
                LoadScene(sceneFiles[sampleSceneIndex]);
                glfwRestoreWindow(window);
                glfwSetWindowSize(window, renderOptions.resolution.x, renderOptions.resolution.y);
                InitRenderer();
            }

            bool optionsChanged = false;

            optionsChanged |= ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 1.0f);

            if (ImGui::CollapsingHeader("Render Settings")) {
                bool requiresReload = false;
                Vec3 *bgCol = &renderOptions.bgColor;

                optionsChanged |= ImGui::SliderInt("Max Depth", &renderOptions.maxDepth, 1, 10);
                requiresReload |= ImGui::Checkbox("Enable EnvMap", &renderOptions.useEnvMap);
                optionsChanged |= ImGui::SliderFloat("HDR multiplier", &renderOptions.hdrMultiplier, 0.1f, 10.0f);
                requiresReload |= ImGui::Checkbox("Enable RR", &renderOptions.enableRR);
                requiresReload |= ImGui::SliderInt("RR Depth", &renderOptions.RRDepth, 1, 10);
                requiresReload |= ImGui::Checkbox("Enable Constant BG", &renderOptions.useConstantBg);
                optionsChanged |= ImGui::ColorEdit3("Background Color", (float *) bgCol, 0);
                ImGui::Checkbox("Enable Denoiser", &renderOptions.enableDenoiser);
                ImGui::SliderInt("Number of Frames to skip", &renderOptions.denoiserFrameCnt, 5, 50);

                if (requiresReload) {
                    scene->renderOptions = renderOptions;
                    InitRenderer();
                }

                scene->renderOptions.enableDenoiser = renderOptions.enableDenoiser;
                scene->renderOptions.denoiserFrameCnt = renderOptions.denoiserFrameCnt;
            }

            if (ImGui::CollapsingHeader("Camera")) {
                float fov = Math::Degrees(scene->camera->fov);
                float aperture = scene->camera->aperture * 1000.0f;
                optionsChanged |= ImGui::SliderFloat("Fov", &fov, 10, 90);
                scene->camera->SetFov(fov);
                optionsChanged |= ImGui::SliderFloat("Aperture", &aperture, 0.0f, 10.8f);
                scene->camera->aperture = aperture / 1000.0f;
                optionsChanged |= ImGui::SliderFloat("Focal Distance", &scene->camera->focalDist, 0.01f, 50.0f);
                ImGui::Text("Pos: %.2f, %.2f, %.2f", scene->camera->position.x, scene->camera->position.y,
                            scene->camera->position.z);
            }

            scene->camera->isMoving = false;

            if (optionsChanged) {
                scene->renderOptions = renderOptions;
                scene->camera->isMoving = true;
            }

            if (ImGui::CollapsingHeader("Objects")) {
                bool objectPropChanged = false;

                std::vector<std::string> listboxItems;
                for (int i = 0; i < scene->meshInstances.size(); i++) {
                    listboxItems.push_back(scene->meshInstances[i].name);
                }

                // Object Selection
                ImGui::ListBoxHeader("Instances");
                for (int i = 0; i < scene->meshInstances.size(); i++) {
                    bool is_selected = selectedInstance == i;
                    if (ImGui::Selectable(listboxItems[i].c_str(), is_selected)) {
                        selectedInstance = i;
                    }
                }
                ImGui::ListBoxFooter();

                ImGui::Separator();
                ImGui::Text("Materials");

                // Material Properties
                Vec3 *albedo = &scene->materials[scene->meshInstances[selectedInstance].materialID].albedo;
                Vec3 *emission = &scene->materials[scene->meshInstances[selectedInstance].materialID].emission;
                Vec3 *extinction = &scene->materials[scene->meshInstances[selectedInstance].materialID].extinction;

                objectPropChanged |= ImGui::ColorEdit3("Albedo", (float *) albedo, 0);
                objectPropChanged |= ImGui::SliderFloat("Metallic",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].metallic,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Roughness",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].roughness,
                                                        0.001f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Specular",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].specular,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("SpecularTint",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].specularTint,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Subsurface",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].subsurface,
                                                        0.0f, 1.0f);
                //objectPropChanged |= ImGui::SliderFloat("Anisotropic", &scene->materials[scene->meshInstances[selectedInstance].materialID].anisotropic, 0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Sheen",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].sheen,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("SheenTint",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].sheenTint,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Clearcoat",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].clearcoat,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("clearcoatGloss",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].clearcoatGloss,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Transmission",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].transmission,
                                                        0.0f, 1.0f);
                objectPropChanged |= ImGui::SliderFloat("Ior",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].ior,
                                                        1.001f, 2.0f);
                objectPropChanged |= ImGui::SliderFloat("atDistance",
                                                        &scene->materials[scene->meshInstances[selectedInstance].materialID].atDistance,
                                                        0.05f, 10.0f);
                objectPropChanged |= ImGui::ColorEdit3("Extinction", (float *) extinction, 0);

                // Transforms Properties
                ImGui::Separator();
                ImGui::Text("Transforms");
                {
                    float viewMatrix[16];
                    float projMatrix[16];

                    auto io = ImGui::GetIO();
                    scene->camera->ComputeViewProjectionMatrix(viewMatrix, projMatrix,
                                                               io.DisplaySize.x / io.DisplaySize.y);
                    Mat4 xform = scene->meshInstances[selectedInstance].transform;

                    EditTransform(viewMatrix, projMatrix, (float *) &xform);

                    if (memcmp(&xform, &scene->meshInstances[selectedInstance].transform, sizeof(float) * 16)) {
                        scene->meshInstances[selectedInstance].transform = xform;
                        objectPropChanged = true;
                    }
                }

                if (objectPropChanged) {
                    scene->RebuildInstances();
                }
            }
            ImGui::End();
        }

        double presentTime = glfwGetTime();

        Update((float) (presentTime - lastTime));
        lastTime = presentTime;
        glClearColor(0., 0., 0., 0.);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        Render();
        glfwSwapBuffers(window);
    }
}

    int main(int argc, char **argv) {
        srand((unsigned int) time(0));

        std::string sceneFile;

        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "-s" || arg == "--scene") {
                sceneFile = argv[++i];
            } else if (arg[0] == '-') {
                printf("Unknown option %s \n'", arg.c_str());
                exit(0);
            }
        }

        if (!sceneFile.empty()) {
            scene = new Scene();

            if (!LoadSceneFromFile(sceneFile, scene, renderOptions))
                exit(0);

            scene->renderOptions = renderOptions;
            std::cout << "Scene Loaded\n\n";
        } else {
            GetSceneFiles();
            LoadScene(sceneFiles[sampleSceneIndex]);
        }

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
        GLFWwindow *window = glfwCreateWindow(renderOptions.resolution.x, renderOptions.resolution.y, "PathTracer", NULL, NULL);
        if (window == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        // glad: load all OpenGL function pointers
        // ---------------------------------------
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1;
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
        if (!InitRenderer())
            return 1;

        while (!done) {
            MainLoop(window);
        }

        delete renderer;
        delete scene;

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    int fWidth  = 0;
    int fHeight = 0;
    glfwGetFramebufferSize(window, &fWidth, &fHeight);
}