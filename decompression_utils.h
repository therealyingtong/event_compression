#ifndef _DECOMPRESSION_UTILS_H
#define _DECOMPRESSION_UTILS_H 1

#include "compression_config.h"
#include "math_utils.h"

unsigned long long read_bits_from_buffer(unsigned long long **current_word, long *bits_read, long *bits_read_in_buf, unsigned char bitwidth, unsigned char *prev_buf_leftover_bitwidth, unsigned char *overlap_bitwidth, unsigned long long *prev_buf_leftover, unsigned char *bufcounter){

	unsigned char word_offset = *bits_read % 64; // start_idx in word
	unsigned long long bitstring = 1;

	int bufsize = INBUFENTRIES * 8 * 8;

	if ((*bits_read_in_buf + bitwidth) >= bufsize){

		(*bufcounter)++;

		// overlap into next buffer
		*overlap_bitwidth = word_offset + bitwidth - 64;

		*prev_buf_leftover_bitwidth = bitwidth - *overlap_bitwidth;
		*prev_buf_leftover = read_bits_from_word(**current_word, 64 - word_offset, word_offset);

		*bits_read_in_buf = 0;
		*bits_read = *bits_read + 64 - word_offset;	

	} else if (word_offset + bitwidth >= 64){

		// overlap into next word
		unsigned char overlap_bitwidth = word_offset + bitwidth - 64;

		unsigned long long first_part_bitstring = read_bits_from_word(**current_word, 64 - word_offset, word_offset);

		(*current_word)++; // read next word
		// ll_to_bin(**current_word);
	
		unsigned long long second_part_bitstring = read_bits_from_word(**current_word, overlap_bitwidth, 0);

		bitstring = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;

		*bits_read = *bits_read + bitwidth;	
		*bits_read_in_buf = *bits_read_in_buf + bitwidth;

	} else {

		bitstring = read_bits_from_word(**current_word, bitwidth, word_offset);

		*bits_read = *bits_read + bitwidth;	
		*bits_read_in_buf = *bits_read_in_buf + bitwidth;

	}

	return bitstring;

}

#endif