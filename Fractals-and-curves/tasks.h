#ifndef __TASKS_IMD_MAR_VIK__
#define __TASKS_IMD_MAR_VIK__

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>
#include <vector>

class AppContext {
public:
    SDL_Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
};

class TaskInterface {
public:
    virtual ~TaskInterface() = default;
    virtual void prepare(const AppContext& ctx) = 0;
    virtual void draw(const AppContext& ctx) = 0;
};

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
    int state = 0;

    int dragging_index = -1;

    const int ADD_POINT = 0;
    const int MOVE_POINT = 1;
    const int DELETE_POINT = 2;

    std::vector<ImVec2> points;                 // исходные точки (пользовательские)
    std::vector<ImVec2> curve;                  // вычисленные точки кривой
    std::vector<ImVec2> prepared_points;        // точки для сегментов (включая добавленные)
    std::vector<bool>   prepared_is_added;      // true — добавленная (середина), false — исходная

    int  find_point_at(const ImVec2& local_pos) const;
    void rebuild_curve();

    void draw_canvas(float canvas_w, float canvas_h);
    void create_curve_segment(int start_point);
    ImVec2 evaluate_Bezier_curve(ImVec2& point1, ImVec2& point2,
        ImVec2& point3, ImVec2& point4, double t);

    void prepare_points_for_curve();

public:
    ~Task3() noexcept override;
    void prepare(const AppContext& ctx) override;
    void draw(const AppContext& ctx) override;
};

#endif // !__TASKS_IMD_MAR_VIK__