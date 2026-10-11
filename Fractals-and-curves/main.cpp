#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <array>
#include <algorithm>
#include <iostream>

#include "tasks.h"

static bool showWindow2(false);

enum class ActiveTask { None, Task1, Task2, Task3 };

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Event eventer;

struct MainImageCallback
{
    SDL_Renderer* renderer = nullptr;
    ImVec2 origin;
    ImVec2 area;
};


int main(int argc, char* argv[])
{
    std::srand((unsigned)std::time(nullptr));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed" << std::endl;
        return -1;
    }
    MainImageCallback mainCb;

    window = SDL_CreateWindow("ImGui Window", 1000, 700, SDL_WINDOW_RESIZABLE);
    if (!window) { std::cerr << "SDL_CreateWindow failed" << std::endl; return 1; }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) { std::cerr << "SDL_CreateRenderer failed" << std::endl; return 1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    ImGuiIO& io = ImGui::GetIO();

    TaskInterface* currentTask = nullptr;
    ActiveTask activeTask = ActiveTask::None;

    bool running(true);

    while (running) {
        while (SDL_PollEvent(&eventer)) {
            ImGui_ImplSDL3_ProcessEvent(&eventer);
            if (eventer.type == SDL_EVENT_QUIT) running = false;

            if (eventer.type == SDL_EVENT_DROP_FILE) {
                const char* dropped = eventer.drop.data;
                if (dropped && currentTask && activeTask == ActiveTask::Task1) {
                    static_cast<Task1*>(currentTask)->load_file(dropped);
                }
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 20, io.DisplaySize.y - 20), ImGuiCond_Always);
        ImGui::Begin("Fractals and curves", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        ImVec2 contentAvail = ImGui::GetContentRegionAvail();

        ImGui::BeginChild("##button_panel", contentAvail, true);

        AppContext ctx;
        ctx.renderer = renderer;
        ctx.window = window;

        if (ImGui::Button("Task 1")) {
            delete currentTask;
            currentTask = new Task1();
            activeTask = ActiveTask::Task1;
            showWindow2 = true;
            if (currentTask) currentTask->prepare(ctx);
        }
        if (ImGui::Button("Task 2")) {
            delete currentTask;
            currentTask = new Task2();
            activeTask = ActiveTask::Task2;
            showWindow2 = true;
            if (currentTask) currentTask->prepare(ctx);
        }
        if (ImGui::Button("Task 3")) {
            delete currentTask;
            currentTask = new Task3();
            activeTask = ActiveTask::Task3;
            showWindow2 = true;
            if (currentTask) currentTask->prepare(ctx);
        }

        ImGui::EndChild();
        ImGui::End();

        if (showWindow2 && currentTask) {
            const char* title =
                activeTask == ActiveTask::Task1 ? "Task 1" :
                activeTask == ActiveTask::Task2 ? "Task 2" :
                activeTask == ActiveTask::Task3 ? "Task 3" : "None";

            ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
            ImGui::Begin(title, &showWindow2, ImGuiWindowFlags_HorizontalScrollbar);

            currentTask->draw(ctx);

            ImGui::End();
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    delete currentTask;

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}