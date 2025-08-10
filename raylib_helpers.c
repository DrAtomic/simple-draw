#include "plug.h"
#include "raylib.h"
#include "raymath.h"

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
