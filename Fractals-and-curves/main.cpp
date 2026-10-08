// Сторонние библиотеки
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

// Стандартные библиотеки
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include <array>
#include <algorithm>
#include <iostream>

// Наши библиотеки
#include "tasks.h"

static bool showWindow2 (false);   // Флаг открытости окна заданий

enum class ActiveTask { None, Task1, Task2, Task3 }; // Перечисление возможных активных заданий

SDL_Window* window = nullptr; // Указатель на окно
SDL_Renderer* renderer = nullptr; // Указатель на рендерер для отрисовки
SDL_Event eventer; // Контролёр событий

struct MainImageCallback // Callback, вызываемый ImGui внутри RenderDrawData
{
    SDL_Renderer* renderer = nullptr;
    ImVec2 origin;
    ImVec2 area;
};


int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { // вызов SDL-подсистемы
        std::cerr << "SDL_Init failed" << std::endl;
        return -1;
    }
    MainImageCallback mainCb;

    window = SDL_CreateWindow("ImGui Window", 1000, 700, SDL_WINDOW_RESIZABLE); // Создание окна
    if (!window) { std::cerr << "SDL_CreateWindow failed" << std::endl; return 1; }

    renderer = SDL_CreateRenderer(window, nullptr); // Создание рендерера
    if (!renderer) { std::cerr << "SDL_CreateRenderer failed" << std::endl; return 1; }

    IMGUI_CHECKVERSION(); // Gроверки версий файлов .h библиотеки ImGUI
    ImGui::CreateContext(); // Создание глобального контекста для отрисовки
    ImGui::StyleColorsDark(); // Установка тёмного стиля

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer); // Dызов слоя, отвечающего за отрисовку в ImGUI

    ImGuiIO& io = ImGui::GetIO(); // Получение глобального объекта ввода-вывода (для контроля текущего состояния программы)

    TaskInterface* currentTask = nullptr; // Текущая активная задача
    ActiveTask activeTask = ActiveTask::None; // Тип текущей активной задачи

    bool running (true); // Флаг работы программы

    while (running) { // Пока не закрыли программу
        while (SDL_PollEvent(&eventer)) { // Контроль событий
            ImGui_ImplSDL3_ProcessEvent(&eventer);
            if (eventer.type == SDL_EVENT_QUIT) running = false;
        }


        ImGui_ImplSDLRenderer3_NewFrame(); // Подготовка внутренних буферов рендера ImGui
        ImGui_ImplSDL3_NewFrame(); // Передача накопленных событий ввода и актуализация состояния мыши/клавиатуры
        ImGui::NewFrame(); // Создание нового кадра, в течение которого формируется список виджетов

        // Отрисовка главного окна
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // 10 пикселей от левого верхнего угла окна
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 20, io.DisplaySize.y - 20), ImGuiCond_Always);
        ImGui::Begin("Fractals and curves", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        ImVec2 contentAvail = ImGui::GetContentRegionAvail(); // Сколько места сейчас доступно внутри главного окна — ширина и высота в пикселях

       
        ImGui::BeginChild("##button_panel", contentAvail, true);

        // Формирование контекста задачи
        AppContext ctx;
        ctx.renderer = renderer;
        ctx.window = window;

        if (ImGui::Button("Task 1")) { // Кнопка Task 1 нажата
            delete currentTask;
            activeTask = ActiveTask::Task1;
            showWindow2 = true;
            if (currentTask) currentTask->prepare(ctx);
        }
        if (ImGui::Button("Task 2")) { // Кнопка Task 2 нажата
            delete currentTask;
            activeTask = ActiveTask::Task2;
            showWindow2 = true;
            if (currentTask) currentTask->prepare(ctx);
        }
        if (ImGui::Button("Task 3")) { // Кнопка Task 3 нажата
            delete currentTask;
            currentTask = new Task3();
            activeTask = ActiveTask::Task3;
            showWindow2 = true;
            if (currentTask) currentTask->prepare(ctx);
        }

        ImGui::EndChild();
        ImGui::End();

        // Установка окна задачи
        if (showWindow2) {
            const char* title =
                activeTask == ActiveTask::Task1 ? "Task 1" :
                activeTask == ActiveTask::Task2 ? "Task 2" :
                activeTask == ActiveTask::Task3 ? "Task 3" : "None";

            ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(700, 800), ImGuiCond_FirstUseEver);
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