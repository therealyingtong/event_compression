#ifndef _COMPRESSION_UTILS_H
#define _COMPRESSION_UTILS_H 1

#include "compression_config.h"
#include "compression_structs.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "math_utils.h"


int encode_t_diff(long long t_diff, char t_diff_bitwidth, unsigned int *outbuf2, int *outbuf2_offset, int64_t *sendword2, int output_fd2){

	// returns 1 if sendword2 is full
	// returns 0 otherwise

	if (t_diff_bitwidth + *outbuf2_offset > 64){

		int padding_to_64 = 64 - t_diff_bitwidth;
		int overflow_bitwidth = t_diff_bitwidth + *outbuf2_offset - 64;
		// 111... for leftmost overflow bits
		long long overflow_bits = t_diff >> (64 - overflow_bitwidth);

		// printf("outbuf2:\n");
		// ll_to_bin(*outbuf2);

		// printf("t_diff << *outbuf2_offset:\n");
		// ll_to_bin(t_diff << *outbuf2_offset);

		// write to sendword2
		*sendword2 = *outbuf2 | (t_diff << *outbuf2_offset);

		// save overflow bits in outbuf2
		*outbuf2 = overflow_bits;

		// increase outbuf2_offset pointer
		*outbuf2_offset = overflow_bitwidth;

		write(output_fd2, sendword2, 8);
		printf("sendword2: ");
		ll_to_bin(*sendword2);

		return 1;

	} else {

		// left-append t_diff to outbuf2 at outbuf2_offset
		*outbuf2 = *outbuf2 | (t_diff << *outbuf2_offset);

		// increase outbuf2_offset pointer
		*outbuf2_offset += t_diff_bitwidth;

		return 0;

	}

}

int encode_large_t_diff(long long t_diff, char t_diff_bitwidth, char large_t_diff_bitwidth, unsigned int *outbuf2, int *outbuf2_offset, int64_t *sendword2, int output_fd2){

	// write string of zeros of width t_diff_bitwidth
	encode_t_diff(0, t_diff_bitwidth,  outbuf2, outbuf2_offset, sendword2, output_fd2);

	// write large_t_diff_bitwidth in next byte
	encode_t_diff(large_t_diff_bitwidth, 8,  outbuf2, outbuf2_offset, sendword2, output_fd2);

	// write t_diff in word of width large_t_diff_bitwidth
	encode_t_diff(t_diff, large_t_diff_bitwidth,  outbuf2, outbuf2_offset, sendword2, output_fd2);

}



#endif