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
		Rectangle rec = buff->data[i];
		DrawRectangleRec(rec, SKYBLUE);
	}
}

void plug_update(Plug *plug)
{
	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), plug->camera);
	mouse_stuff(plug, &mouse_pos, &mouse_2d_pos);

	BeginDrawing();
	{
		ClearBackground(GetColor(0x151515FF));

		BeginMode2D(plug->camera);
		{
			if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
				Rectangle rec = {
					.height = 10,
					.width = 10,
					.x = mouse_2d_pos.x,
					.y = mouse_2d_pos.y,
				};
				hbb_circular_buffer_push(&plug->recs, rec);
			}
			draw_all_recs(&plug->recs);

			if (IsKeyPressed(KEY_D))
				hbb_circular_buffer_init(&plug->recs, REC_SIZE);
		}
		EndMode2D();
	}
	EndDrawing();
}
