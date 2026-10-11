#define _CRT_SECURE_NO_WARNINGS
#include "tasks.h"
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

Task1::~Task1() noexcept {}

void Task1::prepare(const AppContext& ctx) // Первичная инициализация
{
	this->system.axiom.clear(); // Пустая аксиома — по умолчанию ничего не рисуется
	this->system.current_state.clear(); // Очищаем текущее состояние
	this->system.rules.clear(); // Очищаем правила
	this->reset_to_defaults(); // Сбрасываем все параметры к значениям по умолчанию
	this->file_dialog.reset(); //сброс состояния диалога
}

void Task1::reset_to_defaults() // Сброс всех параметров к значениям по умолчанию
{
	this->system.angle_step = 25;
	this->system.initial_angle = 90;
	this->system.base_thickness = 4;
	this->system.tip_thickness = 0.6;
	this->system.base_color = IM_COL32(40, 40, 180, 255);
	this->system.tip_color = IM_COL32(220, 60, 60, 255);
	this->system.leaf_length_factor = 0.8;
	this->system.random_angle_deg = 0;

	this->iteration_number = 3;
	this->color_mode = 0;
	this->tree_base_thickness = 6;
	this->tree_base_color = IM_COL32(101, 67, 33, 255);
	this->tree_tip_color = IM_COL32(60, 160, 60, 255);
	this->tree_random_angle = 15;
	this->random_angle = 0;

	this->offset_x = 0;
	this->offset_y = 0;
	this->zoom = 1;

	this->draw_only_visible = true;
	this->dirty = true;

	std::strncpy(this->status_msg, "Reset to defaults", sizeof(this->status_msg) - 1);
	this->status_msg[sizeof(this->status_msg) - 1] = '\0';
}

void Task1::compute_barier() // Вычисление границ фрактала
{
	if (this->segments.empty()) {
		this->bb_min_x = this->bb_min_y = this->bb_max_x = this->bb_max_y = 0.0f;
		return;
	}

	this->bb_min_x = this->bb_min_y = 1e30f;
	this->bb_max_x = this->bb_max_y = -1e30f;

	for (const auto& s : this->segments) { // Проходим по всем отрезкам и находим границы факториала
		this->bb_min_x = std::min({ this->bb_min_x, s.start.x, s.end.x });
		this->bb_min_y = std::min({ this->bb_min_y, s.start.y, s.end.y });
		this->bb_max_x = std::max({ this->bb_max_x, s.start.x, s.end.x });
		this->bb_max_y = std::max({ this->bb_max_y, s.start.y, s.end.y });
	}
}

void Task1::apply_gradient_color() // Раскраска по расстоянию от центра фрактала
{
	if (this->segments.empty()) return;

	// Координаты центра факториала
	float cx((this->bb_min_x + this->bb_max_x) / 2.f);
	float cy((this->bb_min_y + this->bb_max_y) / 2.f);

	float max_r(1e-6f);
	for (const auto& s : this->segments) {
		float mx((s.start.x + s.end.x) * 0.5f - cx);
		float my((s.start.y + s.end.y) * 0.5f - cy);
		float r(std::sqrt(mx * mx + my * my));
		if (r > max_r) max_r = r;
	}

	for (auto& s : this->segments) {
		float mx((s.start.x + s.end.x) * 0.5f - cx);
		float my((s.start.y + s.end.y) * 0.5f - cy);
		float r(std::sqrt(mx * mx + my * my));
		float t(r / max_r);
		s.color = l_system::lerp_color(this->system.base_color, this->system.tip_color, t);
	}
}

void Task1::apply_tree_color() // Раскраска дерева
{
	if (this->segments.empty()) return;

	int max_depth(0);
	for (const auto& s : this->segments)
		if (s.depth > max_depth) max_depth = s.depth;
	if (max_depth < 1) max_depth = 1;

	for (auto& s : this->segments) {
		float t((float)s.depth / (float)max_depth);
		s.color = l_system::lerp_color(this->tree_base_color,
			this->tree_tip_color, t);
		s.thickness = this->tree_base_thickness * (1.0f - t);
	}
}

