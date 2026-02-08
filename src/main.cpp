#include <GL/glew.h>
#include <SDL2/SDL.h>
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

// ===========================================================================
// Custom retro dark-blue ImGui theme
// ===========================================================================
static void setupRetroTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding
    style.WindowRounding    = 2.0f;
    style.FrameRounding     = 2.0f;
    style.GrabRounding      = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding       = 2.0f;
    style.ChildRounding     = 1.0f;

    // Borders
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;

    // Spacing
    style.ItemSpacing       = ImVec2(8, 4);
    style.ItemInnerSpacing  = ImVec2(4, 4);
    style.WindowPadding     = ImVec2(8, 8);

    // Custom colour palette -- dark blue CRT aesthetic
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]          = ImVec4(0.086f, 0.086f, 0.118f, 1.00f);
    c[ImGuiCol_ChildBg]           = ImVec4(0.071f, 0.071f, 0.102f, 1.00f);
    c[ImGuiCol_PopupBg]           = ImVec4(0.071f, 0.071f, 0.102f, 0.95f);
    c[ImGuiCol_Border]            = ImVec4(0.165f, 0.227f, 0.361f, 1.00f);
    c[ImGuiCol_BorderShadow]      = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);

    c[ImGuiCol_FrameBg]           = ImVec4(0.059f, 0.086f, 0.161f, 1.00f);
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.122f, 0.204f, 0.375f, 1.00f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.149f, 0.255f, 0.502f, 1.00f);

    c[ImGuiCol_TitleBg]           = ImVec4(0.071f, 0.071f, 0.102f, 1.00f);
    c[ImGuiCol_TitleBgActive]     = ImVec4(0.086f, 0.129f, 0.243f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.040f, 0.040f, 0.080f, 0.70f);

    c[ImGuiCol_MenuBarBg]         = ImVec4(0.071f, 0.071f, 0.102f, 1.00f);
    c[ImGuiCol_ScrollbarBg]       = ImVec4(0.039f, 0.039f, 0.102f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]     = ImVec4(0.165f, 0.227f, 0.361f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.200f, 0.290f, 0.440f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.250f, 0.350f, 0.530f, 1.00f);

    c[ImGuiCol_CheckMark]         = ImVec4(0.000f, 0.824f, 1.000f, 1.00f);
    c[ImGuiCol_SliderGrab]        = ImVec4(0.000f, 0.824f, 1.000f, 0.80f);
    c[ImGuiCol_SliderGrabActive]  = ImVec4(0.000f, 0.900f, 1.000f, 1.00f);

    c[ImGuiCol_Button]            = ImVec4(0.086f, 0.129f, 0.243f, 1.00f);
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.122f, 0.204f, 0.375f, 1.00f);
    c[ImGuiCol_ButtonActive]      = ImVec4(0.149f, 0.255f, 0.502f, 1.00f);

    c[ImGuiCol_Header]            = ImVec4(0.086f, 0.129f, 0.243f, 1.00f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.122f, 0.204f, 0.375f, 1.00f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.149f, 0.255f, 0.502f, 1.00f);

    c[ImGuiCol_Separator]         = ImVec4(0.165f, 0.227f, 0.361f, 1.00f);
    c[ImGuiCol_SeparatorHovered]  = ImVec4(0.200f, 0.290f, 0.440f, 1.00f);
    c[ImGuiCol_SeparatorActive]   = ImVec4(0.000f, 0.824f, 1.000f, 1.00f);

    c[ImGuiCol_ResizeGrip]        = ImVec4(0.165f, 0.227f, 0.361f, 0.40f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.200f, 0.290f, 0.440f, 0.70f);
    c[ImGuiCol_ResizeGripActive]  = ImVec4(0.000f, 0.824f, 1.000f, 0.90f);

    c[ImGuiCol_Tab]               = ImVec4(0.071f, 0.071f, 0.102f, 1.00f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.122f, 0.204f, 0.375f, 1.00f);
    c[ImGuiCol_TabActive]         = ImVec4(0.086f, 0.129f, 0.243f, 1.00f);
    c[ImGuiCol_TabUnfocused]      = ImVec4(0.050f, 0.050f, 0.080f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]= ImVec4(0.071f, 0.100f, 0.180f, 1.00f);

    c[ImGuiCol_Text]              = ImVec4(0.784f, 0.839f, 0.898f, 1.00f);
    c[ImGuiCol_TextDisabled]      = ImVec4(0.420f, 0.498f, 0.627f, 1.00f);
    c[ImGuiCol_TextSelectedBg]    = ImVec4(0.149f, 0.255f, 0.502f, 0.60f);

    c[ImGuiCol_PlotLines]         = ImVec4(0.000f, 0.824f, 1.000f, 0.80f);
    c[ImGuiCol_PlotHistogram]     = ImVec4(0.000f, 0.600f, 0.800f, 0.80f);

    c[ImGuiCol_NavHighlight]      = ImVec4(0.000f, 0.824f, 1.000f, 0.70f);
    c[ImGuiCol_ModalWindowDimBg]  = ImVec4(0.000f, 0.000f, 0.000f, 0.60f);
}

