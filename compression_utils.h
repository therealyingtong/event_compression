#ifndef _COMPRESSION_UTILS_H
#define _COMPRESSION_UTILS_H 1

#include "compression_config.h"
#include "compression_structs.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "math_utils.h"

long long write_bits_to_buffer(long long bitstring, long *buffer, long *bits_written, char bitwidth){

	char buffer_offset = *bits_written % 64;
	*bits_written += bitwidth;

	if (buffer_offset + bitwidth > 64){
		// fill word and write overlap into buffer
		char overlap_bitwidth = buffer_offset + bitwidth - 64;

		long long first_part_bitstring = read_bits_from_word(bitstring, bitwidth - overlap_bitwidth, 64 - bitwidth);

		long long second_part_bitstring = read_bits_from_word(bitstring, overlap_bitwidth, 64 - overlap_bitwidth);

		long word = *buffer | (first_part_bitstring >> buffer_offset);
		*buffer = second_part_bitstring << (64 - overlap_bitwidth);

		return word;

	} else {
		// write into buffer
		*buffer = *buffer | (bitstring << (64 - bitwidth));
		return 0;
	}

}

long long encode_t_diff(long long t_diff, char t_diff_bitwidth, long *outbuf2, long *bits_written, int output_fd2){

	long long word = write_bits_to_buffer(t_diff, outbuf2, bits_written, t_diff_bitwidth);

}

int encode_large_t_diff(long long t_diff, char t_diff_bitwidth, char large_t_diff_bitwidth, long *outbuf2, long *bits_written, int output_fd2){

	// write string of zeros of width t_diff_bitwidth
	long long write1 = write_bits_to_buffer(0, outbuf2, bits_written, t_diff_bitwidth);

	// write large_t_diff_bitwidth in next byte
	long long write2 = write_bits_to_buffer(large_t_diff_bitwidth, outbuf2, bits_written, (char) 8);

	// write t_diff in word of width large_t_diff_bitwidth
	long long write3 = write_bits_to_buffer(t_diff, outbuf2, bits_written, large_t_diff_bitwidth);

}



#endif