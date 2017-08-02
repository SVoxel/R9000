/*
 */

#include <config.h>
#include <version.h>
#include <linux/linkage.h>

	.align	5

/* void cycle_delay(unsigned int num_cycles) */
ENTRY(cycle_delay)
1:
	cmp	r0, #0
	beq	2f
	sub	r0, #1
	b	1b
2:
	mov		pc, lr
ENDPROC(cycle_delay)

