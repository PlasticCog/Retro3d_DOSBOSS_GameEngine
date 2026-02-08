#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "core/project.h"
#include "engine/renderer.h"
#include "engine/camera.h"
#include "engine/gizmo.h"
#include "editor/editor.h"
#include "editor/viewport.h"
#include "editor/properties.h"
#include "editor/event_editor.h"
#include "editor/timeline.h"
#include "runtime/runtime.h"

#include <iostream>
#include <cstdlib>
#include <ctime>

int main(int /*argc*/, char* /*argv*/[]) {
    srand((unsigned)time(nullptr));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // OpenGL 3.3 core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow(
        "DOSBOSS 3D - Retro 3D Studio",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // vsync

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW Error: " << glewGetErrorString(glewErr) << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io; // flags configured via style below

    // Custom dark retro theme
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.086f, 0.086f, 0.118f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.071f, 0.071f, 0.102f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.071f, 0.071f, 0.102f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.165f, 0.227f, 0.361f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.059f, 0.086f, 0.161f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.122f, 0.204f, 0.375f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.149f, 0.255f, 0.502f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.071f, 0.071f, 0.102f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.086f, 0.129f, 0.243f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.071f, 0.071f, 0.102f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.039f, 0.039f, 0.102f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.165f, 0.227f, 0.361f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.824f, 1.0f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.824f, 1.0f, 0.8f);
    colors[ImGuiCol_Button] = ImVec4(0.086f, 0.129f, 0.243f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.122f, 0.204f, 0.375f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.149f, 0.255f, 0.502f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.086f, 0.129f, 0.243f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.122f, 0.204f, 0.375f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.149f, 0.255f, 0.502f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.071f, 0.071f, 0.102f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.122f, 0.204f, 0.375f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.086f, 0.129f, 0.243f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.784f, 0.839f, 0.898f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.420f, 0.498f, 0.627f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.165f, 0.227f, 0.361f, 1.0f);

    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize editor
    Editor editor;
    if (!editor.init(window)) {
        std::cerr << "Failed to init editor" << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    editor.project.createDefaultScene();

    // Game runtime
    GameRuntime runtime;
    bool runtimeActive = false;

    // Main loop
    bool running = true;
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - lastTime) / (float)SDL_GetPerformanceFrequency();
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F5 && !event.key.repeat) {
                if (!runtimeActive) {
                    // Start game
                    runtimeActive = true;
                    runtime.init(editor.project, editor.renderer);
                    editor.isRunning = true;
                } else {
                    // Stop game
                    runtime.shutdown();
                    runtimeActive = false;
                    editor.isRunning = false;
                }
            }

            if (runtimeActive && !io.WantCaptureKeyboard && !io.WantCaptureMouse) {
                runtime.handleEvent(event);
            }
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        int windowW, windowH;
        SDL_GetWindowSize(window, &windowW, &windowH);

        if (runtimeActive) {
            // Game runtime mode
            runtime.update(dt);
            runtime.render(windowW, windowH);

            // Minimal overlay
            ImGui::SetNextWindowPos(ImVec2((float)windowW * 0.5f - 40, 10));
            ImGui::SetNextWindowSize(ImVec2(80, 0));
            ImGui::Begin("##StopBtn", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
            if (ImGui::Button("STOP", ImVec2(64, 24))) {
                runtime.shutdown();
                runtimeActive = false;
                editor.isRunning = false;
            }
            ImGui::PopStyleColor();
            ImGui::End();

            if (runtime.isGameOver()) {
                runtime.shutdown();
                runtimeActive = false;
                editor.isRunning = false;
            }
        } else {
            // Editor mode
            editor.update(dt);
            editor.render(windowW, windowH);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    if (runtimeActive) runtime.shutdown();
    editor.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
