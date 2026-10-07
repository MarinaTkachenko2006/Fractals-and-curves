#ifndef __TASKS_IMD_MAR_VIK__
#define __TASKS_IMD_MAR_VIK__

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>


// Класс единого контекста, который получает любая задача
class AppContext {
public:
    SDL_Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
};

// Интерфейс класса задачи
class TaskInterface {
public:
    virtual ~TaskInterface() = default;
    virtual void prepare(const AppContext& ctx) = 0;
};

// Класс задачи 1
class Task1 : public TaskInterface {
public:

    ~Task1() noexcept override;
    void prepare(const AppContext& ctx) override;

};

class Task2 : public TaskInterface {
public:
    ~Task2() noexcept override;
    void prepare(const AppContext& ctx) override;
};

class Task3 : public TaskInterface {
public:
    ~Task3() noexcept override;
    void freeTextures() noexcept;
    void prepare(const AppContext& ctx) override;
};


#endif // !__TASKS_IMD_MAR_VIK__
