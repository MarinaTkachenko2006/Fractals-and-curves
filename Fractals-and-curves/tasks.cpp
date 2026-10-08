#include "tasks.h"
#include <cstdint>
#include <cmath>

Task1::~Task1() noexcept { }
void Task1::prepare(const AppContext& ctx) {}


Task2::~Task2() noexcept {}
void Task2::prepare(const AppContext& ctx) {}


int Task3::find_point_at(const ImVec2& local_pos) const
{
	const float HIT_RADIUS = 8.0f;  // радиус попадания в пикселях
	const float HIT_RADIUS_SQ = HIT_RADIUS * HIT_RADIUS;

	int best = -1;
	float best_dist_sq = HIT_RADIUS_SQ;

	for (size_t i = 0; i < points.size(); ++i) {
		float dx = points[i].x - local_pos.x;
		float dy = points[i].y - local_pos.y;
		float d_sq = dx * dx + dy * dy;
		if (d_sq <= best_dist_sq) {
			best_dist_sq = d_sq;
			best = static_cast<int>(i);
		}
	}
	return best;
}

void Task3::rebuild_curve()
{
	curve.clear();
	prepare_points_for_curve();

	if (prepared_points.size() >= 4) {
		for (size_t start = 0; start + 3 < prepared_points.size(); start += 3) {
			create_curve_segment(static_cast<int>(start));
		}
	}
}

void Task3::draw_canvas(float canvas_w, float canvas_h)
{
	ImGui::BeginChild("canvas", ImVec2(canvas_w, canvas_h), ImGuiChildFlags_Borders);

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();
	ImVec2 canvas_end(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Белый фон холста
	dl->AddRectFilled(canvas_pos, canvas_end, IM_COL32(255, 255, 255, 255));

	// Невидимая кнопка на всю площадь холста — чтобы ловить клики
	ImGui::InvisibleButton("canvas_click", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
	bool hovered = ImGui::IsItemHovered();

	ImVec2 mouse = ImGui::GetMousePos();
	ImVec2 local(mouse.x - canvas_pos.x, mouse.y - canvas_pos.y);

	int hovered_idx = -1;
	if (state == MOVE_POINT || state == DELETE_POINT) {
		hovered_idx = find_point_at(local);
	}

	// ----- Старт взаимодействия -----
	if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		if (state == ADD_POINT) {
			points.emplace_back(local.x, local.y);
			rebuild_curve();
		}
		else if (state == DELETE_POINT) {
			if (hovered_idx >= 0) {
				points.erase(points.begin() + hovered_idx);
				rebuild_curve();
			}
		}
		else if (state == MOVE_POINT) {
			if (hovered_idx >= 0) {
				dragging_index = hovered_idx;
			}
		}
	}

	// ----- Перетаскивание -----
	if (state == MOVE_POINT && dragging_index >= 0 &&
		ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		points[dragging_index] = local;
		rebuild_curve();
	}

	// ----- Конец перетаскивания -----
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		dragging_index = -1;
	}

	// ----- Курсор -----
	if ((state == MOVE_POINT || state == DELETE_POINT) && hovered_idx >= 0) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	// ОТРИСОВКА
	// Кривая
	if (curve.size() >= 2) {
		for (size_t i = 0; i + 1 < curve.size(); ++i) {
			ImVec2 a(curve[i].x + canvas_pos.x, curve[i].y + canvas_pos.y);
			ImVec2 b(curve[i + 1].x + canvas_pos.x, curve[i + 1].y + canvas_pos.y);
			dl->AddLine(a, b, IM_COL32(0, 0, 0, 255), 2.0f);
		}
	}

	// Добавленные (промежуточные) точки — красные
	for (size_t i = 0; i < prepared_points.size(); ++i) {
		if (!prepared_is_added[i]) continue;
		dl->AddCircleFilled(ImVec2(canvas_pos.x + prepared_points[i].x,
			canvas_pos.y + prepared_points[i].y),
			3.0f, IM_COL32(220, 60, 60, 255));
	}

	// Исходные (пользовательские) точки — синие / оранжевые (при наведении мыши) / увеличенные при премещении
	for (size_t i = 0; i < points.size(); ++i) {
		ImU32 color = (static_cast<int>(i) == hovered_idx)
			? IM_COL32(255, 140, 0, 255)
			: IM_COL32(60, 60, 200, 255);
		float radius = (static_cast<int>(i) == dragging_index) ? 6.0f : 4.0f;

		dl->AddCircleFilled(ImVec2(canvas_pos.x + points[i].x,
			canvas_pos.y + points[i].y),
			radius, color);
	}

	ImGui::EndChild();
}

