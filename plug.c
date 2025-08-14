#include "stdio.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "plug.h"
#include "raylib.h"
#include "raymath.h"
#include "raylib_helpers.c"

#define ENTITY_SIZE 10000

#define DEFAULT_ALIGNMENT (2*sizeof(void *))
void *_ArenaPush(struct Arena *arena, uint64_t size, bool clearToZero)
{
	void *ret = arena->base + arena->used;
	arena->used += size;
	if (clearToZero)
		memset(ret, 0, size);

	return ret;
}

void initialize_arena(struct Arena *arena, uint64_t size, uint8_t *base)
{
	arena->size = size;
	arena->base = base;
	arena->used = 0;
}

#define ArenaPushStruct(arena, type) (type *)_ArenaPush(arena, sizeof(type), true)
#define ArenaPush(arena, size) _ArenaPush(arena, size, true);
void plug_init(Plug *plug)
{
	plug->permanent_storage = malloc(Gigabytes(2));
	if (!plug->permanent_storage) {
		printf("by more ram\n");
		exit(0);
	}
	plug->permanent_storage_size = Gigabytes(2);
	initialize_arena(&plug->world_arena, plug->permanent_storage_size, plug->permanent_storage);

	plug->camera = ArenaPushStruct(&plug->world_arena, Camera2D);
	plug->brushes = ArenaPushStruct(&plug->world_arena, circular_buffer);
	hbb_circular_buffer_init(plug->brushes, ENTITY_SIZE);
	plug->mode = ArenaPushStruct(&plug->world_arena, brush_kind);
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
