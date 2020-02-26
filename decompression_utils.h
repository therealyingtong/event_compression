#ifndef _DECOMPRESSION_UTILS_H
#define _DECOMPRESSION_UTILS_H 1

#include "math_utils.h"

long long read_bits_from_buffer(long long *current_word, long *bits_read, char bitwidth){
	
	char word_offset = *bits_read % 64; // start_idx in word
	long long bitstring;

	if (word_offset + bitwidth > 64){
		//overlap into next word
		char overlap_bitwidth = word_offset + bitwidth - 64;
		long long first_part_bitstring = read_bits_from_word(*current_word, 64 - word_offset, word_offset);

		*current_word++; // read next word

		long long second_part_bitstring = read_bits_from_word(*current_word, overlap_bitwidth, 0);

		bitstring = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;

	} else {
		// directly read bitstring
		bitstring = read_bits_from_word(*current_word, bitwidth, word_offset);
	}

	*bits_read += bitwidth;
	return bitstring;
}

long long decode_t_diff(char *t_diff_bitwidth, long *bits_read, long long *current_word, int output_fd){

	long long t_diff = read_bits_from_buffer(current_word, bits_read, *t_diff_bitwidth);
	return t_diff;

}

long long decode_large_t_diff(char *t_diff_bitwidth, long *bits_read, long long *current_word, int output_fd){

	char large_t_diff_bitwidth = read_bits_from_buffer(current_word, bits_read, (char) 8); // read new large bitwidth

	*t_diff_bitwidth = large_t_diff_bitwidth;

	long long t_diff = read_bits_from_buffer(current_word, bits_read, *t_diff_bitwidth);
	return t_diff;

}

#endif