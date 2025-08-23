#ifndef PLUG_H
#define PLUG_H

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
	Camera2D *camera;
	Arena stroke_arena;
	bool dragging;
	bool erasing;
	float eraser_radius;

	stroke_grid grid;
	point_buf points;

	Color brush_color;
	size_t color_index;
	Color palette[8];
	size_t palette_count;
	size_t swatch_size;
	size_t swatch_pad;
	size_t swatch_x0;
	size_t swatch_y0;

	void *permanent_storage;
	size_t permanent_storage_size;
} Plug;

typedef void (*plug_init_t) (Plug *plug);
typedef void (*plug_update_t)(Plug *plug);
typedef void (*plug_pre_reload_t)(Plug *plug);
typedef void (*plug_post_reload_t)(Plug *plug);

#endif /* PLUG_H */
