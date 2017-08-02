/*
 *  relocate - common relocation function for ARM U-Boot
 *
 *  Copyright (c) 2013  Albert ARIBAUD <albert.u.boot@aribaud.net>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <linux/linkage.h>

#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
/*
 * Default/weak exception vectors relocation routine
 *
 * This routine covers the standard ARM cases: normal (0x00000000),
 * high (0xffff0000) and VBAR. SoCs which do not comply with any of
 * the standard cases must provide their own, strong, version.
 */

	.section	.text.relocate_vectors,"ax",%progbits
	.weak		relocate_vectors

ENTRY(relocate_vectors)

#ifdef CONFIG_HAS_VBAR
	/*
	 * If the ARM processor has the security extensions,
	 * use VBAR to relocate the exception vectors.
	 */
	ldr	r0, [r9, #GD_RELOCADDR]	/* r0 = gd->relocaddr */
	mrs	r1, cpsr
	and	r1, r1, #0x1f		/* mask mode bits */
	teq	r1, #0x1a		/* test for HYP mode */
	beq	1f
	mcr	p15, 0, r0, c12, c0, 0  /* Set VBAR */
	b	2f
1:
	mcr	p15, 4, r0, c12, c0, 0  /* Set HVBAR */
	ldr	r2, =_data_abort
	ldr	r1, =data_abort_hyp
	str	r1, [r2]
2:
#else
	/*
	 * Copy the relocated exception vectors to the
	 * correct address
	 * CP15 c1 V bit gives us the location of the vectors:
	 * 0x00000000 or 0xFFFF0000.
	 */
	ldr	r0, [r9, #GD_RELOCADDR]	/* r0 = gd->relocaddr */
	mrc	p15, 0, r2, c1, c0, 0	/* V bit (bit[13]) in CP15 c1 */
	ands	r2, r2, #(1 << 13)
	ldreq	r1, =0x00000000		/* If V=0 */
	ldrne	r1, =0xFFFF0000		/* If V=1 */
	ldmia	r0!, {r2-r8,r10}
	stmia	r1!, {r2-r8,r10}
	ldmia	r0!, {r2-r8,r10}
	stmia	r1!, {r2-r8,r10}
#endif
	bx	lr

ENDPROC(relocate_vectors)
#endif

/*
 * void relocate_code(addr_moni)
 *
 * This function relocates the monitor code.
 *
 * NOTE:
 * To prevent the code below from containing references with an R_ARM_ABS32
 * relocation record type, we never refer to linker-defined symbols directly.
 * Instead, we declare literals which contain their relative location with
 * respect to relocate_code, and at run time, add relocate_code back to them.
 */

ENTRY(relocate_code)
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	mov	r6,	r0		/* save addr of destination */
	ldr	r0,	=_start		/* r0 <- SRC &_start */
	subs	r9,	r6,	r0		/* r9 <- relocation offset */
#endif
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ldr	r1, =__image_copy_start	/* r1 <- SRC &__image_copy_start */
	subs	r4, r0, r1		/* r4 <- relocation offset */
#endif
	beq	relocate_done		/* skip relocation */
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
	mov	r1,	r6		/* r1 <- scratch for copy loop */
	adr	r7,	relocate_code		/* r7 <- SRC &relocate_code */
#endif
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	ldr	r3,	_image_copy_end_ofs	/* r3 <- __image_copy_end local ofs */
	add	r2, r7, r3		/* r2 <- SRC &__image_copy_end */
#endif
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ldr	r2, =__image_copy_end	/* r2 <- SRC &__image_copy_end */
#endif

copy_loop:
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ldmia	r1!, {r10-r11}		/* copy from source address [r1]    */
	stmia	r0!, {r10-r11}		/* copy to   target address [r0]    */
	cmp	r1, r2			/* until source end address [r2]    */
#endif
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
	ldmia	r0!, {r10-r11}		/* copy from source address [r0]    */
	stmia	r1!, {r10-r11}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end address [r2]    */
#endif
	blo	copy_loop

	/*
	 * fix .rel.dyn relocations
	 */
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	ldr	r10, _dynsym_start_ofs	/* r10 <- __dynsym_start local ofs */
	add	r10, r10, r7	/* r10 <- SRC &__dynsym_start */
	ldr	r2, _rel_dyn_start_ofs	/* r2 <- __rel_dyn_start local ofs */
	add	r2, r2, r7		/* r2 <- SRC &__rel_dyn_start */
	ldr	r3, _rel_dyn_end_ofs	/* r3 <- __rel_dyn_end local ofs */
	add	r3, r3, r7		/* r3 <- SRC &__rel_dyn_end */
#endif
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ldr	r2, =__rel_dyn_start	/* r2 <- SRC &__rel_dyn_start */
	ldr	r3, =__rel_dyn_end	/* r3 <- SRC &__rel_dyn_end */
#endif
fixloop:
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	ldmia	r2!, {r0-r1}		/* (r0,r1) <- (SRC location,fixup) */
	and	r1, r1, #0xff
	cmp	r1, #23			/* relative fixup? */
	bne	fixnext

	/* relative fix: increase location by offset */
	add	r0, r0, r4
	ldr	r1, [r0]
	add	r1, r1, r4
	str	r1, [r0]
#endif
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
	ldr	r0, [r2]		/* r0 <- SRC location to fix up */
	add	r0, r0, r9		/* r0 <- DST location to fix up */
	ldr	r1, [r2, #4]
	and	r7, r1, #0xff
	cmp	r7, #23		/* relative fixup? */
	beq	fixrel
	cmp	r7, #2		/* absolute fixup? */
	beq	fixabs
	/* ignore unknown type of fixup */
	b	fixnext
#endif
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
fixabs:
	/* absolute fix: set location to (offset) symbol value */
	mov	r1, r1, LSR #4		/* r1 <- symbol index in .dynsym */
	add	r1, r10, r1		/* r1 <- address of symbol in table */
	ldr	r1, [r1, #4]		/* r1 <- symbol value */
	add	r1, r1, r9		/* r1 <- relocated sym addr */
	b	fixnext
#endif
fixrel:
#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
	ldr	r1, [r0]
	add	r1, r1, r9
#endif
#if defined(CONFIG_HW29765235P0P512P1024P4X4P4X4) || \
    defined(CONFIG_HW29765294P0P128P512P8X8P4X4)
	add	r0, r0, r4
	ldr	r1, [r0]
	add	r1, r1, r4
	str	r1, [r0]
#endif
fixnext:
#if defined(CONFIG_HW29764958P0P128P512P3X3P4X4)
	str	r1, [r0]
	add	r2, r2, #8		/* each rel.dyn entry is 8 bytes */
#endif
	cmp	r2, r3
	blo	fixloop

relocate_done:

#ifdef __XSCALE__
	/*
	 * On xscale, icache must be invalidated and write buffers drained,
	 * even with cache disabled - 4.2.7 of xscale core developer's manual
	 */
	mcr	p15, 0, r0, c7, c7, 0	/* invalidate icache */
	mcr	p15, 0, r0, c7, c10, 4	/* drain write buffer */
#endif

	/* ARMv4- don't know bx lr but the assembler fails to see that */

#ifdef __ARM_ARCH_4__
	mov	pc, lr
#else
	bx	lr
#endif

#ifdef CONFIG_HW29764958P0P128P512P3X3P4X4
_image_copy_end_ofs:
	.word __image_copy_end - relocate_code
_rel_dyn_start_ofs:
	.word __rel_dyn_start - relocate_code
_rel_dyn_end_ofs:
	.word __rel_dyn_end - relocate_code
_dynsym_start_ofs:
	.word __dynsym_start - relocate_code
#endif

ENDPROC(relocate_code)
