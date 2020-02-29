#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#include <math.h>

#include "decompression_utils.h"
#include "compression_structs.h"
#include "compression_config.h"


/*

usage: 

OPTIONS:
	-i input_stream: stream to read compressed words from
	-o output_file: file to write decompressed words to
	-c clock_bitwidth: initial number of bits used to represent a timestamp
	-d detector_bitwidth: number of bits used to represent a detector
	-p protocol: string indicating protocol to be used

*/


// global variables for I/O handling
int input_fd, output_fd;
unsigned char output_file[FNAMELENGTH] = "";
unsigned long long prev_buf_leftover; // leftover bits from previous buffer
unsigned char prev_buf_leftover_bitwidth; //bitwidth of leftover bits from previous buffer
unsigned char overlap_bitwidth; // bitwidth of overlap into next buffer
unsigned char bufcounter = 1; // counter tracks which buffer we're reading from
unsigned long long t_diff;

int protocol_idx;

int inbuf_bitwidth = INBUFENTRIES * 8; // number of bits to allocate to input buffer
long bits_read = 0; // counter for bits read 
long bits_read_in_buf = 0;

int main(int argc, unsigned char *argv[]){

	unsigned char input_file[FNAMELENGTH] = "";
	unsigned char clock_bitwidth, detector_bitwidth; // width (in bits) of clock value and detector value

	// parse options
	int opt;
	while((opt = getopt(argc, argv, "i:o:c:d:p:")) != EOF){
		switch(opt){
		case 'i':
			sscanf(optarg, FNAMEFORMAT, input_file);
			input_fd = open(input_file, O_RDONLY|O_NONBLOCK);
			if (input_fd == -1) fprintf(stderr, "input_fd open: %s\n", strerror(errno));
			break;

		case 'o': // outfile name and type
			sscanf(optarg, FNAMEFORMAT, output_file);
			output_fd = open(output_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (output_fd == -1) fprintf(stderr, "output_fd2 open: %s\n", strerror(errno));
			break;

		case 'c':
			if (sscanf(optarg, "%d", &clock_bitwidth) != 1){
				fprintf(stderr, "clock_bitwidth sscanf: %s\n", strerror(errno));
			} 
			break;

		case 'd':
			if (sscanf(optarg, "%d", &detector_bitwidth) != 1){
				fprintf(stderr, "detector_bitwidth sscanf: %s\n", strerror(errno));
			} 
			break;

		case 'p':
			if (sscanf(optarg, "%d", &protocol_idx) != 1){
				fprintf(stderr, "protocol sscanf: %s\n", strerror(errno));
			} 
			break;
		}
	}

	unsigned char t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to clock bitwidth
	unsigned long long t_diff_bitmask = ((unsigned long long) 1 << t_diff_bitwidth) - 1; 
	// unsigned long long t_diff;

	// initialise inbuf and current_word 
    fd_set fd_poll;  /* for polling */

	unsigned long long *inbuf; // input buffer pointer
	inbuf = (unsigned long long *) malloc(inbuf_bitwidth);
	if (!inbuf) exit(0);
	unsigned long long *current_word;

	int bytes_read;

	/* prepare input buffer settings for first read */
	bytes_read = 0;
	unsigned char bufcounter = 1;

	unsigned char increase_bitwidth = 0; // flag to indicate if next read is to increase bitwidth (instead of a normal timestamp read)

	// start adding raw words to inbuf
	while (1) {

		// bufcounter++;

		// wait for data on input_fd

		FD_ZERO(&fd_poll); FD_SET(input_fd, &fd_poll);
		timeout.tv_usec = RETRYREADWAIT; timeout.tv_sec = 0;

		int retval = select(input_fd + 1, &fd_poll, NULL, NULL, &timeout);
		if (retval == -1) {
			fprintf(stderr,"error on select: %d\n",errno);
			break;
		}

		if (!FD_ISSET(input_fd, &fd_poll)) {
			fprintf(stderr,"error on FD_ISSET: %d\n",errno);
			break;
		}

		bytes_read = read(input_fd, inbuf, INBUFENTRIES*8);

		if (!bytes_read) continue; /* wait for next word */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		current_word = inbuf;
		printf("bufcounter: %d\n", bufcounter);

		do {

			ll_to_bin(*current_word);

			if (prev_buf_leftover_bitwidth > 0) {

				// overlap from previous buffer

				// printf("current_word: ");
				// ll_to_bin(*current_word);

				// printf("the first word of this buffer has some leftover bits in the previous buffer\n");

				unsigned long long first_part_bitstring = prev_buf_leftover;

				bits_read_in_buf = 0;

				unsigned long long second_part_bitstring = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, overlap_bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);

				if (increase_bitwidth){
					// printf("increase_bitwidth across buffer\n");
					t_diff_bitwidth = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;
					increase_bitwidth = 0;

					// printf("t_diff_bitwidth better not be zero: %d\n", t_diff_bitwidth);

					t_diff = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, t_diff_bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);

					// printf("t_diff:\n");
					// ll_to_bin(t_diff);

				} else {
					t_diff = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;
					// printf("t_diff:\n");
					// ll_to_bin(t_diff);

				}

				prev_buf_leftover_bitwidth = 0;

			} else {

				if (increase_bitwidth){
					t_diff_bitwidth = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, 8, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);	

					t_diff = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, t_diff_bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);	

					increase_bitwidth = 0;	

				} else {
					t_diff = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, t_diff_bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);

				}


				// printf("no leftover from prev buffer\n");


				// printf("t_diff_bitwidth: %d\n", t_diff_bitwidth);
				// printf("t_diff:\n");
				// ll_to_bin(t_diff);

			} 

			// printf("t_diff before if statements: %lld\n", t_diff);

			if (t_diff == 0){
				// all zeros, meaning we increased bitwidth
				// printf("increase_bitwidth \n");

				t_diff_bitwidth = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, 8, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);

				printf("t_diff_bitwidth better not be zero: %d\n", t_diff_bitwidth);


				if (prev_buf_leftover_bitwidth > 0){

					increase_bitwidth = 1;

					// printf("large_t_diff_bitwidth == 1, which means the bits encoding the increased bitwidth size span two buffers\n");

					continue;
				}

				increase_bitwidth = 0;

				t_diff = read_bits_from_buffer(&current_word, &bits_read, &bits_read_in_buf, t_diff_bitwidth, &prev_buf_leftover_bitwidth, &overlap_bitwidth, &prev_buf_leftover, &bufcounter);


			} 
			
			// else {
				// we got a t_diff, check if we need to decrease bitwidth
				// ll_to_bin(t_diff);
				if (t_diff < ((unsigned long long)1 << (t_diff_bitwidth - 1))){
					t_diff_bitwidth--;
				}
			// }

			// printf("bufcounter: %d\n", bufcounter);

		} while((bits_read + t_diff_bitwidth <= bufcounter * INBUFENTRIES*8*8) && bufcounter < 3);		
		// } while((bits_read_in_buf + t_diff_bitwidth < INBUFENTRIES*8*8));		

	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_word
		// - set value of current_word to be tmp

			// call processor()

	return 0;


}
