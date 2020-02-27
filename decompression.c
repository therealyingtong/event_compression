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
char output_file[FNAMELENGTH] = "";
unsigned long long prev_buf_leftover; // leftover bits from previous buffer
char prev_buf_leftover_bitwidth; //bitwidth of leftover bits from previous buffer
char bufcounter = 1; // cuunter tracks which buffer we're reading from
unsigned long long t_diff;

int protocol_idx;

int inbuf_bitwidth = INBUFENTRIES * 8; // number of bits to allocate to input buffer
long bits_read = 0; // counter for bits read 
long bits_read_in_buf = 0;

int main(int argc, char *argv[]){

	char input_file[FNAMELENGTH] = "";
	char clock_bitwidth, detector_bitwidth; // width (in bits) of clock value and detector value

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

	char t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to clock bitwidth
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

	// start adding raw words to inbuf
	while (1) {

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

		do {

			// 0. rescue leftover bits from previous buffer

			// 1. read next t_diff_bitwidth bits out of word
			t_diff = decode_t_diff(&t_diff_bitwidth, &bits_read, &bits_read_in_buf, &current_word, output_fd, &prev_buf_leftover_bitwidth, &prev_buf_leftover);
			
			// 2. adjust t_diff_bitwidth 
			if (!t_diff){
				t_diff = decode_large_t_diff(&t_diff_bitwidth, &bits_read, &bits_read_in_buf, &current_word, output_fd, &prev_buf_leftover_bitwidth, &prev_buf_leftover);
		
			} else {
				if (t_diff < ((unsigned long long)1 << (t_diff_bitwidth - 1))){
					t_diff_bitwidth--;
				}
			}

			ll_to_bin(t_diff);
			printf("bits_read: %d\n", bits_read);
			printf("bits_read_in_buf: %d\n", bits_read_in_buf);

		} while(bits_read_in_buf <  INBUFENTRIES*8*8);		
	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_word
		// - set value of current_word to be tmp

			// call processor()

	return 0;


}