void Task1::rebuild() // Пересборка отрезков и раскраска
{
	if (this->color_mode == 1) {
		this->system.base_color = this->tree_base_color;
		this->system.tip_color = this->tree_tip_color;
		this->system.base_thickness = this->tree_base_thickness;
		this->system.tip_thickness = 0.f;
		this->system.random_angle_deg = this->tree_random_angle;
	}
	else {
		this->system.base_thickness = 1.f;
		this->system.tip_thickness = 1.f;
		this->system.random_angle_deg = this->random_angle;
	}

	this->system.expand(this->iteration_number);
	this->segments = this->system.interpret();
	this->compute_barier();

	if (this->color_mode == 1) this->apply_tree_color();
	else this->apply_gradient_color();

	this->last_iterations = this->iteration_number;
	this->dirty = false;
}

void Task1::load_file(const char* path) //загрузка l-системы из файла
{
	if (!path || !path[0]) return;

	if (this->system.load_from_file(path)) {
		this->system.base_color = IM_COL32(40, 40, 180, 255);
		this->system.tip_color = IM_COL32(220, 60, 60, 255);
		this->system.leaf_length_factor = 0.8f;
		this->random_angle = 0.f;
		this->color_mode = 0;
		this->iteration_number = 5;
		this->dirty = true;
		this->offset_x = 0.f;
		this->offset_y = 0.f;
		this->zoom = 1.f;
		std::snprintf(this->status_msg, sizeof(this->status_msg), "Loaded: %s", path);
	}
	else {
		std::snprintf(this->status_msg, sizeof(this->status_msg), "Can't load: %s", path);
	}
}

void Task1::open_file_dialog(SDL_Window* window)
{
	if (this->file_dialog.pending) return;

	static const SDL_DialogFileFilter filters[] = {
	{ "Text files", "txt" },
	{ "All files",  "*"   }
	};

	this->file_dialog.reset();
	this->file_dialog.pending = true;

	SDL_ShowOpenFileDialog(
		onFileDialogResult,
		&this->file_dialog,
		window,
		filters,
		2,
		nullptr,
		false
	);
}

void Task1::poll_file_dialog()
{
	if (!this->file_dialog.ready) return;

	if (this->file_dialog.ok) {
		this->load_file(this->file_dialog.path.c_str());
	}
	else {
		std::snprintf(this->status_msg, sizeof(this->status_msg), "File dialog cancelled");
	}
	this->file_dialog.ready = false;
	this->file_dialog.ok = false;
	this->file_dialog.path.clear();
}

