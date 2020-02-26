#ifndef _DECOMPRESSION_UTILS_H
#define _DECOMPRESSION_UTILS_H 1

#include <stdlib.h>
#include <stdio.h>
#include "math_utils.h"

long long decode_t_diff(char *t_diff_bitwidth, int *bits_read, int64_t *current_word, int output_fd){

	long long t_diff = read_bits_from_buffer(current_word, bits_read, *t_diff_bitwidth);
	return t_diff;

}

long long decode_large_t_diff(char *t_diff_bitwidth, int *bits_read, int64_t *current_word, int output_fd){

	printf("decode_large_t_diff");

	char large_t_diff_bitwidth = read_bits_from_buffer(current_word, bits_read, (char) 8); // read new large bitwidth

	*t_diff_bitwidth = large_t_diff_bitwidth;

	long long t_diff = read_bits_from_buffer(current_word, bits_read, *t_diff_bitwidth);
	return t_diff;

}

#endif