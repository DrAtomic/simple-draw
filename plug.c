#include <assert.h>
#include <stdbool.h>

#include "arena.h"
#include "plug.h"
#include "raylib.h"
#include "raylib_helpers.h"
#include "raymath.h"

static void points_init(Arena *a, point_buf *pb, size_t cap)
{
	pb->data = arena_push_array(a, cap, brush_pt);
	pb->count = 0;
	pb->cap = cap;
}

void plug_init(Plug *plug)
{
	initialize_arena(&plug->world_arena, sizeof(*plug), plug->permanent_storage);
	initialize_arena(&plug->stroke_arena, plug->permanent_storage_size - sizeof(*plug), plug->permanent_storage + sizeof(*plug));

	plug->eraser_radius = 8.0f;
	plug->camera = arena_push_struct(&plug->world_arena, Camera2D);
	plug->camera->zoom = 1.0f;
	points_init(&plug->stroke_arena, &plug->points, 1000000);
}

void plug_pre_reload(Plug *plug)
{
	(void)plug;
}

void plug_post_reload(Plug *plug)
{
	(void)plug;
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
	row->start = 0;
	row->count = 0;
	row->down  = NULL;

	if (!g->tail) {
		g->head = g->tail = row;
	} else {
		g->tail->down = row;
		g->tail = row;
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
	for (stroke_list *row = g->head; row; row = row->down) {
		if (row->count < 2)
			continue;

		size_t s = row->start;
		size_t e = s + row->count;

		for (size_t i = s; i + 1 < e; ++i) {
			const brush_pt *a = &pb->data[i];
			const brush_pt *b = &pb->data[i+1];
			DrawLineEx(a->pos, b->pos, a->size, a->brush_color);
		}
	}
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

static bool erase_at(Plug *plug, Vector2 p, float radius)
{
	stroke_grid *g = &plug->grid;
	point_buf   *pb = &plug->points;

	for (stroke_list *row = g->head; row; row = row->down) {
		if (row->count < 2)
			continue;

		size_t s = row->start;
		size_t e = s + row->count;

		for (size_t i = s; i + 1 < e; ++i) {
			if (dist_point_segment(p, pb->data[i].pos, pb->data[i+1].pos) <= radius) {
				size_t left_count  = (i - s + 1);
				size_t right_start = i + 1;
				size_t right_count = e - right_start;

				if (right_count > 0) {
					stroke_list *below = stroke_grid_insert_row_below(&plug->stroke_arena, g, row);
					below->start = right_start;
					below->count = right_count;
				}
				row->count = left_count;
				return true;
			}
		}
	}
	return false;
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

static void handle_input(Plug *plug)
{
	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), *plug->camera);
	mouse_and_camera_stuff(plug->camera, &mouse_pos, &mouse_2d_pos);

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
			if (erase_at(plug, mouse_2d_pos, plug->eraser_radius)) {
				stroke_grid_cleanup(&plug->grid);
			}
		} else {
			const brush_pt p = { .pos = mouse_2d_pos, .size = 2.0f, .brush_color = RED };
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
			if (plug->erasing)
				DrawCircleLinesV(GetScreenToWorld2D(GetMousePosition(), *plug->camera), plug->eraser_radius, RAYWHITE);
		}
		EndMode2D();
	}
	EndDrawing();
}