// ===========================================================================
// main() -- application entry point
// ===========================================================================
int main(int /*argc*/, char* /*argv*/[]) {
    srand((unsigned)time(nullptr));

    // -----------------------------------------------------------------
    // Initialize SDL2
    // -----------------------------------------------------------------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Request OpenGL 3.3 core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // -----------------------------------------------------------------
    // Create window
    // -----------------------------------------------------------------
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

    // -----------------------------------------------------------------
    // Initialize GLEW
    // -----------------------------------------------------------------
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW Error: " << glewGetErrorString(glewErr) << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Clear any spurious GL error from glewInit
    while (glGetError() != GL_NO_ERROR) {}

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // -----------------------------------------------------------------
    // Initialize Dear ImGui
    // -----------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    (void)io; // ImGui docking not available in this build

    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Apply custom retro dark-blue theme
    setupRetroTheme();

    // -----------------------------------------------------------------
    // Initialize editor
    // -----------------------------------------------------------------
    Editor editor;
    if (!editor.init(window)) {
        std::cerr << "Failed to initialise editor" << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // -----------------------------------------------------------------
    // Game runtime (created on demand when F5 is pressed)
    // -----------------------------------------------------------------
    GameRuntime runtime;
    bool runtimeActive = false;

    // -----------------------------------------------------------------
    // Main loop
    // -----------------------------------------------------------------
    bool running  = true;
    Uint64 lastTime = SDL_GetPerformanceCounter();
    Uint64 freq     = SDL_GetPerformanceFrequency();

    while (running) {
        // Calculate delta time
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - lastTime) / (float)freq;
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f; // cap to prevent huge steps

        // -------------------------------------------------------------
        // Process SDL events
        // -------------------------------------------------------------
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Always handle quit
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (runtimeActive) {
                // ---- Runtime mode ----
                // ImGui still needs to process events for the overlay UI
                ImGui_ImplSDL2_ProcessEvent(&event);

                // ESC or F5 stops the runtime
                if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                    SDL_Keycode sym = event.key.keysym.sym;
                    if (sym == SDLK_ESCAPE || sym == SDLK_F5) {
                        runtime.shutdown();
                        runtimeActive = false;
                        editor.isRunning = false;
                        continue; // don't pass ESC/F5 to the runtime
                    }
                }

                // Forward all other events to the game runtime
                runtime.handleEvent(event);

            } else {
                // ---- Editor mode ----
                ImGui_ImplSDL2_ProcessEvent(&event);

                // F5 starts the runtime
                if (event.type == SDL_KEYDOWN && !event.key.repeat &&
                    event.key.keysym.sym == SDLK_F5) {
                    runtimeActive = true;
                    editor.isRunning = true;
                    runtime.init(editor.project, editor.renderer);
                }

                // Keyboard shortcuts (only when ImGui doesn't want the keyboard)
                if (event.type == SDL_KEYDOWN && !event.key.repeat &&
                    !io.WantCaptureKeyboard) {
                    SDL_Keycode sym = event.key.keysym.sym;
                    Uint16 mod      = event.key.keysym.mod;

                    if ((mod & KMOD_CTRL) && sym == SDLK_s) {
                        if (editor.project.save("project.json")) {
                            editor.showStatus("Saved project.json");
                        } else {
                            editor.showStatus("Save failed!");
                        }
                    }
                    if ((mod & KMOD_CTRL) && sym == SDLK_n) {
                        editor.project.clear();
                        editor.project.createDefaultScene();
                        editor.clearMeshCache();
                        editor.selectedId = editor.project.objects.empty()
                            ? "" : editor.project.objects[0].id;
                        editor.showStatus("New project created");
                    }
                    if ((mod & KMOD_CTRL) && sym == SDLK_o) {
                        if (editor.project.load("project.json")) {
                            editor.clearMeshCache();
                            editor.selectedId = editor.project.objects.empty()
                                ? "" : editor.project.objects[0].id;
                            editor.showStatus("Loaded project.json");
                        } else {
                            editor.showStatus("Load failed!");
                        }
                    }
                    if ((mod & KMOD_CTRL) && sym == SDLK_d) {
                        editor.duplicateObject();
                    }
                    if (sym == SDLK_DELETE) {
                        if (editor.getSelected()) {
                            editor.deleteObject(editor.selectedId);
                        }
                    }
                }
            }
        }

        // -------------------------------------------------------------
        // Begin ImGui frame
        // -------------------------------------------------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        int windowW, windowH;
        SDL_GetWindowSize(window, &windowW, &windowH);

        // -------------------------------------------------------------
        // Update and render
        // -------------------------------------------------------------
        if (runtimeActive) {
            // --- Game runtime mode ---
            runtime.update(dt);
            runtime.render(windowW, windowH);

            // Overlay: game status and stop button
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowBgAlpha(0.70f);
            ImGui::Begin("##RuntimeOverlay", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

            ImGui::TextColored(ImVec4(0.1f, 1.0f, 0.3f, 1.0f), "GAME RUNNING");
            ImGui::SameLine();
            ImGui::TextDisabled("  ESC / F5 to stop");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 0.9f));
            if (ImGui::SmallButton("STOP")) {
                runtime.shutdown();
                runtimeActive = false;
                editor.isRunning = false;
            }
            ImGui::PopStyleColor(2);
            ImGui::End();

            // Check if the game ended itself (via end_game action)
            if (runtime.isGameOver()) {
                runtime.shutdown();
                runtimeActive = false;
                editor.isRunning = false;
            }

        } else {
            // --- Editor mode ---
            editor.update(dt);
            editor.render(windowW, windowH);
        }

        // -------------------------------------------------------------
        // Render ImGui draw data
        // -------------------------------------------------------------
        ImGui::Render();
        glViewport(0, 0, windowW, windowH);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // -----------------------------------------------------------------
    // Clean shutdown
    // -----------------------------------------------------------------
    if (runtimeActive) {
        runtime.shutdown();
    }
    editor.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
