#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "arena.h"
#include "plug.h"
#include "raylib.h"
#include "raymath.h"
#include "raylib_helpers.c"

#define ENTITY_SIZE 10000

void plug_init(Plug *plug)
{
	plug->permanent_storage = malloc(Gigabytes(2));
	if (!plug->permanent_storage) {
		printf("by more ram\n");
		exit(0);
	}
	plug->permanent_storage_size = Gigabytes(2);
	initialize_arena(&plug->world_arena, plug->permanent_storage_size, plug->permanent_storage);

	plug->camera = arena_push_struct(&plug->world_arena, Camera2D);

	// circular buffer for brushes
	plug->brushes = arena_push_struct(&plug->world_arena, brush_buffer);
	plug->brushes->data = arena_push_array(&plug->world_arena, ENTITY_SIZE, brush);
	plug->brushes->h.max_count = ENTITY_SIZE;

	plug->mode = arena_push_struct(&plug->world_arena, brush_kind);
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

static void draw_all_brushes(brush_buffer *buff)
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
	(void)plug;
	Vector2 mouse_pos = GetMousePosition();
	Vector2 mouse_2d_pos = GetScreenToWorld2D(GetMousePosition(), *plug->camera);
	mouse_stuff(plug->camera, &mouse_pos, &mouse_2d_pos);

	if (IsKeyPressed(KEY_R)) {
		*plug->mode = BRUSH_RECTANGLE;
	} else if (IsKeyPressed(KEY_C)) {
		*plug->mode = BRUSH_CIRCLE;
	}
	BeginDrawing();
	{
		ClearBackground(GetColor(0x151515FF));

		BeginMode2D(*plug->camera);
		{
			if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
				brush b = {};

				switch(*plug->mode) {
				case BRUSH_RECTANGLE:
					Rectangle rec = { .height = 10, .width = 10, .x = mouse_2d_pos.x, .y = mouse_2d_pos.y, };
					b = (brush) { .kind = BRUSH_RECTANGLE, .b_data = { .rec = rec }, .brush_color = RED, .draw_brush = &draw_rec };
					break;
				case BRUSH_CIRCLE:
					Circle circ = { .center = mouse_2d_pos, .radius = 5, };
					b = (brush) { .kind = BRUSH_CIRCLE, .b_data = { .circ = circ }, .brush_color = GREEN, .draw_brush = &draw_circ };
					break;
				case BRUSH_NONE:
				default:
					assert(0 && "unreachable");
					break;
				}

				hbb_circular_buffer_push(plug->brushes, b);
			}
			draw_all_brushes(plug->brushes);

			if (IsKeyPressed(KEY_D))
				hbb_circular_buffer_reset(plug->brushes);
		}
		EndMode2D();
	}
	EndDrawing();
}