void Task1::draw_canvas(float canvas_w, float canvas_h)
{
	ImGui::BeginChild("system_canvas", ImVec2(canvas_w, canvas_h),
		ImGuiChildFlags_Borders);

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();
	ImVec2 canvas_end(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(canvas_pos, canvas_end, IM_COL32(255, 255, 255, 255));

	ImGui::InvisibleButton("system_canvas_btn", canvas_size,
		ImGuiButtonFlags_MouseButtonLeft |
		ImGuiButtonFlags_MouseButtonMiddle |
		ImGuiButtonFlags_MouseButtonRight);

	bool hovered = ImGui::IsItemHovered();
	bool active = ImGui::IsItemActive();

	if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
		ImVec2 d = ImGui::GetIO().MouseDelta;
		this->offset_x += d.x;
		this->offset_y += d.y;
	}
	if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
		ImVec2 d = ImGui::GetIO().MouseDelta;
		this->offset_x += d.x;
		this->offset_y += d.y;
	}
	if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
		!ImGui::GetIO().KeyShift) {
		ImVec2 d = ImGui::GetIO().MouseDelta;
		this->offset_x += d.x;
		this->offset_y += d.y;
	}

	if (hovered) {
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			ImVec2 mouse = ImGui::GetIO().MousePos;
			float mx = mouse.x;
			float my = mouse.y;

			float old_zoom = this->zoom;
			this->zoom *= (wheel > 0.0f) ? 1.1f : (1.0f / 1.1f);
			if (this->zoom < 0.02f) this->zoom = 0.02f;
			if (this->zoom > 500.0f) this->zoom = 500.0f;

			float factor = this->zoom / old_zoom;
			float ccx = canvas_pos.x + canvas_size.x * 0.5f;
			float ccy = canvas_pos.y + canvas_size.y * 0.5f;

			this->offset_x = (this->offset_x + (ccx - mx)) * factor - (ccx - mx);
			this->offset_y = (this->offset_y + (ccy - my)) * factor - (ccy - my);
		}
	}

	if (this->segments.empty()) {
		const char* hint = "Choose a file";
		ImVec2 sz = ImGui::CalcTextSize(hint);
		ImVec2 p(canvas_pos.x + (canvas_size.x - sz.x) * 0.5f,
			canvas_pos.y + (canvas_size.y - sz.y) * 0.5f);
		dl->AddText(p, IM_COL32(150, 150, 150, 255), hint);
		ImGui::EndChild();
		return;
	}

	const float margin = 20.0f;
	float bw = this->bb_max_x - this->bb_min_x;
	float bh = this->bb_max_y - this->bb_min_y;
	if (bw <= 1e-6f) bw = 1.0f;
	if (bh <= 1e-6f) bh = 1.0f;

	float avail_w = canvas_size.x - 2.0f * margin;
	float avail_h = canvas_size.y - 2.0f * margin;
	float base_scale = std::min(avail_w / bw, avail_h / bh);
	float scale = base_scale * this->zoom;

	float cx = (this->bb_min_x + this->bb_max_x) * 0.5f;
	float cy = (this->bb_min_y + this->bb_max_y) * 0.5f;
	float canvas_cx = canvas_pos.x + canvas_size.x * 0.5f + this->offset_x;
	float canvas_cy = canvas_pos.y + canvas_size.y * 0.5f + this->offset_y;

	auto to_screen = [&](const ImVec2& p) -> ImVec2 {
		return ImVec2(
			canvas_cx + (p.x - cx) * scale,
			canvas_cy + (p.y - cy) * scale
		);
		};

	float view_min_x = (canvas_pos.x - canvas_cx) / scale + cx;
	float view_max_x = (canvas_end.x - canvas_cx) / scale + cx;
	float view_min_y = (canvas_pos.y - canvas_cy) / scale + cy;
	float view_max_y = (canvas_end.y - canvas_cy) / scale + cy;

	float thickness_scale = std::max(0.5f, std::min(scale / base_scale, 3.0f));

	dl->PushClipRect(canvas_pos, canvas_end, true);

	size_t drawn(0);
	for (const auto& s : this->segments) {
		if (this->draw_only_visible) {
			float sx_min = std::min(s.start.x, s.end.x);
			float sx_max = std::max(s.start.x, s.end.x);
			float sy_min = std::min(s.start.y, s.end.y);
			float sy_max = std::max(s.start.y, s.end.y);

			if (sx_max < view_min_x) continue;
			if (sx_min > view_max_x) continue;
			if (sy_max < view_min_y) continue;
			if (sy_min > view_max_y) continue;
		}

		ImVec2 a = to_screen(s.start);
		ImVec2 b = to_screen(s.end);
		float th = std::max(0.5f, std::min(s.thickness * thickness_scale, 12.0f));
		dl->AddLine(a, b, s.color, th);
		++drawn;
	}

	dl->PopClipRect();

	this->drawn_last_frame = drawn;

	ImGui::EndChild();
}

