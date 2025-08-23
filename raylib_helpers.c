#include "plug.h"
#include "raylib.h"
#include "raymath.h"

static void mouse_and_camera_stuff(Camera2D *camera, Vector2 *mouse_pos, Vector2 *mouse_2d_pos)
{
	*mouse_pos = GetMousePosition();
	*mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), *camera);
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		Vector2 delta = GetMouseDelta();
		delta = Vector2Scale(delta, -1.0f / camera->zoom);
		camera->target = Vector2Add(camera->target, delta);
	}

	float wheel = GetMouseWheelMove();
	if (wheel != 0) {
		camera->offset = *mouse_pos;
		camera->target = *mouse_2d_pos;

		float scale_factor = 1.0f + (0.25f * fabsf(wheel));
		if (wheel < 0)
			scale_factor = 1.0f / scale_factor;
		camera->zoom = Clamp(camera->zoom * scale_factor, 0.125f, 64.0f);
	}
}
