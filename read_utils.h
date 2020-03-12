#ifndef _READ_UTILS_H
#define _READ_UTILS_H 1

#include "math_utils.h"

long int findSize(char file_name[]) 
{ 
    // opening the file in read mode 
    FILE* fp = fopen(file_name, "r"); 
  
    // checking if the file exist or not 
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
  
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long int res = ftell(fp); 
  
    // closing the file 
    fclose(fp); 
  
    return res; 
} 
  

unsigned long long read_bits_from_buffer(unsigned long long **current_word, long *bits_read, long *bits_read_in_buf, unsigned char bitwidth, unsigned char *prev_buf_leftover_bitwidth, unsigned char *overlap_bitwidth, unsigned long long *prev_buf_leftover, unsigned char *bufcounter, int bufsize){

	unsigned char word_offset = *bits_read % 64; // start_idx in word
	unsigned long long bitstring = 1;

	// printf("bits_read: %d\n", *bits_read);

	if ((*bits_read_in_buf + bitwidth) >= bufsize){

		// overlap into next buffer
		*bits_read_in_buf = 0;

		// printf("overlap into next buffer\n");
		// printf("*bits_read_in_buf: %d\n", *bits_read_in_buf);
		// printf("*bits_read: %d\n", *bits_read);
		// printf("bitwidth: %d\n", bitwidth);
		// printf("bufsize: %d\n", bufsize);

		(*bufcounter)++;
		// printf("*bufcounter: %d\n", *bufcounter);

		*overlap_bitwidth = word_offset + bitwidth - 64;

		*prev_buf_leftover_bitwidth = bitwidth - *overlap_bitwidth;
		*prev_buf_leftover = read_bits_from_word(**current_word, 64 - word_offset, word_offset);


		*bits_read = *bits_read + 64 - word_offset;	

		// ll_to_bin(**current_word);


	} else if (word_offset + bitwidth >= 64){

		// overlap into next word but in the same buffer

		unsigned char overlap_bitwidth = word_offset + bitwidth - 64;

		unsigned long long first_part_bitstring = read_bits_from_word(**current_word, 64 - word_offset, word_offset);

		// ll_to_bin(**current_word);
		unsigned long long second_part_bitstring;
		(*current_word)++; // read next word
		second_part_bitstring = read_bits_from_word(**current_word, overlap_bitwidth, 0);
		
		bitstring = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;

		*bits_read = *bits_read + bitwidth;	
		*bits_read_in_buf = *bits_read_in_buf + bitwidth;


	} else {

		// does not overlap into next word

		bitstring = read_bits_from_word(**current_word, bitwidth, word_offset);

		*bits_read = *bits_read + bitwidth;	

		*bits_read_in_buf = *bits_read_in_buf + bitwidth;

	}

	return bitstring;

}

#endif