void Task1::draw_controls()
{
	ImGui::Text("L-System");
	ImGui::Separator();

	if (ImGui::Button("Open file...", ImVec2(-FLT_MIN, 0))) this->open_file_dialog(nullptr);

	ImGui::Spacing();

	if (ImGui::Button("Regenerate", ImVec2(-FLT_MIN, 0))) {
		this->dirty = true;
		std::snprintf(this->status_msg, sizeof(this->status_msg), "Regenerated");
	}

	if (ImGui::Button("Reset to defaults", ImVec2(-FLT_MIN, 0))) {
		this->reset_to_defaults();
	}

	if (this->status_msg[0] != '\0') ImGui::TextWrapped("%s", this->status_msg);

	ImGui::Separator();
	ImGui::Text("Color mode");

	int mode = this->color_mode;
	if (ImGui::RadioButton("Gradient", &mode, 0)) { this->color_mode = 0; this->dirty = true; }
	ImGui::SameLine();
	if (ImGui::RadioButton("Tree", &mode, 1)) { this->color_mode = 1; this->dirty = true; }

	ImGui::Separator();

	if (ImGui::SliderInt("Iterations", &this->iteration_number, 0, 50)) this->dirty = true;
	if (ImGui::SliderFloat("Angle step", &this->system.angle_step, 1, 180)) this->dirty = true;
	if (ImGui::SliderFloat("Initial angle", &this->system.initial_angle, -180, 180)) this->dirty = true;

	ImGui::Separator();

	if (this->color_mode == 0) {
		ImGui::Text("Gradient colors");
		float base_rgba[4] = {
			((this->system.base_color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
			((this->system.base_color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
			((this->system.base_color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
			((this->system.base_color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
		};
		if (ImGui::ColorEdit4("Center color", base_rgba)) {
			this->system.base_color = IM_COL32(
				(int)(base_rgba[0] * 255), (int)(base_rgba[1] * 255),
				(int)(base_rgba[2] * 255), (int)(base_rgba[3] * 255));
			this->dirty = true;
		}
		float tip_rgba[4] = {
			((this->system.tip_color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
			((this->system.tip_color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
			((this->system.tip_color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
			((this->system.tip_color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
		};
		if (ImGui::ColorEdit4("Edge color", tip_rgba)) {
			this->system.tip_color = IM_COL32(
				(int)(tip_rgba[0] * 255), (int)(tip_rgba[1] * 255),
				(int)(tip_rgba[2] * 255), (int)(tip_rgba[3] * 255));
			this->dirty = true;
		}
		if (ImGui::SliderFloat("Random angle +-", &this->random_angle, 0, 360, "%.2f"))
			this->dirty = true;
	}
	else {
		ImGui::Text("Tree colors & thickness");
		float base_rgba[4] = {
			((this->tree_base_color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
			((this->tree_base_color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
			((this->tree_base_color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
			((this->tree_base_color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
		};
		if (ImGui::ColorEdit4("Trunk color", base_rgba)) {
			this->tree_base_color = IM_COL32(
				(int)(base_rgba[0] * 255), (int)(base_rgba[1] * 255),
				(int)(base_rgba[2] * 255), (int)(base_rgba[3] * 255));
			this->dirty = true;
		}
		float tip_rgba[4] = {
			((this->tree_tip_color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
			((this->tree_tip_color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
			((this->tree_tip_color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
			((this->tree_tip_color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
		};
		if (ImGui::ColorEdit4("Leaf color", tip_rgba)) {
			this->tree_tip_color = IM_COL32(
				(int)(tip_rgba[0] * 255), (int)(tip_rgba[1] * 255),
				(int)(tip_rgba[2] * 255), (int)(tip_rgba[3] * 255));
			this->dirty = true;
		}
		if (ImGui::SliderFloat("Trunk thickness", &this->tree_base_thickness, 0.5f, 20.0f))
			this->dirty = true;
		if (ImGui::SliderFloat("Random branch angle +-", &this->tree_random_angle, 0, 45))
			this->dirty = true;
		if (ImGui::SliderFloat("Leaf length factor", &this->system.leaf_length_factor, 0.01f, 2.0f, "%.3f"))
			this->dirty = true;
	}

	ImGui::Separator();
	ImGui::Checkbox("Draw only visible", &this->draw_only_visible);

	ImGui::Separator();
	if (ImGui::Button("Reset view")) {
		this->offset_x = this->offset_y = 0;
		this->zoom = 1;
	}
	ImGui::SameLine();
	ImGui::Text("Zoom: %.4f", this->zoom);
	ImGui::Text("Segments total: %zu", this->segments.size());
	ImGui::Text("Segments drawn: %zu", this->drawn_last_frame);
	ImGui::Text("String len: %zu", this->system.current_state.size());
}

void Task1::draw(const AppContext& ctx)
{
	const float PANEL_W = 320.0f;

	if (this->dirty || this->last_iterations != this->iteration_number) {
		this->rebuild();
	}

	this->poll_file_dialog();

	ImVec2 avail = ImGui::GetContentRegionAvail();
	float canvas_w = avail.x - PANEL_W - ImGui::GetStyle().ItemSpacing.x;

	this->draw_canvas(canvas_w, avail.y);
	ImGui::SameLine();

	ImGui::BeginChild("system_controls", ImVec2(PANEL_W, avail.y),
		ImGuiChildFlags_Borders);
	this->draw_controls();
	ImGui::EndChild();
}

Task2::~Task2() noexcept {}

void Task2::prepare(const AppContext& ctx) {}

void Task2::draw(const AppContext& ctx)
{
	ImGui::Text("Task 2 is not implemented yet");
}

int Task3::find_point_at(const ImVec2& local_pos) const
{
	const float HIT_RADIUS = 8.0f;
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

	dl->AddRectFilled(canvas_pos, canvas_end, IM_COL32(255, 255, 255, 255));

	ImGui::InvisibleButton("canvas_click", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
	bool hovered = ImGui::IsItemHovered();

	ImVec2 mouse = ImGui::GetMousePos();
	ImVec2 local(mouse.x - canvas_pos.x, mouse.y - canvas_pos.y);

	int hovered_idx = -1;
	if (state == MOVE_POINT || state == DELETE_POINT) {
		hovered_idx = find_point_at(local);
	}

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

	if (state == MOVE_POINT && dragging_index >= 0 &&
		ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		points[dragging_index] = local;
		rebuild_curve();
	}

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		dragging_index = -1;
	}

	if ((state == MOVE_POINT || state == DELETE_POINT) && hovered_idx >= 0) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	if (curve.size() >= 2) {
		for (size_t i = 0; i + 1 < curve.size(); ++i) {
			ImVec2 a(curve[i].x + canvas_pos.x, curve[i].y + canvas_pos.y);
			ImVec2 b(curve[i + 1].x + canvas_pos.x, curve[i + 1].y + canvas_pos.y);
			dl->AddLine(a, b, IM_COL32(0, 0, 0, 255), 2.0f);
		}
	}

	for (size_t i = 0; i < prepared_points.size(); ++i) {
		if (!prepared_is_added[i]) continue;
		dl->AddCircleFilled(ImVec2(canvas_pos.x + prepared_points[i].x,
			canvas_pos.y + prepared_points[i].y),
			3.0f, IM_COL32(220, 60, 60, 255));
	}

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

bool Task3::is_collinear(const ImVec2& a, const ImVec2& b, const ImVec2& c)
{
	float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	return std::fabs(cross) < 1e-3f;
}

void Task3::prepare_points_for_curve()
{
	prepared_points = points;
	prepared_is_added.assign(points.size(), false);

	size_t i = 3;
	while (i + 1 < prepared_points.size()) {
		const ImVec2& A = prepared_points[i - 1];
		const ImVec2& B = prepared_points[i];
		const ImVec2& C = prepared_points[i + 1];

		if (!is_collinear(A, B, C)) {
			ImVec2 mid((A.x + B.x) * 0.5f, (A.y + B.y) * 0.5f);
			prepared_points.insert(prepared_points.begin() + i, mid);
			prepared_is_added.insert(prepared_is_added.begin() + i, true);
		}
		i += 3;
	}

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

	draw_canvas(canvas_w, avail_h);
	ImGui::SameLine();

	ImGui::BeginChild("control_panel", ImVec2(BUTTON_PANEL_W, avail_h),
		ImGuiChildFlags_Borders);

	ImGui::Text("task:");
	ImGui::SetNextItemWidth(-FLT_MIN);
	static const char* TASK_NAMES[] = { "add point", "move_point", "delete point" };
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