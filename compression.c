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
int clock_bitwidth, detector_bitwidth; // width (in bits) of clock value and detector value

int inbuf_bitwidth = INBUFENTRIES * 8; // number of bits to allocate to input buffer
unsigned int *outbuf2, *outbuf3; // output buffers pointers
int outbuf2_offset, outbuf3_offset; // offset from right of output buffers
int64_t *sendword2, *sendword3; // full words to send to decoder

int main(int argc, char *argv[]){

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

	int clock_bitwidth_overlap = clock_bitwidth - 32;
	int t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to timestamp bitwidth

	// specify bitmask for detector
	struct protocol *protocol = &protocol_list[protocol_idx];
	int64_t clock_bitmask = ((1 << clock_bitwidth) - 1) << (64 - clock_bitwidth);
	// ll_to_bin(clock_bitmask);
	int64_t detector_bitmask = (1 << detector_bitwidth) - 1;
	// ll_to_bin(detector_bitmask);

	// initialise inbuf and current_event 
    fd_set fd_poll;  /* for polling */

	struct raw_event *inbuf; // input buffer pointer
	inbuf = (struct raw_event *) malloc(inbuf_bitwidth);
	if (!inbuf) exit(0);
	char *active_pointer = (char *) inbuf;
	char *active_free_pointer;

	struct raw_event *current_event;

	int bytes_leftover, bytes_read, elements_read;

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
	elements_read = 0;
	current_event = inbuf;

    t_old = 0;

	// start adding raw events to inbuf
	while (1) {

		/* rescue leftovers from previous read */

		bytes_leftover =  bytes_read/8;
		bytes_leftover *= 8;
		for (int i = 0 ; i<bytes_read - bytes_leftover ; i++) active_pointer[i] = active_pointer[i + bytes_leftover];
		bytes_leftover = bytes_read - bytes_leftover;  /* leftover from last time */
		active_free_pointer = &active_pointer[bytes_leftover]; /* pointer to next free character */

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

		bytes_read = read(input_fd, inbuf, INBUFENTRIES*8 - bytes_leftover);

		if (!bytes_read) continue; /* wait for next event */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		bytes_read += bytes_leftover; /* add leftovers from last time */
		elements_read = bytes_read/8;
		current_event = inbuf;


		do {
			// 0. read one value out of buffer
			long long msw, lsw;
			msw = current_event->msw; // most significant word
			lsw = current_event->lsw; // least significant word
			long long full_word = (msw << 32) | (lsw);
			ll_to_bin(full_word);

			long long clock_value = (full_word & clock_bitmask) >> (64 - clock_bitwidth);
			printf("clock_value: %lld\n", clock_value);
			ll_to_bin(clock_value);

			unsigned int detector_value = full_word & detector_bitmask;
			printf("detector_value: %d\n", detector_value);

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

			long long t_diff;
	    	t_diff = t_new - t_old; /* time difference */
			t_old = t_new;

			if (t_diff > (1 << t_diff_bitwidth)){
				// if t_diff is too large to contain
				int large_t_diff_bitwidth = log2(t_diff) + 1;
				encode_large_t_diff(t_diff, t_diff_bitwidth, large_t_diff_bitwidth);
				t_diff_bitwidth = large_t_diff_bitwidth;

			} else {
				encode_t_diff(t_diff, t_diff_bitwidth);
				if (t_diff < (1 << t_diff_bitwidth)){
					// if t_diff can be contained in a smaller bitwidth
					t_diff_bitwidth--;
				}
			}

			current_event++;

		} while(--elements_read);		
	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_event
		// - set value of current_event to be tmp

			// call processor()

	return 0;

}


