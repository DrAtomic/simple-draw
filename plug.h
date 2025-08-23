#ifndef PLUG_H
#define PLUG_H

#include <stdint.h>
#include "arena.h"
#include "raylib.h"

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

typedef struct brush {
	Vector2 b_data;
	Color brush_color;
	size_t size;
} brush;

struct hbb_node {
	struct hbb_node *next;
	brush el;
};

typedef struct stroke_list {
	struct hbb_node *root;
	struct stroke_list *down;
} stroke_list;

typedef struct {
	struct Arena world_arena;
	Camera2D *camera;
	struct Arena stroke_arena;
	bool dragging;

	stroke_list *strokes_head;
	stroke_list *strokes_tail;

	void *permanent_storage;
	size_t permanent_storage_size;
} Plug;

typedef void (*plug_init_t) (Plug *plug);
typedef void (*plug_update_t)(Plug *plug);
typedef void (*plug_pre_reload_t)(Plug *plug);
typedef void (*plug_post_reload_t)(Plug *plug);

#endif /* PLUG_H */
