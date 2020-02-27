#ifndef _COMPRESSION_UTILS_H
#define _COMPRESSION_UTILS_H 1

#include "math_utils.h"
#include <unistd.h>

unsigned long long write_bits_to_buffer(unsigned long long bitstring, unsigned long long *buffer, long *bits_written, char bitwidth){

	char buffer_offset = *bits_written % 64;
	*bits_written += bitwidth;

	if (buffer_offset + bitwidth > 64){
		// fill word and write overlap into buffer
		char overlap_bitwidth = buffer_offset + bitwidth - 64;

		// printf("\n\n\n\n\n");

		// printf("bitstring:");
		// ll_to_bin(bitstring);

		unsigned long long first_part_bitstring = read_bits_from_word(bitstring, bitwidth - overlap_bitwidth, 64 - bitwidth);

		// printf("first_part_bitstring:");
		// ll_to_bin(first_part_bitstring);


		unsigned long long second_part_bitstring = read_bits_from_word(bitstring, overlap_bitwidth, 64 - overlap_bitwidth);

		// printf("second_part_bitstring:");
		// ll_to_bin(second_part_bitstring);

		// printf("*buffer:");
		// ll_to_bin(*buffer);

		unsigned long long word = *buffer | first_part_bitstring ;
		*buffer = second_part_bitstring << (64 - overlap_bitwidth);

		// printf("*buffer:");
		// ll_to_bin(*buffer);

		// printf("word:\n");
		// ll_to_bin(word);

		return word;

	} else {
		// write into buffer

		printf("write into buffer without making new word\n");
		*buffer = *buffer | (bitstring << (64 - bitwidth - buffer_offset)) ;
		return 0;

	}

}

unsigned long long encode_t_diff(unsigned long long t_diff, char t_diff_bitwidth, unsigned long long *outbuf2, long *bits_written, int output_fd2){

	unsigned long long word = write_bits_to_buffer(t_diff, outbuf2, bits_written, t_diff_bitwidth);

	if (word){
		write(output_fd2, &word, 8);
	} 

	return word;

}

int encode_large_t_diff(unsigned long long t_diff, char t_diff_bitwidth, char large_t_diff_bitwidth, unsigned long long *outbuf2, long *bits_written, int output_fd2){

	// write string of zeros of width t_diff_bitwidth
	unsigned long long write1 = write_bits_to_buffer(0, outbuf2, bits_written, t_diff_bitwidth);

	// write large_t_diff_bitwidth in next byte
	unsigned long long write2 = write_bits_to_buffer(large_t_diff_bitwidth, outbuf2, bits_written, (char) 8);

	// write t_diff in word of width large_t_diff_bitwidth
	unsigned long long write3 = write_bits_to_buffer(t_diff, outbuf2, bits_written, large_t_diff_bitwidth);

}



#endif