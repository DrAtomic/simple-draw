#include <stdbool.h>

#include "arena.h"
#include "plug.h"
#include "raylib.h"
#include "raylib_helpers.h"
#include "raymath.h"

void plug_init(Plug *plug)
{
	initialize_arena(&plug->world_arena, sizeof(*plug), plug->permanent_storage);
	initialize_arena(&plug->stroke_arena, plug->permanent_storage_size - sizeof(*plug), plug->permanent_storage + sizeof(*plug));

	plug->eraser_radius = 8.0f;
	plug->camera = arena_push_struct(&plug->world_arena, Camera2D);
	plug->camera->zoom = 1.0f;
}

void plug_pre_reload(Plug *plug)
{
	(void)plug;
}

void plug_post_reload(Plug *plug)
{
	(void)plug;
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

	row->root = NULL;
	row->down = above->down;
	above->down = row;

	if (g->tail == above)
		g->tail = row;

	return row;
}


static bool erase_at(Plug *plug, Vector2 p, float radius)
{
	stroke_grid *g = &plug->grid;

	for (stroke_list *row = g->head; row; row = row->down) {
		for (brush_node *n = row->root; n && n->next; n = n->next) {
			if (dist_point_segment(p, n->el.pos, n->next->el.pos) <= radius) {
				brush_node *tail = n->next;
				n->next = NULL;

				stroke_list *below = stroke_grid_insert_row_below(&plug->stroke_arena, g, row);
				below->root = tail;

				return true;
			}
		}
	}
	return false;
}

static void draw(const brush *cur, const brush *next)
{
	DrawLineEx(cur->pos, next->pos, cur->size, cur->brush_color);
}

static void draw_all_brushes(stroke_list *head)
{
	for (stroke_list *row = head; row; row = row->down) {
		for (brush_node *n = row->root; n && n->next; n = n->next) {
			draw(&n->el, &n->next->el);
		}
	}
}

static void stroke_list_push(Arena *a, stroke_list *l, const brush b)
{
	brush_node *node = arena_push_struct(a, brush_node);

	node->el = b;
	node->next = l->root;
	l->root = node;
}

static void stroke_grid_add_row(Arena *a, stroke_grid *g)
{
	stroke_list *row = arena_push_struct(a, stroke_list);

	if (g->tail == NULL) {
		g->head = row;
		g->tail = row;
	} else {
		g->tail->down = row;
		g->tail = row;
	}
}

static void reset_strokes(Arena *a, stroke_grid *g)
{
	g->head = NULL;
	g->tail = NULL;
	a->used = 0;
}

static void stroke_grid_cleanup(stroke_grid *g)
{
	stroke_list *prev = NULL, *cur = g->head;
	while (cur) {
		if (cur->root == NULL) {
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
	if (!g->head) g->tail = NULL;
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
		reset_strokes(&plug->stroke_arena, &plug->grid);
		return;
	}

	if (!plug->dragging) {
		if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
			plug->dragging = true;

			if (!plug->erasing && !plug->grid.tail) {
				stroke_grid_add_row(&plug->stroke_arena, &plug->grid);
			}
		}
	} else {
		if (plug->erasing) {
			if (erase_at(plug, mouse_2d_pos, plug->eraser_radius)) {
				stroke_grid_cleanup(&plug->grid);
			}
		} else {
			const brush b = { .pos = mouse_2d_pos, .size = 2, .brush_color = SKYBLUE };
			stroke_list_push(&plug->stroke_arena, plug->grid.tail, b);
		}

		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			plug->dragging = false;

			if (plug->erasing) {
				stroke_grid_cleanup(&plug->grid);

				if (plug->grid.tail && plug->grid.tail->root != NULL) {
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
			draw_all_brushes(plug->grid.head);
		}
		EndMode2D();
	}
	EndDrawing();
}
