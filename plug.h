#ifndef PLUG_H
#define PLUG_H

#include <stdbool.h>
#include <stdint.h>
#include "arena.h"
#include "raylib.h"

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

typedef struct {
	Vector2 pos;
	float size;
	Color brush_color;
} brush_pt;

typedef struct {
	brush_pt *data;
	size_t count;
	size_t cap;
} point_buf;

typedef struct stroke_list {
	size_t start;
	size_t count;
	struct stroke_list *down;
} stroke_list;

typedef struct stroke_grid {
	stroke_list *head;
	stroke_list *tail;
} stroke_grid;

typedef struct {
	Arena world_arena;
	Arena stroke_arena;
	Arena erase_arena;
	Camera2D *camera;
	bool dragging;
	bool erasing;

	stroke_grid grid;
	point_buf points;

	Color brush_color;
	float brush_size;

	Rectangle brush_size_slider;
	float brush_min;
	float brush_max;

	void *permanent_storage;
	size_t permanent_storage_size;

	Texture2D wheel_tex;
	size_t wheel_diam;
	Vector2 wheel_pos;

	float color_wheel_hue;
	float color_wheel_sat;
	float color_wheel_val;
	float color_wheel_alpha;

	Rectangle color_wheel_val_slider;
	bool color_wheel_picker_open;
	Rectangle color_wheel_picker_btn;
} Plug;

typedef void (*plug_init_t) (Plug *plug);
typedef void (*plug_update_t)(Plug *plug);
typedef void (*plug_pre_reload_t)(Plug *plug);
typedef void (*plug_post_reload_t)(Plug *plug);

#endif /* PLUG_H */
