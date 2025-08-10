#include "plug.h"
#include "raylib.h"
#include "raymath.h"

#define REC_SIZE 10000

void plug_init(Plug *plug)
{
	plug->camera = (Camera2D){0};
	hbb_circular_buffer_init(&plug->recs, REC_SIZE);
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

void draw_all_recs(circular_buffer *buff)
{
	for (size_t i = 0; i < buff->h.current_count; i++) {
		if (buff->data[i].kind == RECTANGLE) {
			Rectangle rec = buff->data[i].b_data.rec;
			DrawRectangleRec(rec, SKYBLUE);
		} else if (buff->data[i].kind == CIRCLE) {
			Circle circ = buff->data[i].b_data.circ;
			DrawCircleV(circ.center, circ.radius, RED);
		}
	}
}

void plug_update(Plug *plug)
{
	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), plug->camera);
	bool circ_mode = false;
	mouse_stuff(plug, &mouse_pos, &mouse_2d_pos);

	BeginDrawing();
	{
		ClearBackground(GetColor(0x151515FF));

		BeginMode2D(plug->camera);
		{
			if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
				brush b = {};
				if (circ_mode == false) {
					Rectangle rec = {
						.height = 10,
						.width = 10,
						.x = mouse_2d_pos.x,
						.y = mouse_2d_pos.y,
					};
					b = (brush) {.kind = RECTANGLE, .b_data.rec = rec};
				} else if (circ_mode == true) {
					Circle circ = {
						.center = mouse_2d_pos,
						.radius = 5,
					};
					b = (brush) {.kind = CIRCLE, .b_data.circ = circ};
				}
				hbb_circular_buffer_push(&plug->recs, b);
			}
			draw_all_recs(&plug->recs);

			if (IsKeyPressed(KEY_D))
				hbb_circular_buffer_reset(&plug->recs);
		}
		EndMode2D();
	}
	EndDrawing();
}
