#include <assert.h>
#include <stdbool.h>

#include "arena.h"
#include "plug.h"
#include "raylib.h"
#include "raylib_helpers.h"
#include "raymath.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

static void points_init(Arena *a, point_buf *pb, size_t cap)
{
	pb->data = arena_push_array(a, cap, brush_pt);
	pb->count = 0;
	pb->cap = cap;
}

static Image build_color_wheel_image(int diameter, float val)
{
	size_t W = diameter;
	size_t H = diameter;
	Image img = GenImageColor(W, H, BLANK);
	Color *px = (Color*)img.data;

	float R = diameter * 0.5f;
	float cx = R;
	float cy = R;

	for (size_t y = 0; y < H; ++y) {
		for (size_t x = 0; x < W; ++x) {
			float dx = (float)x - cx;
			float dy = (float)y - cy;
			float r = sqrtf(dx*dx + dy*dy);
			if (r <= R) {
				float sat = r / R;
				float hue = atan2f(dy, dx) * (180.0f/PI);

				if (hue < 0)
					hue += 360.0f;

				Color c = ColorFromHSV(hue, sat, val);

				float edge = 1.0f - fmaxf(0.0f, (r - (R-1.0f)));
				unsigned char a = (unsigned char) (255.0f * fminf(1.0f, edge));
				c.a = a;
				px[y*W + x] = c;
			} else {
				px[y*W + x] = BLANK;
			}
		}
	}
	return img;
}

static Texture2D build_color_wheel_texture(int diameter, float val)
{
	Image img = build_color_wheel_image(diameter, val);
	Texture2D tex = LoadTextureFromImage(img);
	UnloadImage(img);
	SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
	return tex;
}

void plug_init(Plug *plug)
{
	uint8_t *base = (uint8_t*)plug->permanent_storage;
	size_t cap = plug->permanent_storage_size;
	size_t world_bytes = sizeof(*plug);
	initialize_arena(&plug->world_arena, world_bytes, base);

	size_t erase_bytes = Megabytes(8);
	initialize_arena(&plug->erase_arena, erase_bytes, base + world_bytes);
	size_t stroke_bytes = cap - world_bytes - erase_bytes;
	initialize_arena(&plug->stroke_arena, stroke_bytes, base + world_bytes + erase_bytes);

	plug->brush_size = 8.0f;
	plug->camera = arena_push_struct(&plug->world_arena, Camera2D);
	plug->camera->zoom = 1.0f;
	points_init(&plug->stroke_arena, &plug->points, 1000000);

	plug->wheel_diam = 200;
	plug->wheel_pos  = (Vector2){ 140.0f, 120.0f };
	plug->color_wheel_hue   = 0.0f;
	plug->color_wheel_sat   = 1.0f;
	plug->color_wheel_val   = 1.0f;
	plug->color_wheel_alpha = 1.0f;

	plug->color_wheel_val_slider = (Rectangle){
		plug->wheel_pos.x + plug->wheel_diam / 2 + 16,
		plug->wheel_pos.y - plug->wheel_diam / 2,
		14,
		(float)plug->wheel_diam
	};

	plug->wheel_tex = build_color_wheel_texture(plug->wheel_diam, plug->color_wheel_val);
	plug->color_wheel_picker_btn = (Rectangle){ 12, 12, 28, 28 };

	plug->brush_min = 1.0f;
	plug->brush_max = 100.0f;
	float wheel_right = plug->wheel_pos.x + plug->wheel_diam * 0.5f;
	plug->color_wheel_val_slider = (Rectangle){ wheel_right + 16, plug->wheel_pos.y - plug->wheel_diam*0.5f, 14, (float)plug->wheel_diam };
	plug->brush_size_slider = (Rectangle){ plug->color_wheel_val_slider.x + plug->color_wheel_val_slider.width + 12, plug->color_wheel_val_slider.y, 14, plug->color_wheel_val_slider.height };
	plug->brush_color = (Color){0xff, 0x00, 0x00, 0xff};
}

