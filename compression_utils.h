#ifndef _COMPRESSION_UTILS_H
#define _COMPRESSION_UTILS_H 1

#include "math_utils.h"
#include <unistd.h>

unsigned long long write_bits_to_buffer(unsigned long long bitstring, unsigned long long *buffer, long *bits_written, unsigned char bitwidth, unsigned char *sendword){

	unsigned char buffer_offset = *bits_written % 64;

	*bits_written = *bits_written + bitwidth;

	if (buffer_offset + bitwidth >= 64){
		// fill word and write overlap into buffer
		unsigned char overlap_bitwidth = buffer_offset + bitwidth - 64;

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

unsigned long long encode_bitstring(unsigned long long bitstring, unsigned char bitwidth, unsigned long long *outbuf, long *bits_written, unsigned char *sendword, int output_fd){

	unsigned long long word = write_bits_to_buffer(bitstring, outbuf, bits_written, bitwidth, sendword);

	if (*sendword){
		// ll_to_bin(word);
		write(output_fd, &word, 8);
	} 

	return word;

}

int encode_large_bitstring(unsigned long long bitstring, unsigned char bitwidth, unsigned char large_bitwidth, unsigned long long *outbuf, long *bits_written, unsigned char *sendword, int output_fd){

	// write string of zeros of width bitwidth
	unsigned long long word1 = write_bits_to_buffer(0, outbuf, bits_written, bitwidth, sendword);

	if (*sendword){
		// ll_to_bin(word1);

		write(output_fd, &word1, 8);
	} 

	// write large_bitwidth in next byte
	unsigned long long word2 = write_bits_to_buffer(large_bitwidth, outbuf, bits_written, (unsigned char) 8, sendword);

	if (*sendword){
		// ll_to_bin(word2);

		write(output_fd, &word2, 8);	
	} 

	// write bitstring in word of width large_bitwidth
	unsigned long long word3 = write_bits_to_buffer(bitstring, outbuf, bits_written, large_bitwidth, sendword);

	if (*sendword){
		// ll_to_bin(word3);

		write(output_fd, &word3, 8);	
	} 

}



#endif