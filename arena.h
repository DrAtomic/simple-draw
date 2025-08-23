#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Arena {
	size_t size;
	size_t used;
	void *base;
};

void initialize_arena(struct Arena *arena, size_t size, uint8_t *base);
void *_arena_push(struct Arena *arena, size_t size, bool clearToZero);

#define arena_push_struct(arena, type) (type *)_arena_push(arena, sizeof(type), true)
#define arena_push_array(arena, count, type) (type *)_arena_push(arena, (count) * sizeof(type), true)
#define arena_push(arena, size) _arena_push(arena, size, true);

#endif /* ARENA_H */
