#ifndef ARENA_H
#define ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Arena {
	size_t size;
	size_t used;
	void *base;
} Arena;

void initialize_arena(Arena *arena, size_t size, uint8_t *base);
void *_arena_push(Arena *arena, size_t size, bool clear_to_zero);

#define arena_push_struct(arena, type) _arena_push(arena, sizeof(type), true)
#define arena_push_array(arena, count, type) _arena_push(arena, (count) * sizeof(type), true)
#define arena_push(arena, size) _arena_push(arena, size, true);

#endif /* ARENA_H */
