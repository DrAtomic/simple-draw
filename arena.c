#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "arena.h"

void *_arena_push(struct Arena *arena, size_t size, bool clearToZero)
{
	assert((arena->used + size) <= arena->size);

	void *ret = arena->base + arena->used;
	arena->used += size;
	if (clearToZero)
		memset(ret, 0, size);

	return ret;
}

void initialize_arena(struct Arena *arena, size_t size, uint8_t *base)
{
	arena->size = size;
	arena->base = base;
	arena->used = 0;
}
