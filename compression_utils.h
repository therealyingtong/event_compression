#ifndef _COMPRESSION_UTILS_H
#define _COMPRESSION_UTILS_H 1

#include "math_utils.h"
#include <unistd.h>

unsigned long long write_bits_to_buffer(unsigned long long bitstring, unsigned long long *buffer, long *bits_written, char bitwidth, char *sendword){

	char buffer_offset = *bits_written % 64;

	*bits_written = *bits_written + bitwidth;

	if (buffer_offset + bitwidth >= 64){
		// fill word and write overlap into buffer
		char overlap_bitwidth = buffer_offset + bitwidth - 64;

		unsigned long long first_part_bitstring = read_bits_from_word(bitstring, bitwidth - overlap_bitwidth, 64 - bitwidth);

		unsigned long long second_part_bitstring = read_bits_from_word(bitstring, overlap_bitwidth, 64 - overlap_bitwidth);


		unsigned long long word = *buffer | first_part_bitstring ;
		*buffer = second_part_bitstring << (64 - overlap_bitwidth);
		
		*sendword = 1;
		return word;

	} else {
		// write into buffer

		*buffer = *buffer | (bitstring << (64 - bitwidth - buffer_offset)) ;

		*sendword = 0;
		return 0;

	}

}

unsigned long long encode_t_diff(unsigned long long t_diff, char t_diff_bitwidth, unsigned long long *outbuf2, long *bits_written, char *sendword, int output_fd2){

	unsigned long long word = write_bits_to_buffer(t_diff, outbuf2, bits_written, t_diff_bitwidth, sendword);

	if (*sendword){
		write(output_fd2, &word, 8);
	} 

	return word;

}

int encode_large_t_diff(unsigned long long t_diff, char t_diff_bitwidth, char large_t_diff_bitwidth, unsigned long long *outbuf2, long *bits_written, char *sendword, int output_fd2){

	// write string of zeros of width t_diff_bitwidth
	unsigned long long word1 = write_bits_to_buffer(0, outbuf2, bits_written, t_diff_bitwidth, sendword);

	if (*sendword){
		write(output_fd2, &word1, 8);
	} 

	// write large_t_diff_bitwidth in next byte
	unsigned long long word2 = write_bits_to_buffer(large_t_diff_bitwidth, outbuf2, bits_written, (char) 8, sendword);

	if (*sendword){
		write(output_fd2, &word2, 8);	
	} 

	// write t_diff in word of width large_t_diff_bitwidth
	unsigned long long word3 = write_bits_to_buffer(t_diff, outbuf2, bits_written, large_t_diff_bitwidth, sendword);

	if (*sendword){
		write(output_fd2, &word3, 8);	
	} 

}



#endif