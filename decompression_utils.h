#ifndef _DECOMPRESSION_UTILS_H
#define _DECOMPRESSION_UTILS_H 1

#include "compression_config.h"
#include "math_utils.h"

unsigned long long read_bits_from_buffer(unsigned long long **current_word, long *bits_read, long *bits_read_in_buf, char bitwidth, char *prev_buf_leftover_bitwidth, unsigned long long *prev_buf_leftover){

	char word_offset = *bits_read % 64; // start_idx in word
	unsigned long long bitstring = 0;

	int bufsize = INBUFENTRIES * 8 * 8;

	if ((*bits_read_in_buf + bitwidth) >= bufsize){
		// overlap into next buffer
		char overlap_bitwidth = word_offset + bitwidth - 64;
		*bits_read_in_buf = overlap_bitwidth;

		*prev_buf_leftover_bitwidth = bitwidth - overlap_bitwidth;
		*prev_buf_leftover = read_bits_from_word(**current_word, 64 - word_offset, word_offset);

	} else if (word_offset + bitwidth >= 64){
		// overlap into next word
		char overlap_bitwidth = word_offset + bitwidth - 64;

		unsigned long long first_part_bitstring = read_bits_from_word(**current_word, 64 - word_offset, word_offset);

		(*current_word)++; // read next word
		
		unsigned long long second_part_bitstring = read_bits_from_word(**current_word, overlap_bitwidth, 0);

		bitstring = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;

	} else {
		// directly read bitstring
		bitstring = read_bits_from_word(**current_word, bitwidth, word_offset);
	}

	*bits_read = *bits_read + bitwidth;	
	*bits_read_in_buf = *bits_read_in_buf + bitwidth;
	return bitstring;
}

unsigned long long decode_t_diff(char *t_diff_bitwidth, long *bits_read, long *bits_read_in_buf, unsigned long long **current_word, int output_fd,  char *prev_buf_leftover_bitwidth, unsigned long long *prev_buf_leftover){

	if (*prev_buf_leftover_bitwidth){
		// there are leftover bits from the previous buffer
		char word_offset = *bits_read % 64; // how many bits to read in new buffer
		unsigned long long first_part_bitstring = *prev_buf_leftover;
		unsigned long long second_part_bitstring = read_bits_from_word(**current_word, word_offset, 0);
		unsigned long long bitstring = (first_part_bitstring << word_offset) | second_part_bitstring;
		*prev_buf_leftover_bitwidth = 0;
	} 
	unsigned long long t_diff = read_bits_from_buffer(current_word, bits_read, bits_read_in_buf, *t_diff_bitwidth, prev_buf_leftover_bitwidth, prev_buf_leftover);

	return t_diff;

}

unsigned long long decode_large_t_diff(char *t_diff_bitwidth, long *bits_read, long *bits_read_in_buf, unsigned long long **current_word, int output_fd,  char *prev_buf_leftover_bitwidth, unsigned long long *prev_buf_leftover){


	if (*prev_buf_leftover_bitwidth){
		// there are leftover bits from the previous buffer
		char word_offset = *bits_read % 64; // how many bits to read in new buffer
		unsigned long long first_part_bitstring = *prev_buf_leftover;
		unsigned long long second_part_bitstring = read_bits_from_word(**current_word, word_offset, 0);
		unsigned long long bitstring = (first_part_bitstring << word_offset) | second_part_bitstring;
		ll_to_bin(bitstring);
		*prev_buf_leftover_bitwidth = 0;

	} 

	char large_t_diff_bitwidth = read_bits_from_buffer(current_word, bits_read, bits_read_in_buf, (char) 8, prev_buf_leftover_bitwidth, prev_buf_leftover); // read new large bitwidth

	*t_diff_bitwidth = large_t_diff_bitwidth;

	unsigned long long t_diff = read_bits_from_buffer(current_word, bits_read, bits_read_in_buf, *t_diff_bitwidth, prev_buf_leftover_bitwidth, prev_buf_leftover);

	return t_diff;


}

#endif