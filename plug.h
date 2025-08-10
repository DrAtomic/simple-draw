#ifndef PLUG_H
#define PLUG_H

#include "raylib.h"
#include "hbb_circular_queue.h"

typedef enum {
	BRUSH_RECTANGLE,
	BRUSH_CIRCLE,
	BRUSH_NONE
} brush_kind;

typedef struct {
	Vector2 center;
	float radius;
} Circle;

typedef union {
	Rectangle rec;
	Circle circ;
} brush_data;

typedef struct brush {
	brush_kind kind;
	brush_data b_data;
	Color brush_color;
	void (*draw_brush)(const struct brush *);
} brush;

typedef struct circular_buffer {
	hbb_circular_buffer_handler h;
	brush *data;
} circular_buffer;

typedef struct {
	Camera2D camera;
	circular_buffer brushes;
	brush_kind mode;
} Plug;

typedef void (*plug_init_t) (Plug *plug);
typedef void (*plug_update_t)(Plug *plug);
typedef void (*plug_pre_reload_t)(Plug *plug);
typedef void (*plug_post_reload_t)(Plug *plug);

#endif /* PLUG_H */
