#ifndef __L_SYSTEM_TKA_IGN_GOR__
#define __L_SYSTEM_TKA_IGN_GOR__

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <imgui.h>

struct l_system_segment {
	ImVec2 start, end; // Начало и конец отрезка в мировых координатах
	ImU32 color; // Цвет отрезка
	float thickness; // Толщина отрезка
	int depth; // Глубина стека в момент рисования (0 — ствол, больше — ветки)
};

struct turtle_state {
	ImVec2 position; // Позиция черепахи в мировых координатах
	float angle; // Текущий угол в градусах
	float thickness; // Текущая толщина пера
	ImU32 color; // Текущий цвет пера
};

class l_system {
public:
	std::string axiom; // Стартовая строка (аксиома)
	float angle_step = 25; // Угол поворота для + и -, в градусах
	float initial_angle = 90; // Начальное направление черепахи, в градусах
	std::string current_state; // Текущая строка после последнего expand

	std::unordered_map<char, std::string> rules; // Правила подстановки: символ -> строка

	float base_thickness = 4.0f; // Толщина у основания (ствола)
	float tip_thickness = 0.6f; // Толщина у кончика (веток)
	ImU32 base_color = IM_COL32(40, 40, 180, 255); // Цвет основания (тёмно-синий)
	ImU32 tip_color = IM_COL32(220, 60, 60, 255); // Цвет кончиков (красный)
	float random_angle_deg = 0.0f; // Амплитуда случайного отклонения угла
	float leaf_length_factor = 0.8f; // Во сколько раз лист короче родителя

	std::vector<l_system_segment> interpret() const; // Превращает current_state в список отрезков

	bool load_from_file(const char* path); // Читает файл и вызывает load_from_string
	bool load_from_string(const std::string& text); // Парсит строку формата "<аксиома> <угол> <направление>" и правила

	void expand(int iterations); // Применяет правила указанное число раз

	static ImU32 lerp_color(ImU32 a, ImU32 b, float t); // Линейная интерполяция между двумя цветами

	static constexpr size_t MAX_STRING_LENGTH = 2'000'000; // Защита от экспоненциального роста строки
private:
	static float random() noexcept; // Генерирует случайное вещественное число в диапазоне [0, 1]
	void step(turtle_state& t, bool draw, std::vector<l_system_segment>& segments, int depth) const; // Шаг черепахи вперёд
};

#endif // !__L_SYSTEM_TKA_IGN_GOR__