void Task3::create_curve_segment(int start_point)
{
	for (double t = 0.0; t < 1.01; t += 0.02)
		curve.push_back(evaluate_Bezier_curve(prepared_points[start_point], prepared_points[start_point + 1], 
											  prepared_points[start_point + 2], prepared_points[start_point + 3], t));
}

ImVec2 Task3::evaluate_Bezier_curve(ImVec2& point1, ImVec2& point2, ImVec2& point3, ImVec2& point4, double t)
{
	double sub = 1 - t;

	double coef1 = sub * sub * sub;
	double coef2 = 3 * sub * sub * t;
	double coef3 = 3 * sub * t * t; 
	double coef4 = t * t * t;

	return ImVec2(coef1 * point1.x + coef2 * point2.x + coef3 * point3.x + coef4 * point4.x,
		coef1 * point1.y + coef2 * point2.y + coef3 * point3.y + coef4 * point4.y);
}

void Task3::prepare_points_for_curve()
{
	prepared_points.clear();
	prepared_is_added.clear();

	if (points.size() < 3) return;

	// Первые три точки — как есть
	prepared_points.push_back(points[0]); prepared_is_added.push_back(false);
	prepared_points.push_back(points[1]); prepared_is_added.push_back(false);
	prepared_points.push_back(points[2]); prepared_is_added.push_back(false);

	// Для каждой пары (X_{2k}, X_{2k+1}), k = 1, 2, 3, ...
	for (size_t k = 1; 2 * k + 1 < points.size(); ++k) {
		const ImVec2& a = points[2 * k];       // X_{2k}
		const ImVec2& b = points[2 * k + 1];   // X_{2k+1}

		// узел — середина
		prepared_points.push_back(ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f));
		prepared_is_added.push_back(true);

		// вторая точка пары
		prepared_points.push_back(b);
		prepared_is_added.push_back(false);

		// начало следующей пары, если есть
		if (2 * k + 2 < points.size()) {
			prepared_points.push_back(points[2 * k + 2]);
			prepared_is_added.push_back(false);
		}
	}

	if (prepared_points.back().x != points.back().x ||
		prepared_points.back().y != points.back().y) {
		prepared_points.back() = points.back();
		prepared_is_added.back() = false;
	}

	// Добиваем размер до 3N+1 дубликатами последней точки.
	while ((prepared_points.size() - 1) % 3 != 0) {
		prepared_points.push_back(points.back());
		prepared_is_added.push_back(false);  
	}
}

Task3::~Task3() noexcept {}

void Task3::prepare(const AppContext& ctx) {}

void Task3::draw(const AppContext& ctx) {

	const float BUTTON_PANEL_W(260.0f);

	ImVec2 content_avail = ImGui::GetContentRegionAvail();
	float avail_w = content_avail.x;
	float avail_h = content_avail.y;
	float canvas_w = avail_w - BUTTON_PANEL_W - ImGui::GetStyle().ItemSpacing.x;

	// ЛЕВАЯ ВЕРХНЯЯ ОБЛАСТЬ: белый холст
	draw_canvas(canvas_w, avail_h);
	ImGui::SameLine();

	// ПРАВАЯ ОБЛАСТЬ: панель управления
	ImGui::BeginChild("control_panel", ImVec2(BUTTON_PANEL_W, avail_h),
		ImGuiChildFlags_Borders);

	ImGui::Text("task:");
	ImGui::SetNextItemWidth(-FLT_MIN);
	static const char* TASK_NAMES[] = { "add point", "move_point", "delete point"};
	ImGui::Combo("##task", &state, TASK_NAMES, 3);

	ImGui::Spacing();
	ImGui::Spacing();

	if (ImGui::Button("clear canvas", ImVec2(-FLT_MIN, 0))) {
		points.clear();
		curve.clear();
		prepared_points.clear();
	}
	ImGui::EndChild();
}

