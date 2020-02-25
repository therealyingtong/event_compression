#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#include <math.h>

#include "compression_utils.h"
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
unsigned int *outbuf;

int protocol_idx;

int inbuf_bitwidth = INBUFENTRIES * 8; // number of bits to allocate to input buffer

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

	char t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to timestamp bitwidth

	// specify bitmask for detector
	struct protocol *protocol = &protocol_list[protocol_idx];
	int64_t clock_bitmask = (((long long) 1 << clock_bitwidth) - 1) << (64 - clock_bitwidth);
	// ll_to_bin(clock_bitmask);
	int64_t detector_bitmask = ((long long) 1 << detector_bitwidth) - 1;
	// ll_to_bin(detector_bitmask);

	// initialise inbuf and current_word 
    fd_set fd_poll;  /* for polling */

	int64_t *inbuf; // input buffer pointer
	inbuf = (int64_t *) malloc(inbuf_bitwidth);
	if (!inbuf) exit(0);
	char *active_pointer = (char *) inbuf;
	char *active_free_pointer;

	int64_t *current_word;

	int bytes_read, words_read;

	// initialise output buffers for output files
	
	outbuf = (unsigned int*)malloc(TYPE2_BUFFERSIZE*sizeof(unsigned int));
    if (!outbuf) printf("outbuf2 malloc failed");

	/* prepare input buffer settings for first read */
	bytes_read = 0;
	words_read = 0;
	current_word = inbuf;
	int outbuf_offset = 0; // no. of bits offset in outbuf
	int64_t clock_bitmask = (((long long) 1 << clock_bitwidth) - 1) << (64 - clock_bitwidth);
	char t_diff_bitwidth = clock_bitwidth;
	long long t_diff_bitmask = clock_bitmask;

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
		// bytes_read = read(input_fd, inbuf, 8);

		if (!bytes_read) continue; /* wait for next word */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		words_read = bytes_read/8; // each word is 64 bits aka 8 bytes
		current_word = inbuf;
		char new_buf = 0; // boolean to indicate whether to stay with current buffer
		char new_word = 0; // boolean to indicate whether to read a new word
		int bits_read = 0; // counter for bits read in current buffer

		int64_t timestamp = 0; // timestamps to write to output file

		do {

			if (new_word){
				// read a new word
				current_word++;
				printf("words_read: %d\n", words_read);
			}

			// 0. read next t_diff_bitwidth bits out of word
			long long t_diff = decode_t_diff(t_diff_bitwidth, &current_word, &outbuf_offset, &timestamp, &output_fd);
			
			ll_to_bin(t_diff);

			// 1. adjust t_diff_bitwidth 
			if (!t_diff){
				long long t_diff = decode_large_t_diff(&current_word, &outbuf_offset);
			
			} else {
				if (t_diff < ((long long)1 << (t_diff_bitwidth - 1))){
					t_diff_bitwidth--;
				}
			}

		} while(!new_buf);		
	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_word
		// - set value of current_word to be tmp

			// call processor()

	return 0;


}
