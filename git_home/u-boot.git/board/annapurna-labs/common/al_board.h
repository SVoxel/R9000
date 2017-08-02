#ifndef __AL_BOARD_H__
#define __AL_BOARD_H__

#include "al_hal_bootstrap.h"

/**
 * Get bootstrap value
 *
 * If devices already initialized, use global precalculated values,
 * Otherwise, calculate on stack.
 *
 * @param	b_ptr
 *		a pointer to a bootstrap struct
 *
 * @returns	0 if successfull
 *		<0 otherwise
 */
int bootstrap_get(struct al_bootstrap *b_ptr);

/**
 * Read and parse bootstrap value
 *
 * @param	b_ptr
 *		a pointer to a bootstrap struct
 *
 * @returns	0 if successfull
 *		<0 otherwise
 */
int bootstrap_read_and_parse(struct al_bootstrap *b_ptr);

#endif

