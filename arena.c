#include <string.h>
#include <stdint.h>

#include "arena.h"

#define DEFAULT_ALIGNMENT (2*sizeof(void *))
void *_arena_push(struct Arena *arena, size_t size, bool clearToZero)
{
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
