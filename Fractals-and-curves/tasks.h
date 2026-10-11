#ifndef __TASKS_IMD_MAR_VIK__
#define __TASKS_IMD_MAR_VIK__

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

#include "l_system.h"
#include "dialog_windows.h"

class AppContext {
public:
    SDL_Renderer* renderer = nullptr; // Указатель на отрисовщика
    SDL_Window* window = nullptr; // Указатель на окно
};

class TaskInterface {
public:
    virtual ~TaskInterface() = default;
    virtual void prepare(const AppContext& ctx) = 0;
    virtual void draw(const AppContext& ctx) = 0;
};

class Task1 : public TaskInterface {
public:
	l_system system;

	int iteration_number = 3; // Текущее число итераций
	int last_iterations = -1; // Число итераций на момент последней сборки

	float offset_x = 0; // Сдвиг вида по оси X
	float offset_y = 0; // Сдвиг вида по оси Y
	float zoom = 1; // Масштаб вида

	std::vector<l_system_segment> segments; // Построенные отрезки
	float bb_min_x = 0.0f, bb_min_y = 0.0f, bb_max_x = 0.0f, bb_max_y = 0.0f; // Границы фрактала
	bool dirty = true; // Флаг, указывающий нужна ли пересборка

	int color_mode = 0; // 0 — градиент, 1 — дерево

	float tree_base_thickness = 6.0f; // Толщина ствола
	ImU32 tree_base_color = IM_COL32(101, 67, 33, 255); // Коричневый цвет
	ImU32 tree_tip_color = IM_COL32(60, 160, 60, 255); // Зелёный цвет
	float tree_random_angle = 15.0f; // Случайное отклонение угла ветвей

	float random_angle = 0; // Общая случайность угла в градусах: delta = angle_step + (random() * 2 - 1) * random_angle

	bool draw_only_visible = true; // Рисовать только видимые отрезки
	size_t drawn_last_frame = 0; // Сколько отрезков реально отрисовано в прошлом кадре

	char status_msg[512] = ""; // Сообщение в панели управления

	FileDialogState file_dialog; //состояние системного диалога выбора файла

private:
	void rebuild(); // Пересобрать отрезки по текущей l-системе
	void compute_barier(); // Вычислить границы фрактала
	void apply_gradient_color(); // Раскраска по расстоянию от центра
	void apply_tree_color(); // Раскраска по глубине стека
	void draw_canvas(float w, float h); // Нарисовать холст с фракталом
	void draw_controls(); // Нарисовать панель управления

public:
	void load_file(const char* path); // Загрузить l-систему из файла
	void reset_to_defaults(); // Сброс всех параметров к значениям по умолчанию
public:
	~Task1() noexcept override;
	void prepare(const AppContext& ctx) override;
	void draw(const AppContext& ctx) override;

	void open_file_dialog(SDL_Window* window); // Открыть системный диалог выбора файла
	void poll_file_dialog(); // Опросить результат диалога и загрузить файл
};

class Task2 : public TaskInterface {
public:
    ~Task2() noexcept override;
    void prepare(const AppContext& ctx) override;
    void draw(const AppContext& ctx) override;
};

class Task3 : public TaskInterface {
    int state = 0;
    int dragging_index = -1;

    const int ADD_POINT = 0;
    const int MOVE_POINT = 1;
    const int DELETE_POINT = 2;

    std::vector<ImVec2> points;
    std::vector<ImVec2> curve;
    std::vector<ImVec2> prepared_points;
    std::vector<bool>   prepared_is_added;

    int  find_point_at(const ImVec2& local_pos) const;
    void rebuild_curve();

    void draw_canvas(float canvas_w, float canvas_h);
    void create_curve_segment(int start_point);
    ImVec2 evaluate_Bezier_curve(ImVec2& point1, ImVec2& point2,
        ImVec2& point3, ImVec2& point4, double t);

    bool is_collinear(const ImVec2& a, const ImVec2& b, const ImVec2& c);

    void prepare_points_for_curve();

public:
    ~Task3() noexcept override;
    void prepare(const AppContext& ctx) override;
    void draw(const AppContext& ctx) override;
};

#endif // !__TASKS_IMD_MAR_VIK__