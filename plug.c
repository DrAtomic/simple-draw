#include <assert.h>

#include "plug.h"
#include "raylib.h"
#include "raymath.h"

#define ENTITY_SIZE 10000

void plug_init(Plug *plug)
{
	plug->camera = (Camera2D){0};
	hbb_circular_buffer_init(&plug->brushes, ENTITY_SIZE);
	plug->camera.zoom = 1.0f;
}

void plug_pre_reload(Plug *plug)
{
	(void)plug;
}

void plug_post_reload(Plug *plug)
{
	(void)plug;
}

static void mouse_stuff(Plug *plug, Vector2 *mouse_pos, Vector2 *mouse_2d_pos)
{
	*mouse_pos = GetMousePosition();
	*mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), plug->camera);
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		Vector2 delta = GetMouseDelta();
		delta = Vector2Scale(delta, -1.0f / plug->camera.zoom);
		plug->camera.target = Vector2Add(plug->camera.target, delta);
	}

	float wheel = GetMouseWheelMove();
	if (wheel != 0) {
		plug->camera.offset = *mouse_pos;
		plug->camera.target = *mouse_2d_pos;

		float scale_factor = 1.0f + (0.25f * fabsf(wheel));
		if (wheel < 0)
			scale_factor = 1.0f / scale_factor;
		plug->camera.zoom = Clamp(plug->camera.zoom * scale_factor, 0.125f, 64.0f);
	}
}

static void draw_all_brushes(circular_buffer *buff)
{
	for (size_t i = 0; i < buff->h.current_count; i++) {
		brush *cur_brush = &buff->data[i];
		cur_brush->draw_brush(cur_brush);
	}
}

static void draw_rec(const struct brush *b)
{
	assert(b->kind == BRUSH_RECTANGLE);
	Rectangle rec = b->b_data.rec;
	Color col = b->brush_color;
	DrawRectangleRec(rec, col);
}

static void draw_circ(const struct brush *b)
{
	assert(b->kind == BRUSH_CIRCLE);
	Circle circ = b->b_data.circ;
	Color col = b->brush_color;
	DrawCircleV(circ.center, circ.radius, col);
}

void plug_update(Plug *plug)
{
	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), plug->camera);
	mouse_stuff(plug, &mouse_pos, &mouse_2d_pos);

	if (IsKeyPressed(KEY_R)) {
		plug->mode = BRUSH_RECTANGLE;
	} else if (IsKeyPressed(KEY_C)) {
		plug->mode = BRUSH_CIRCLE;
	}
	BeginDrawing();
	{
		ClearBackground(GetColor(0x151515FF));

		BeginMode2D(plug->camera);
		{
			if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
				brush b = {};
				if (plug->mode == BRUSH_RECTANGLE) {
					Rectangle rec = { .height = 10, .width = 10, .x = mouse_2d_pos.x, .y = mouse_2d_pos.y, };
					b = (brush) { .kind = BRUSH_RECTANGLE, .b_data = { .rec = rec }, .brush_color = RED, .draw_brush = &draw_rec };
				} else if (plug->mode == BRUSH_CIRCLE) {
					Circle circ = { .center = mouse_2d_pos, .radius = 5, };
					b = (brush) { .kind = BRUSH_CIRCLE, .b_data = { .circ = circ }, .brush_color = GREEN, .draw_brush = &draw_circ };
				}
				hbb_circular_buffer_push(&plug->brushes, b);
			}
			draw_all_brushes(&plug->brushes);

			if (IsKeyPressed(KEY_D))
				hbb_circular_buffer_reset(&plug->brushes);
		}
		EndMode2D();
	}
	EndDrawing();
}
