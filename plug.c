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

static void draw(const struct brush *cur, const struct brush *next)
{
	DrawLineEx(cur->pos, next->pos, cur->size, cur->brush_color);
}

static void draw_all_brushes(stroke_list *head)
{
	for (stroke_list *row = head; row; row = row->down) {
		for (struct hbb_node *n = row->root; n && n->next; n = n->next) {
			draw(&n->el, &n->next->el);
		}
	}
}

static void stroke_list_push(struct Arena *a, stroke_list *l, brush b)
{
	struct hbb_node *node = arena_push_struct(a, struct hbb_node);

	node->el = b;
	node->next = l->root;
	l->root = node;
}

static void stroke_list_add_row(struct Arena *a, stroke_grid *g)
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

static void reset_strokes(struct Arena *a, stroke_grid *g)
{
	g->head = NULL;
	g->tail = NULL;
	a->used = 0;
}

static void handle_input(Plug *plug)
{
	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), *plug->camera);
	mouse_and_camera_stuff(plug->camera, &mouse_pos, &mouse_2d_pos);

	if (!plug->dragging) {
		if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
			plug->dragging = true;
			if (!plug->grid.tail) {
				stroke_list_add_row(&plug->stroke_arena, &plug->grid);
			}
		}
	} else {
		brush b = { .pos = mouse_2d_pos, .size = 2, .brush_color = RED };
		stroke_list_push(&plug->stroke_arena, plug->grid.tail, b);

		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			plug->dragging = false;
			stroke_list_add_row(&plug->stroke_arena, &plug->grid);
		}
	}

	if (IsKeyPressed(KEY_D) && !plug->dragging) {
		reset_strokes(&plug->stroke_arena, &plug->grid);
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
