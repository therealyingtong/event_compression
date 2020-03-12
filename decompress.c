#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>

#include "read_utils.h"
#include "compression_structs.h"
#include "compression_config.h"


/*

usage: 

OPTIONS:
	-i input_stream: stream to read compressed words from
	-o output_file: file to write decompressed words to
	-b init_bitwidth: initial number of bits used to represent a value
	-p protocol: string indicating protocol to be used
	-a adaptive: boolean indicating whether or not to change init_bitwidth

*/


// global variables for I/O handling
int input_fd, output_fd;
unsigned char output_file[FNAMELENGTH] = "";
unsigned long long prev_buf_leftover = 0; // leftover bits from previous buffer
unsigned char prev_buf_leftover_bitwidth; //bitwidth of leftover bits from previous buffer
unsigned char overlap_bitwidth; // bitwidth of overlap into next buffer
unsigned char bufcounter = 1; // counter tracks which buffer we're reading from
unsigned long long value;

int protocol_idx = 1;
char adaptive = 0;
int inbuf_bytes = INBUFENTRIES * 8; // number of bytes to allocate to input buffer
int inbuf_bits = INBUFENTRIES * 8 * 8;
long bits_read = 0; // counter for bits read 
long bits_read_in_buf = 0;
unsigned long long *current_word;

unsigned long long rescue_bits(){
	unsigned long long first_part_bitstring = prev_buf_leftover;

	unsigned long long second_part_bitstring = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, overlap_bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter, inbuf_bits);

	unsigned long long rescued = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;

	prev_buf_leftover = 0;
	prev_buf_leftover_bitwidth = 0;

	return rescued;
}

int main(int argc, unsigned char *argv[]){

	unsigned char input_file[FNAMELENGTH] = "";
	unsigned char init_bitwidth; // width (in bits) of init value

	// parse options
	int opt;
	while((opt = getopt(argc, argv, "i:o:b:p:a")) != EOF){
		switch(opt){
		case 'i':
			sscanf(optarg, FNAMEFORMAT, input_file);
			input_fd = open(input_file, O_RDONLY|O_NONBLOCK);
			if (input_fd == -1) fprintf(stderr, "input_fd open: %s\n", strerror(errno));
			break;

		case 'o': // outfile name and type
			sscanf(optarg, FNAMEFORMAT, output_file);
			output_fd = open(output_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (output_fd == -1) fprintf(stderr, "output_fd open: %s\n", strerror(errno));
			break;

		case 'b':
			if (sscanf(optarg, "%d", &init_bitwidth) != 1){
				fprintf(stderr, "init_bitwidth sscanf: %s\n", strerror(errno));
			} 
			break;

		case 'p':
			if (sscanf(optarg, "%d", &protocol_idx) != 1){
				fprintf(stderr, "protocol sscanf: %s\n", strerror(errno));
			} 
			break;

		case 'a':
			adaptive = 1;
			break;
		}
	}

	unsigned char bitwidth = init_bitwidth; // initialise bitwidth to init bitwidth
	unsigned long long value_bitmask = ((unsigned long long) 1 << bitwidth) - 1; 

	// initialise inbuf and current_word 
    fd_set poll;  /* for polling */

	unsigned long long *inbuf; // input buffer pointer
	inbuf = (unsigned long long *) calloc(inbuf_bytes, 1);
	if (!inbuf){
		printf("!inbuf");
		exit(0);
	} 	

	int bytes_read = 0;
	int bytes_in_file = findSize(input_file);

	int bitwidths_read_in_buf; // for non-adaptive case

	/* prepare input buffer settings for first read */
	unsigned char bufcounter = 1;

	unsigned char increase_bitwidth = 0; // flag to indicate if next read is to increase bitwidth (instead of a normal value read)

	int EOF_counter = 0;


	// start adding raw words to inbuf
	while (1 && EOF_counter < 10 && bytes_read < bytes_in_file) {

		// wait for data on input_fd

		FD_ZERO(&poll); FD_SET(input_fd, &poll);
		timeout.tv_usec = RETRYREADWAIT; timeout.tv_sec = 0;

		int retval = select(input_fd + 1, &poll, NULL, NULL, &timeout);
		if (retval == -1) {
			fprintf(stderr,"error on select: %d\n",errno);
			break;
		}

		if (!FD_ISSET(input_fd, &poll)) {
			fprintf(stderr,"error on FD_ISSET: %d\n",errno);
			break;
		}

		bytes_read += read(input_fd, inbuf, INBUFENTRIES*8);

		if (!bytes_read) {
			EOF_counter++;
			printf("!bytes_read");
			continue; /* wait for next event */
		}
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		current_word = inbuf;
		int old_bufcounter = bufcounter;

		// printf("bufcounter: %d\n", bufcounter);
		// printf("bytes in file: %d\n", bytes_in_file);
		// printf("bytes read: %d\n", bytes_read);
		// printf("bits_read_in_buf: %d\n", bits_read_in_buf);


		do {

			if (bits_read == 0){

				// printf("first word in first buffer\n");
				value = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter, inbuf_bits);
				 
			} else {
				unsigned long long rescued = 0;

				if (prev_buf_leftover_bitwidth > 0){
					// printf("rescuing bits\n");
					rescued = rescue_bits();

					if (increase_bitwidth > 0){

						// printf("increase_bitwidth > 0\n");

						bitwidth = rescued;

						value = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter, inbuf_bits);

						if (prev_buf_leftover_bitwidth > 0){
							increase_bitwidth = 0;
							continue;
						}
		
					} else if (increase_bitwidth == 0) {

						// printf("increase_bitwidth == 0\n");

						value = rescued;
					}

				} else {

					value = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter, inbuf_bits);

					if (prev_buf_leftover_bitwidth > 0){
						increase_bitwidth = 0;
						continue;
					}

				}

				if (adaptive){
					if (value == 0){

					// all zeros, meaning we increased bitwidth
					// printf("increase_bitwidth \n");

						bitwidth = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, 8, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter, inbuf_bits);

						if (prev_buf_leftover_bitwidth > 0){
							increase_bitwidth = 1;
							continue;
						} 

						value = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter, inbuf_bits);

						if (prev_buf_leftover_bitwidth > 0){
							increase_bitwidth = 0;
							continue;
						}
						
					}
				}
	 
			} 

			if ((long long) value > 0) write(output_fd, &value, 8);

			if (adaptive){
				// we got a value, check if we need to decrease bitwidth
				if (value < ((unsigned long long) 1 << (bitwidth - 1))){
					bitwidth--;
				}
			}

		} while(
			(prev_buf_leftover_bitwidth == 0 && adaptive &&  bits_read < bytes_in_file*8) ||		
			(old_bufcounter == bufcounter && !adaptive && bits_read < bytes_in_file*8)
		);	

		fflush (stdout);	
	
	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_word
		// - set value of current_word to be tmp

			// call processor()

	// printf("\n");
	// fflush (stdout);

	return 0;


}
