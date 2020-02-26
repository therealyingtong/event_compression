#ifndef _DECOMPRESSION_UTILS_H
#define _DECOMPRESSION_UTILS_H 1

#include <stdlib.h>
#include <stdio.h>
#include "math_utils.h"

long long decode_t_diff(char *t_diff_bitwidth, int *bits_read, long long *leftover_bits, char *leftover_bitwidth, int64_t *current_word, char *word_offset, int64_t *t_diff, int output_fd, char *new_buf, char* new_word){
	
	if (*word_offset + *t_diff_bitwidth > 63) {

		// we've finished reading the word before reading our whole t_diff. so we read whatever we can of the t_diff into leftover_bits, and rescue it in the next buffer

		printf("*word_offset + *t_diff_bitwidth > 64\n");
		printf("*word_offset: %d\n", *word_offset);
		printf("*t_diff_bitwidth: %d\n", *t_diff_bitwidth);

		*leftover_bitwidth = *t_diff_bitwidth + *word_offset - 64;

		char truncated_bitwidth = *t_diff_bitwidth - *leftover_bitwidth;
		long long truncated_bitmask = (((long long) 1 << truncated_bitwidth) - 1); // make bitmask for specified bitwidth
		long long truncated_t_diff = ((truncated_bitmask << *word_offset) & *current_word) >> *word_offset;
		*leftover_bits = truncated_t_diff;

		*word_offset = 0; // reset word_offset for new word
		*bits_read += *t_diff_bitwidth;
		*new_word = 1;

		printf("*word_offset: %d\n", *word_offset);
		printf("*t_diff_bitwidth: %d\n", *t_diff_bitwidth);

		return 0;

	} else {

		// the whole remaining part of the t_diff is contained within the word, so we can simply read t_diff from the last word_offset

		printf("*word_offset + *t_diff_bitwidth <= 63\n");
		printf("*word_offset: %d\n", *word_offset);
		printf("*t_diff_bitwidth: %d\n", *t_diff_bitwidth);

		char read_bitwidth = 0;
		if (*new_word){
			read_bitwidth = *leftover_bitwidth;
		} else {
			read_bitwidth = *t_diff_bitwidth;
		}

		long long t_diff_bitmask = ((long long) 1 << read_bitwidth) - 1; // make bitmask for specified bitwidth
		long long current_t_diff = ((t_diff_bitmask << *word_offset) & *current_word) >> *word_offset;

		// rescue leftover bits from previous word
		if (*leftover_bits){
			current_t_diff = (current_t_diff << *leftover_bitwidth) | *leftover_bits;
		}


		*t_diff = current_t_diff >> (64 - *t_diff_bitwidth); // write complete t_diff to t_diff
		*bits_read += *t_diff_bitwidth;

 		if (*word_offset + read_bitwidth == 63){
			 // edge case: we've reached exactly end of word
			 *new_word = 1;
			 *word_offset = 0;
		}

 		else if (*bits_read == INBUFENTRIES * 8){
			 // edge case: we've reached exactly end of buffer
			 *new_buf = 1;
			 *word_offset = 0;
		}

		else {
			*word_offset += *t_diff_bitwidth;
			*new_word = 0;
		}

		printf("*word_offset: %d\n", *word_offset);
		printf("*t_diff_bitwidth: %d\n", *t_diff_bitwidth);

		printf("\n\n t_diff:\n\n");
		ll_to_bin(*t_diff);

		return *t_diff;

	}

}

long long decode_large_t_diff(char *t_diff_bitwidth, int *bits_read, long long *leftover_bits, char *leftover_bitwidth, int64_t *current_word, char *word_offset, int64_t *t_diff, int output_fd, char *new_buf, char* new_word){

	printf("decode_large_t_diff");

	char *byte_width = 8;

	char large_t_diff_bitwidth = decode_t_diff(byte_width, bits_read, leftover_bits, leftover_bitwidth, current_word, word_offset, t_diff, output_fd, new_buf, new_word); // read new large bitwidth
	*t_diff_bitwidth = large_t_diff_bitwidth;
	decode_t_diff(t_diff_bitwidth, bits_read, leftover_bits, leftover_bitwidth, current_word, word_offset, t_diff, output_fd, new_buf, new_word);

	return 0;
}

#endif