/*
 * This file was transplanted with slight modifications from Linux sources
 * (fs/cifs/md5.h) into U-Boot by Bartlomiej Sieka <tur@semihalf.com>.
 */

#ifndef _MD5_H
#define _MD5_H

#include "compiler.h"

struct MD5Context {
/*
 *  a progress message will be print each step.
 *  0 = no printing,
 *  must be a  product of 64 (i.e. 64, 128, ..., 1024*1024, ...)
 */
	size_t progress_print_step;
	__u32  buf[4];
	__u32  bits[2];
	union {
		unsigned char in[64];
		__u32 in32[16];
	};
};

/*
 * Calculate and store in 'output' the MD5 digest of 'len' bytes at
 * 'input'. 'output' must have enough space to hold 16 bytes.
 */
void md5 (unsigned char *input, int len, unsigned char output[16]);

/*
 * Calculate and store in 'output' the MD5 digest of 'len' bytes at 'input'.
 * 'output' must have enough space to hold 16 bytes. If 'chunk' Trigger the
 * watchdog every 'chunk_sz' bytes of input processed.
 */
void md5_wd (unsigned char *input, int len, unsigned char output[16],
		unsigned int chunk_sz);

/* same function as md5_wd, only with progress messages prints */
void md5_wd_prog_msg (unsigned char *input, int len, unsigned char output[16],
                unsigned int chunk_sz, size_t prog_msg_step);

#endif /* _MD5_H */
