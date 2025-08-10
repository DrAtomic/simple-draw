#ifndef HBB_CIRCULAR_BUFFER_H
#define HBB_CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/**
 * @brief      circular buffer handler
 *
 * @details    this handles the pointers in the data structure for the macros to work
 *             the handler needs to be called h in the data structure using it
 */
typedef struct {
	size_t head;
	size_t tail;
	size_t max_count;
	size_t current_count;
	bool full;
} hbb_circular_buffer_handler;

#ifdef __cplusplus
#define cast_ptr(ptr) (decltype(ptr))
#else  /* __cplusplus */
#define cast_ptr(...)
#endif /* __cplusplus */

#define hbb_circular_buffer_reset(buff)	\
do {						\
	(buff)->h.head = 0;			\
	(buff)->h.tail = 0;			\
	(buff)->h.current_count = 0;		\
	(buff)->h.full = false;		\
} while (0)

#define hbb_circular_buffer_init(buff, size)								\
do {													\
	hbb_circular_buffer_reset(buff);								\
	(buff)->h.max_count = size;									\
	(buff)->data = cast_ptr((buff)->data)realloc((buff)->data, sizeof(*(buff)->data) * size);	\
} while (0)

/**
 * @brief      put thing into buffer
 *
 * @param      buff
 * @param      thing
 *
 * @return     return void
 */
#define hbb_circular_buffer_push(buff, thing)					\
do {										\
	(buff)->data[(buff)->h.head] = (thing);				\
	(buff)->h.head = ((buff)->h.head + 1) % (buff)->h.max_count;		\
	if ((buff)->h.full)							\
		(buff)->h.tail = ((buff)->h.tail + 1) % (buff)->h.max_count;	\
	else									\
		(buff)->h.current_count++;					\
	(buff)->h.full = ((buff)->h.head == (buff)->h.tail);			\
} while (0)

/**
 * @brief      pop value into thing
 *
 * @details    this checks to see if the handler is not empty
 *             if not empty assign thing to the value at the tail
 *             else assign thing to 0
 *
 * @param      buff
 * @param      address of thing
 *
 * @return     return void
 */
#define hbb_circular_buffer_zero(T) ((T){0})
#define hbb_circular_buffer_pop(buff, thing)				\
do {									\
	*(thing) = hbb_circular_buffer_zero(__typeof__(*(thing)));	\
									\
	if (((buff)->h.full || (buff)->h.head != (buff)->h.tail)) {	\
		*(thing) = (buff)->data[(buff)->h.tail];		\
		(buff)->h.full = false;				\
		(buff)->h.current_count--;				\
		if (++(buff)->h.tail == (buff)->h.max_count)		\
			(buff)->h.tail = 0;				\
	}								\
} while (0)

#define hbb_circular_buffer_free(buff) free((buff)->data);

#endif /* HBB_CIRCULAR_BUFFER_H */
