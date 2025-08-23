#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "arena.h"

void *_arena_push(Arena *arena, size_t size, bool clear_to_zero)
{
	assert((arena->used + size) <= arena->size);

	void *ret = arena->base + arena->used;
	arena->used += size;
	if (clear_to_zero)
		memset(ret, 0, size);

	return ret;
}

void initialize_arena(Arena *arena, size_t size, uint8_t *base)
{
	arena->size = size;
	arena->base = base;
	arena->used = 0;
}
