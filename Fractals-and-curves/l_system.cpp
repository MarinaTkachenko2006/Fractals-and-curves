#include "l_system.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <numbers>

ImU32 l_system::lerp_color(ImU32 a, ImU32 b, float t) // Линейная интерполяция между цветами a, b
{
	if (t < 0) t = 0;
	if (t > 1) t = 1;

	// Каналы цвета a
	int ar((a >> IM_COL32_R_SHIFT) & 0xFF);
	int ag((a >> IM_COL32_G_SHIFT) & 0xFF);
	int ab((a >> IM_COL32_B_SHIFT) & 0xFF);
	int aa((a >> IM_COL32_A_SHIFT) & 0xFF);

	// Каналы цвета b
	int br((b >> IM_COL32_R_SHIFT) & 0xFF);
	int bg((b >> IM_COL32_G_SHIFT) & 0xFF);
	int bb((b >> IM_COL32_B_SHIFT) & 0xFF);
	int ba((b >> IM_COL32_A_SHIFT) & 0xFF);

	// Интерполирование
	int r(ar + (br - ar) * t);
	int g(ag + (bg - ag) * t);
	int bl(ab + (bb - ab) * t);
	int al(aa + (ba - aa) * t);

	return IM_COL32(r, g, bl, al);
}

float l_system::random() noexcept // Возвращает случайное вещественное число в диапазоне [0, 1]
{
	return (float)std::rand() / (float)RAND_MAX;
}

void l_system::step(turtle_state& t, bool draw, std::vector<l_system_segment>& segments,
	int depth) const // Шаг черепахи вперёд; depth — текущая глубина стека
{
	float len(1.0f); // Базовая длина
	for (int i = 0; i < depth; ++i) // Каждый уровень глубины уменьшает длину
		len *= this->leaf_length_factor;

	float radian = t.angle * std::numbers::pi / 180; // Перевод в радианы
	ImVec2 next(t.position.x + len * std::cos(radian),
		t.position.y - len * std::sin(radian));

	if (draw) {
		l_system_segment s;
		s.start = t.position;
		s.end = next;
		s.color = t.color;
		s.thickness = t.thickness;
		s.depth = depth; // Сохраняем глубину на момент рисования
		segments.push_back(s);
	}

	t.position = next;
}

bool l_system::load_from_string(const std::string& text) // Загрузка кода из строки
{
	// Сброс состояния L-системы
	this->rules.clear();
	this->axiom.clear();
	this->current_state.clear();

	std::string content(text); // Рабочая копия текста, чтобы не менять оригинал
	if (content.size() >= 3 &&
		(unsigned char)content[0] == 0xEF &&
		(unsigned char)content[1] == 0xBB &&
		(unsigned char)content[2] == 0xBF) { // Проверка BOM в начале файла
		content.erase(0, 3); // Удаляем BOM, иначе аксиома не распарсится
	}

	std::istringstream iss(content); // Поток для чтения построчно
	std::string line;
	bool flag(true); // Флаг, что первая непустая строка — аксиома

	while (std::getline(iss, line)) { // Чтение строки
		// Обрезание пробельных символов справа
		size_t last = line.find_last_not_of(" \t\r\n");
		if (last == std::string::npos) { line.clear(); continue; }
		line.erase(last + 1);

		// Обрезание пробельных символов слева
		size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos) continue;
		line.erase(0, first);

		if (line.empty()) continue;
		if (line[0] == '#') continue; // Пропускаем комментарии

		if (flag) {
			std::istringstream ls(line);
			ls >> this->axiom >> this->angle_step >> this->initial_angle; // Аксиома, угол, начальное направление
			flag = false;
		}
		else {
			size_t delimeter = line.find('='); // Ищем разделитель
			if (delimeter == std::string::npos || delimeter == 0) return false; // Неудача
			char from = line[0];
			std::string to = line.substr(delimeter + 1);
			this->rules[from] = to;
		}
	}

	if (this->axiom.empty()) return false; // Неудача
	this->current_state = this->axiom;
	return true; // Удача
}

bool l_system::load_from_file(const char* path) // Загрузка кода из файла
{
	std::ifstream f(path, std::ios::binary);
	if (!f) return false;
	std::stringstream ss;
	ss << f.rdbuf();
	return this->load_from_string(ss.str());
}

void l_system::expand(int iteration_number) // Применяет правила подстановки указанное число раз
{
	this->current_state = this->axiom; // Начинаем с аксиомы

	for (int it(0); it < iteration_number; ++it) { // Проходим все итерации
		std::string next; // Следующая строка
		next.reserve(this->current_state.size() * 2);

		for (char c : this->current_state) {
			auto found = this->rules.find(c); // Ищем правило для символа
			if (found == this->rules.end()) next.push_back(c); // Правила нет — символ остаётся как есть
			else next += found->second; // Правило есть — заменяем на его правую часть

			if (next.size() > l_system::MAX_STRING_LENGTH) { // Защита от неконтролируемого роста
				this->current_state = std::move(next);
				return;
			}
		}

		this->current_state = std::move(next); // Переходим к следующей итерации
	}
}

std::vector<l_system_segment> l_system::interpret() const // Интерпретация строки черепахой
{
	std::vector<l_system_segment> segments; // Список отрезков для отрисовки
	if (this->current_state.empty()) return segments;

	std::vector<turtle_state> st; // Стек состояний для квадратных скобок [, ]

	// Инициализация черепахи
	turtle_state t;
	t.position = ImVec2(0, 0);
	t.angle = this->initial_angle;
	t.thickness = this->base_thickness;
	t.color = this->base_color;

	const float max_depth_est(8); // Для нормировки глубины стека

	for (char c : this->current_state) { // Обход каждого символа строки
		if (std::isupper((unsigned char)c)) {
			step(t, true, segments, (int)st.size()); // Заглавные буквы — рисуют
			continue;
		}
		if (std::islower((unsigned char)c)) {
			step(t, false, segments, (int)st.size()); // Строчные буквы — не рисуют
			continue;
		}
		
		switch (c) {
		case '+': { // Поворот по часовой
			float delta(this->angle_step);
			if (this->random_angle_deg > 0)
				delta += (random() * 2 - 1) * this->random_angle_deg;
			t.angle += delta;
			break;
		}

		case '-': { // Поворот против часовой
			float delta(this->angle_step);
			if (this->random_angle_deg > 0)
				delta += (random() * 2 - 1) * this->random_angle_deg;
			t.angle -= delta;
			break;
		}

		case '[': { // Сохранение состояния
			st.push_back(t);

			float depth01((float)st.size() / max_depth_est);
			if (depth01 > 1.0f) depth01 = 1.0f;

			t.thickness = this->base_thickness +
				(this->tip_thickness - this->base_thickness) * depth01;
			t.color = l_system::lerp_color(this->base_color, this->tip_color, depth01);
			break;
		}

		case ']': {
			if (!st.empty()) {
				t = st.back();
				st.pop_back();
			}
			break;
		}

		case '@': { // Утоньшить, осветлить и приблизить к цвету кончиков
			t.thickness *= 0.75f;
			if (t.thickness < this->tip_thickness)
				t.thickness = this->tip_thickness;
			t.color = l_system::lerp_color(t.color, this->tip_color, 0.25f);
			break;
		}

		default:
			throw std::runtime_error("Wring sytaxis");
		}
	}

	return segments;
}