void plug_pre_reload(Plug *plug)
{
	(void)plug;
}

void plug_post_reload(Plug *plug)
{
	(void)plug;
}

static void handle_size_slider_input(Plug *plug)
{
    Vector2 m = GetMousePosition();
    if (!CheckCollisionPointRec(m, plug->brush_size_slider))
	    return;
    if (!(IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
	    return;

    float t = (m.y - plug->brush_size_slider.y) / plug->brush_size_slider.height;
    if (t < 0)
	    t = 0;
    if (t > 1)
	    t = 1;
    float u = 1.0f - t;
    plug->brush_size = plug->brush_min + u * (plug->brush_max - plug->brush_min);
}

static void draw_size_slider_UI(const Plug *plug)
{
	DrawRectangleRec(plug->brush_size_slider, (Color){30,30,30,255});
	DrawRectangleLinesEx(plug->brush_size_slider, 1.0 ,RAYWHITE);

	float u = (plug->brush_size - plug->brush_min) / (plug->brush_max - plug->brush_min);
	float ty = plug->brush_size_slider.y + (1.0f - u) * plug->brush_size_slider.height;

	DrawLine((int)plug->brush_size_slider.x - 4, (int)ty, (int)(plug->brush_size_slider.x + plug->brush_size_slider.width) + 4, (int)ty, WHITE);
}


static void draw_picker_button(const Plug *plug)
{
	DrawRectangleRec(plug->color_wheel_picker_btn, (plug->color_wheel_picker_open ? DARKGRAY : GRAY));
	DrawRectangleLinesEx(plug->color_wheel_picker_btn, 1.0, RAYWHITE);
	const char *label = plug->color_wheel_picker_open ? "X" : "C";
	int tw = MeasureText(label, 18);
	DrawText(label,
		 (int)(plug->color_wheel_picker_btn.x + (plug->color_wheel_picker_btn.width - tw)/2),
		 (int)(plug->color_wheel_picker_btn.y + 5), 18, RAYWHITE);
}

static bool mouse_over_rect(Rectangle r, Vector2 m)
{
    return CheckCollisionPointRec(m, r);
}

static bool mouse_over_circle(Vector2 m, Vector2 c, float radius)
{
	Vector2 d = Vector2Subtract(m, c);
	return Vector2LengthSqr(d) <= radius*radius;
}

static void handle_color_wheel_input(Plug *plug)
{
	Vector2 m = GetMousePosition();
	float R = plug->wheel_diam * 0.5f;

	bool overWheel = mouse_over_circle(m, plug->wheel_pos, R);
	bool overVal   = CheckCollisionPointRec(m, plug->color_wheel_val_slider);

	if (overVal && (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
		float t = (m.y - plug->color_wheel_val_slider.y) / plug->color_wheel_val_slider.height;
		if (t < 0)
			t = 0;
		if (t > 1)
			t = 1;
		plug->color_wheel_val = 1.0f - t;
		plug->wheel_tex = build_color_wheel_texture(plug->wheel_diam, plug->color_wheel_val);
	}

	if (overWheel && (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
		Vector2 d  = Vector2Subtract(m, plug->wheel_pos);
		float   ang= atan2f(d.y, d.x) * (180.0f/PI);
		if (ang < 0) ang += 360.0f;
		float   r  = Vector2Length(d);
		float   sat = r / R;
		if (sat < 0)
			sat = 0;
		if (sat > 1)
			sat = 1;
		plug->color_wheel_hue = ang;
		plug->color_wheel_sat = sat;
	}

	Color sel = ColorFromHSV(plug->color_wheel_hue, plug->color_wheel_sat, plug->color_wheel_val);
	sel.a = (unsigned char)(plug->color_wheel_alpha * 255.0f);
	plug->brush_color = sel;
}

static void draw_color_wheel_UI(const Plug *plug)
{
	Rectangle src = { 0, 0, (float)plug->wheel_tex.width, (float)plug->wheel_tex.height };
	Rectangle dst = {
		plug->wheel_pos.x - plug->wheel_diam / 2.0f,
		plug->wheel_pos.y - plug->wheel_diam / 2.0f,
		(float)plug->wheel_diam,
		(float)plug->wheel_diam
	};
	DrawTexturePro(plug->wheel_tex, src, dst, (Vector2){0,0}, 0.0f, WHITE);

	float R = plug->wheel_diam * 0.5f;
	float rad = plug->color_wheel_sat * R;
	float ang = plug->color_wheel_hue * (PI/180.0f);
	Vector2 p = {
		plug->wheel_pos.x + rad*cosf(ang),
		plug->wheel_pos.y + rad*sinf(ang)
	};
	DrawCircleLinesV(p, 6.0f, RAYWHITE);
	DrawCircleLinesV(p, 4.0f, BLACK);

	DrawRectangleRec(plug->color_wheel_val_slider, BLACK);

	float ty = plug->color_wheel_val_slider.y + (1.0f - plug->color_wheel_val) * plug->color_wheel_val_slider.height;
	DrawLine((int)plug->color_wheel_val_slider.x - 4, (int)ty, (int)(plug->color_wheel_val_slider.x + plug->color_wheel_val_slider.width) + 4, (int)ty, WHITE);

	Rectangle sw = { plug->color_wheel_val_slider.x + plug->color_wheel_val_slider.width + 15 + plug->brush_size_slider.width, plug->color_wheel_val_slider.y, 32, 32 };
	DrawRectangleRec(sw, plug->brush_color);
	DrawRectangleLines((int)sw.x, (int)sw.y, (int)sw.width, (int)sw.height, DARKGRAY);
}

static void reset_strokes(Arena *a, stroke_grid *g, point_buf *pb)
{
	g->head = NULL;
	g->tail = NULL;
	a->used = 0;
	points_init(a, pb, pb->cap);
}

static size_t points_push(point_buf *pb, brush_pt p)
{
	assert(pb->count < pb->cap && "point buffer full");

	pb->data[pb->count] = p;

	return pb->count++;
}

static void stroke_grid_add_row(Arena *a, stroke_grid *g)
{
	stroke_list *row = arena_push_struct(a, stroke_list);

	if (!g->tail) {
		g->head = g->tail = row;
	} else {
		g->tail->down = row;
		g->tail = row;
	}
}

static void draw_row_stamp(const point_buf *pb, const stroke_list *row)
{
	if (row->count < 2)
		return;
	size_t s = row->start;
	size_t e = s + row->count;

	for (size_t i = s; i + 1 < e; ++i) {
		const brush_pt *A = &pb->data[i];
		const brush_pt *B = &pb->data[i+1];

		Vector2 ab = Vector2Subtract(B->pos, A->pos);
		float len = Vector2Length(ab);

		if (len <= 0.0f) {
			DrawCircleV(A->pos, A->size * 0.5f, A->brush_color);
			continue;
		}

		float r0 = A->size * 0.5f;
		float r1 = B->size * 0.5f;
		float step = fmaxf(1.0f, 0.5f * fminf(r0, r1));

		Vector2 dir = Vector2Scale(ab, 1.0f / len);
		float t = 0.0f;
		while (t <= len) {
			float u = (len > 0.0f) ? t / len : 0.0f;
			Vector2 p = Vector2Add(A->pos, Vector2Scale(dir, t));
			float r = (1.0f - u) * r0 + u * r1;
			Color c = A->brush_color;
			DrawCircleV(p, r, c);
			t += step;
		}
		DrawCircleV(B->pos, r1, B->brush_color);
	}
}

static void stroke_row_add_point(point_buf *pb, stroke_list *row, brush_pt p)
{
	size_t idx = points_push(pb, p);

	if (row->count == 0)
		row->start = idx;

	row->count++;
}

static void draw_all_brushes(const stroke_grid *g, const point_buf *pb)
{
	for (const stroke_list *row = g->head; row; row = row->down)
		draw_row_stamp(pb, row);
}

static float dist_point_segment(Vector2 p, Vector2 a, Vector2 b)
{
	Vector2 ab = Vector2Subtract(b, a);
	float ab2 = Vector2DotProduct(ab, ab);

	if (ab2 <= 1e-6f) {
		return Vector2Length(Vector2Subtract(p, a));
	}
	float t = Vector2DotProduct(Vector2Subtract(p, a), ab) / ab2;

	if (t < 0.0f)
		t = 0.0f;
	else if (t > 1.0f)
		t = 1.0f;

	Vector2 proj = Vector2Add(a, Vector2Scale(ab, t));
	return Vector2Length(Vector2Subtract(p, proj));
}

static stroke_list *stroke_grid_insert_row_below(Arena *a, stroke_grid *g, stroke_list *above)
{
	stroke_list *row = arena_push_struct(a, stroke_list);

	row->down  = above->down;
	above->down = row;

	if (g->tail == above)
		g->tail = row;

	return row;
}

static void stroke_grid_cleanup(stroke_grid *g)
{
	stroke_list *prev = NULL;
	stroke_list *cur = g->head;

	while (cur) {
		if (cur->count == 0) {
			if (prev)
				prev->down = cur->down;
			else
				g->head = cur->down;

			if (g->tail == cur)
				g->tail = prev;

			cur = (prev ? prev->down : g->head);
		} else {
			prev = cur;
			cur = cur->down;
		}
	}

	if (!g->head)
		g->tail = NULL;
}

static int cut_first_hit_in_row(Plug *plug, stroke_list *row, Vector2 p, float radius)
{
	point_buf *pb = &plug->points;
	if (row->count < 2)
		return 0;

	size_t s = row->start;
	size_t e = s + row->count;

	for (size_t i = s; i + 1 < e; ++i) {
		Vector2 A = pb->data[i].pos, B = pb->data[i+1].pos;

		float minx = (A.x < B.x ? A.x : B.x) - radius;
		float maxx = (A.x > B.x ? A.x : B.x) + radius;
		float miny = (A.y < B.y ? A.y : B.y) - radius;
		float maxy = (A.y > B.y ? A.y : B.y) + radius;

		if (p.x < minx || p.x > maxx || p.y < miny || p.y > maxy)
			continue;

		if (dist_point_segment(p, A, B) <= radius) {
			size_t left_count  = (i - s + 1);
			size_t right_start = i + 1;
			size_t right_count = e - right_start;

			if (right_count > 0) {
				stroke_list *below = stroke_grid_insert_row_below(&plug->stroke_arena, &plug->grid, row);
				below->start = right_start;
				below->count = right_count;
			}
			row->count = left_count;
			return 1;
		}
	}
	return 0;
}

static bool batch_erase_at(Plug *plug, Vector2 p, float radius, int max_cuts)
{
	stroke_grid *g = &plug->grid;

	if (!g->head)
		return false;

	int row_count = 0;
	for (stroke_list *r = g->head; r; r = r->down)
		row_count++;

	if (row_count == 0)
		return false;

	stroke_list **rows = arena_push_array(&plug->erase_arena, row_count, stroke_list*);
	int idx = 0;

	for (stroke_list *r = g->head; r; r = r->down)
		rows[idx++] = r;

	bool changed = false;
	int cuts = 0;

	for (int i = row_count - 1; i >= 0 && cuts < max_cuts; --i) {
		stroke_list *cur = rows[i];
		while (cur && cuts < max_cuts) {
			int did = cut_first_hit_in_row(plug, cur, p, radius);
			if (!did) break;
			changed = true;
			cuts++;
			cur = cur->down;
		}
	}

	if (changed)
		stroke_grid_cleanup(g);

	return changed;
}

static void handle_input(Plug *plug)
{
	plug->erase_arena.used = 0;

	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), *plug->camera);
	mouse_and_camera_stuff(plug->camera, &mouse_pos, &mouse_2d_pos);

	if (IsKeyPressed(KEY_C) && !plug->dragging) {
		plug->color_wheel_picker_open = !plug->color_wheel_picker_open;
	}
	bool is_mouse_over_rect = mouse_over_rect(plug->color_wheel_picker_btn, mouse_pos);
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && is_mouse_over_rect) {
		plug->color_wheel_picker_open = !plug->color_wheel_picker_open;
		return;
	}

	if (plug->color_wheel_picker_open) {
		bool over_wheel = mouse_over_circle(mouse_pos, plug->wheel_pos, plug->wheel_diam * 0.5f);
		bool over_val   = CheckCollisionPointRec(mouse_pos, plug->color_wheel_val_slider);
		bool over_size  = CheckCollisionPointRec(mouse_pos, plug->brush_size_slider);

		handle_color_wheel_input(plug);
		handle_size_slider_input(plug);

		if ((IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) && (over_wheel || over_val || over_size || is_mouse_over_rect)) {
			return;
		}
	}

	if (!plug->erasing) {
		if (IsKeyPressed(KEY_UP))
			plug->brush_size += 1.0f;
		if (IsKeyPressed(KEY_DOWN))
			plug->brush_size -= 1.0f;

		if (plug->brush_size < 1.0f)
			plug->brush_size = 1.0f;
		if (plug->brush_size > 100.0f)
			plug->brush_size = 100.0f;
	}
	if (IsKeyPressed(KEY_E) && !plug->dragging) {
		plug->erasing = !plug->erasing;
	}

	if (IsKeyPressed(KEY_D) && !plug->dragging) {
		reset_strokes(&plug->stroke_arena, &plug->grid, &plug->points);
		return;
	}

	if (!plug->dragging) {
		if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
			plug->dragging = true;

			if (!plug->erasing) {
				if (!plug->grid.tail || plug->grid.tail->count != 0) {
					stroke_grid_add_row(&plug->stroke_arena, &plug->grid);
				}
			}
		}
	} else {

		if (plug->erasing) {
			if (batch_erase_at(plug, mouse_2d_pos, plug->brush_size, 64)) {
				stroke_grid_cleanup(&plug->grid);
			}
		} else {
			const brush_pt p = { .pos = mouse_2d_pos, .size = plug->brush_size, .brush_color = plug->brush_color };
			stroke_row_add_point(&plug->points, plug->grid.tail, p);
		}


		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			plug->dragging = false;

			if (plug->erasing) {
				stroke_grid_cleanup(&plug->grid);

				if (plug->grid.tail && plug->grid.tail->count != 0) {
					stroke_grid_add_row(&plug->stroke_arena, &plug->grid);
				}
			} else {
				stroke_grid_add_row(&plug->stroke_arena, &plug->grid);
			}
		}
	}
}

void plug_update(Plug *plug)
{
	handle_input(plug);

	BeginDrawing();
	{
		ClearBackground(GetColor(0x151515FF));
		BeginMode2D(*plug->camera);
		{
			draw_all_brushes(&plug->grid, &plug->points);
			if (plug->erasing) {
				DrawCircleLinesV(GetScreenToWorld2D(GetMousePosition(), *plug->camera), plug->brush_size / 2, RAYWHITE);
			} else {
				DrawCircleLinesV(GetScreenToWorld2D(GetMousePosition(), *plug->camera), plug->brush_size / 2, plug->brush_color);
			}
		}
		EndMode2D();
		draw_picker_button(plug);
		if (plug->color_wheel_picker_open) {
			draw_color_wheel_UI(plug);
			draw_size_slider_UI(plug);
		}

	}
	EndDrawing();
}
