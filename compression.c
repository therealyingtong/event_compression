#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>

#include "compression_utils.h"
#include "compression_structs.h"
#include "compression_config.h"

/*

usage: 

OPTIONS:
	-i input_stream: stream to read raw events from
	-o output_file: file to write type2 compressed chunks to
	-O output_file: file to write type3 compressed chunks to
	-c clock_bitwidth: number of bits used to represent a timestamp
	-d detector_bitwidth: number of bits used to represent a detector
	-p protocol: string indicating protocol to be used

*/

// global variables for I/O handling
int input_fd, output_fd2, output_fd3; // input and output file descriptors
char type2_file[FNAMELENGTH] = "";
char type3_file[FNAMELENGTH] = "";

int detcnts[16]; /* detector counts */
int protocol_idx;

int inbuf_bitwidth = INBUFENTRIES * 8; // number of bits to allocate to input buffer
unsigned long long *outbuf2, *outbuf3; // output buffers pointers
int outbuf2_offset, outbuf3_offset; // offset from right of output buffers

int main(int argc, char *argv[]){

	char clock_bitwidth, detector_bitwidth; // width (in bits) of clock value and detector value

	char input_file[FNAMELENGTH] = "";
	int *type2patterntable, *type3patterntable; /* for protocol */

	// parse options
	int opt;
	while((opt = getopt(argc, argv, "i:o:O:c:d:p:")) != EOF){
		switch(opt){
		case 'i':
			sscanf(optarg, FNAMEFORMAT, input_file);
			input_fd = open(input_file, O_RDONLY|O_NONBLOCK);
			if (input_fd == -1) fprintf(stderr, "input_fd open: %s\n", strerror(errno));
			break;

		case 'o': // outfile2 name and type
			sscanf(optarg, FNAMEFORMAT, type2_file);
			output_fd2 = open(type2_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (output_fd2 == -1) fprintf(stderr, "output_fd2 open: %s\n", strerror(errno));
			break;

	    case 'O': /* outfile3 name and type */
			sscanf(optarg, FNAMEFORMAT, type3_file);
			output_fd3 = open(type3_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (output_fd3 == -1) fprintf(stderr, "output_fd3 open: %s\n", strerror(errno));
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

	// // protocol details
	// struct protocol *protocol = &protocol_list[protocol_idx];

	char t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to clock_bitwidth

	// specify bitmasks for detector and clock
	unsigned long long clock_bitmask = (((unsigned long long) 1 << clock_bitwidth) - 1) << (64 - clock_bitwidth); //leftmost bits
	unsigned long long detector_bitmask = ((unsigned long long) 1 << detector_bitwidth) - 1; //rightmost bits

	// initialise inbuf and current_event 
    fd_set fd_poll;  /* for polling */

	struct raw_event *inbuf; // input buffer pointer
	inbuf = (struct raw_event *) malloc(inbuf_bitwidth);
	if (!inbuf) exit(0);
	struct raw_event *current_event;

	int bytes_read, events_read;

	// initialise output buffers for type2 and type3 files
	
	outbuf2 = (unsigned int*)malloc(TYPE2_BUFFERSIZE*sizeof(unsigned int));
    if (!outbuf2) printf("outbuf2 malloc failed");
	outbuf3 = (unsigned int*)malloc(TYPE3_BUFFERSIZE*sizeof(unsigned int));
    if (!outbuf3) printf("outbuf3 malloc failed");

	unsigned long long t_new, t_old; // for consistency checks

	for (int i = 1 << detector_bitwidth; i; i--) detcnts[i] = 0; /* clear histogram */

	/* initialize output buffers and temp storage*/

    /* prepare input buffer settings for first read */
	bytes_read = 0;
	events_read = 0;
	current_event = inbuf;

    t_old = 0;

	long bits_written = 0;

	// start adding raw events to inbuf
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

		if (!bytes_read) continue; /* wait for next event */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		events_read = bytes_read/8; // each event is 64 bits aka 8 bytes
		current_event = inbuf;

		unsigned long long sendword2 = 0, sendword3; // full words to send to decoder

		do {
			// 0. read one value out of buffer
			unsigned long long msw, lsw;
			msw = current_event->msw; // most significant word
			lsw = current_event->lsw; // least significant word
			unsigned long long full_word = (msw << 32) | (lsw);

			unsigned long long clock_value = (full_word & clock_bitmask) >> (64 - clock_bitwidth);

			unsigned int detector_value = full_word & detector_bitmask;

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

			// 2. timestamp compression

	    	unsigned long long t_diff = t_new - t_old; /* time difference */
			t_old = t_new;
			ll_to_bin(t_diff);

			if (t_diff + 1 > ((unsigned long long) 1 << t_diff_bitwidth)){

				// if t_diff is too large to contain
				char large_t_diff_bitwidth = log2(t_diff) + 1;

				sendword2 = encode_large_t_diff(t_diff, t_diff_bitwidth, large_t_diff_bitwidth, outbuf2, &bits_written, output_fd2);

				t_diff_bitwidth = large_t_diff_bitwidth;
				// printf("increase\n");

			} else {
				sendword2 = encode_t_diff(t_diff, t_diff_bitwidth, outbuf2, &bits_written, output_fd2);

				if (t_diff < ((unsigned long long)1 << (t_diff_bitwidth - 1))){
					// if t_diff can be contained in a smaller bitwidth
					t_diff_bitwidth--;
				} else {
					// printf("stay the same\n");
				}

			}

			current_event++;

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


