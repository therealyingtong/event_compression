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
	-i input_stream: stream to read compressed events from
	-o output_file: file to write decompressed events to
	-O output_file: file to write type3 compressed chunks to
	-c clock_bitwidth: initial number of bits used to represent a timestamp
	-d detector_bitwidth: number of bits used to represent a detector
	-p protocol: string indicating protocol to be used

*/


// global variables for I/O handling
int input_fd, output_fd;
char output_file[FNAMELENGTH] = "";
unsigned int *outbuf;

int protocol_idx;

int clock_bitwidth, detector_bitwidth; // width (in bits) of clock value and detector value
int inbuf_bitwidth = INBUFENTRIES * 8; // number of bits to allocate to input buffer

int main(int argc, char *argv[]){

	char input_file[FNAMELENGTH] = "";

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

	int t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to timestamp bitwidth

	// specify bitmask for detector
	struct protocol *protocol = &protocol_list[protocol_idx];
	int64_t clock_bitmask = (((long long) 1 << clock_bitwidth) - 1) << (64 - clock_bitwidth);
	// ll_to_bin(clock_bitmask);
	int64_t detector_bitmask = ((long long) 1 << detector_bitwidth) - 1;
	// ll_to_bin(detector_bitmask);

	// initialise inbuf and current_event 
    fd_set fd_poll;  /* for polling */

	struct raw_event *inbuf; // input buffer pointer
	inbuf = (struct raw_event *) malloc(inbuf_bitwidth);
	if (!inbuf) exit(0);
	char *active_pointer = (char *) inbuf;
	char *active_free_pointer;

	struct raw_event *current_event;

	int bytes_read, events_read;

	// initialise output buffers for output files
	
	outbuf = (unsigned int*)malloc(TYPE2_BUFFERSIZE*sizeof(unsigned int));
    if (!outbuf) printf("outbuf2 malloc failed");

	/* prepare input buffer settings for first read */
	bytes_read = 0;
	events_read = 0;
	current_event = inbuf;


	// start adding raw events to inbuf
	while (1) {

		/* rescue leftovers from previous read */

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

		if (!bytes_read) continue; /* wait for next event */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		events_read = bytes_read/8; // each event is 64 bits aka 8 bytes
		current_event = inbuf;

		int64_t sendword2 = 0, sendword3; // full words to send to decoder

		do {
			// 0. read one value out of buffer
			long long msw, lsw;
			msw = current_event->msw; // most significant word
			lsw = current_event->lsw; // least significant word
			long long full_word = (msw << 32) | (lsw);
			// ll_to_bin(full_word) 	;

			long long clock_value = (full_word & clock_bitmask) >> (64 - clock_bitwidth);
			// printf("clock_value: %lld\n", clock_value);
			// ll_to_bin(clock_value);

			unsigned int detector_value = full_word & detector_bitmask;
			// printf("detector_value: %d\n", detector_value);

			// 1. consistency checks

			// 1a) trap negative time differences
		    t_new = clock_value; /* get event time */
		    if (t_new < t_old ) { /* negative time difference */
				if ((t_new-t_old) & 0x1000000000000ll) { /* check rollover */
		    		current_event++;
		    		continue; /* ...are ignored */
				}
				fprintf(stderr, "negative time difference: point 2, old: %llx, new: %llx\n", t_old,t_new);
			}

			// // 2. timestamp compression

	    	long long t_diff = t_new - t_old; /* time difference */
			printf("t_diff: %lld\n", t_diff);
			t_old = t_new;

			if (t_diff + 1 > ((long long) 1 << t_diff_bitwidth)){

				// if t_diff is too large to contain
				int large_t_diff_bitwidth = log2(t_diff) + 1;
				int sendword = encode_large_t_diff(t_diff, t_diff_bitwidth, large_t_diff_bitwidth, outbuf2, &outbuf2_offset, &sendword2, output_fd2);
				t_diff_bitwidth = large_t_diff_bitwidth;
				printf("increase\n");

			} else {
				int sendword = encode_t_diff(t_diff, t_diff_bitwidth, outbuf2, &outbuf2_offset, &sendword2, output_fd2);
				if (t_diff < ((long long)1 << (t_diff_bitwidth - 1))){
					// if t_diff can be contained in a smaller bitwidth
					t_diff_bitwidth--;
					printf("decrease\n");
				} else {
					printf("stay the same\n");
				}

			}

			// write(output_fd2, current_event, 8);

			// printf("t_diff_bitwidth: %d\n", t_diff_bitwidth);

			current_event++;
			printf("events_read: %d\n", events_read);

		} while(--events_read);		
	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_event
		// - set value of current_event to be tmp

			// call processor()

	return 0;


}
