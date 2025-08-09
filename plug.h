#ifndef PLUG_H
#define PLUG_H

#include "raylib.h"
#include "hbb_circular_queue.h"

typedef struct circular_buffer {
	hbb_circular_buffer_handler h;
	Rectangle *data;
} circular_buffer;

typedef struct {
	Camera2D camera;
	circular_buffer recs;
} Plug;

typedef void (*plug_init_t) (Plug *plug);
typedef void (*plug_update_t)(Plug *plug);
typedef void (*plug_pre_reload_t)(Plug *plug);
typedef void (*plug_post_reload_t)(Plug *plug);

#endif /* PLUG_H */
