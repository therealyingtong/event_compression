#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "write_utils.h"
#include "compression_structs.h"
#include "compression_config.h"

/*

usage: 

OPTIONS:
	-i input_stream: stream to read raw events from
	-o timestamp_file: file to write compressed timestamps to
	-O detector_file: file to write detector patterns to
	-c clock_bitwidth: number of bits used to represent a timestamp
	-d detector_bitwidth: number of bits used to represent a detector
	-p protocol: string indicating protocol to be used

*/

// global variables for I/O handling
int input_fd, timestamp_fd, detector_fd; // input and output file descriptors
unsigned char timestamp_file[FNAMELENGTH] = "";
unsigned char detector_file[FNAMELENGTH] = "";

int detcnts[16]; /* detector counts */
int protocol_idx;

int inbuf_bytes = INBUFENTRIES * 8; // number of bytes to allocate to input buffer
unsigned long long *outbuf2, *outbuf3; // output buffers pointers
int outbuf2_offset, outbuf3_offset; // offset from right of output buffers
long timestamp_bits_written, detector_bits_written;
unsigned char timestamp_sendword, detector_sendword; // bool to indicate whether word is full and should be written to outfile
unsigned long long t_new, t_old; // for consistency checks


int main(int argc, unsigned char *argv[]){

	unsigned char clock_bitwidth, detector_bitwidth; // width (in bits) of clock value and detector value

	unsigned char input_file[FNAMELENGTH] = "";
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

		case 'o': // timestamp_file name and type
			sscanf(optarg, FNAMEFORMAT, timestamp_file);
			timestamp_fd = open(timestamp_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (timestamp_fd == -1) fprintf(stderr, "timestamp_fd open: %s\n", strerror(errno));
			break;

	    case 'O': /* outfile3 name and type */
			sscanf(optarg, FNAMEFORMAT, detector_file);
			detector_fd = open(detector_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (detector_fd == -1) fprintf(stderr, "detector_fd open: %s\n", strerror(errno));
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

	unsigned char t_diff_bitwidth = clock_bitwidth; // initialise t_diff_bitwidth to clock_bitwidth
	unsigned long long t_diff ;
	unsigned long long clock_value;
	unsigned long long detector_value;

	// specify bitmasks for detector and clock
	unsigned long long clock_bitmask = (((unsigned long long) 1 << clock_bitwidth) - 1) << (64 - clock_bitwidth); //leftmost bits

	unsigned long long detector_bitmask = ((unsigned long long) 1 << detector_bitwidth) - 1; //rightmost bits

	// initialise inbuf and current_event 
    fd_set fd_poll;  /* for polling */

	struct raw_event *inbuf; // input buffer pointer
	inbuf = (struct raw_event *) calloc(inbuf_bytes, sizeof(struct raw_event));
	if (!inbuf) exit(0);
	struct raw_event *current_event;

	int bytes_read, events_read;

	// initialise output buffers for type2 and type3 files
	
	outbuf2 = (unsigned long long*)calloc(TYPE2_BUFFERSIZE,sizeof(unsigned long long));
    if (!outbuf2) printf("outbuf2 malloc failed");
	outbuf3 = (unsigned long long*)malloc(TYPE3_BUFFERSIZE*sizeof(unsigned long long*));
    if (!outbuf3) printf("outbuf3 malloc failed");


	for (int i = 1 << detector_bitwidth; i; i--) detcnts[i] = 0; /* clear histogram */

	/* initialize output buffers and temp storage*/

    /* prepare input buffer settings for first read */
	bytes_read = 0;
	events_read = 0;
	// current_event = inbuf;
    t_old = 0;
	timestamp_bits_written = 0;
	timestamp_sendword = 0; // bool to indicate whether word is full and should be written to outfile
	unsigned long long msw, lsw;
	unsigned long long full_word;

	struct stat buf;
	fstat(input_fd, &buf);
	off_t num_events = buf.st_size / 8;
	int EOF_counter = 0;
	int total_events_read = 0;

	// start adding raw events to inbuf
	while (1 && EOF_counter < 10) {

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

		if (!bytes_read) {
			EOF_counter++;
			continue; /* wait for next event */
		}
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}
		// events_read = bytes_read/8; // each event is 64 bits aka 8 bytes
		events_read = bytes_read/8; // each event is 64 bits aka 8 bytes

		current_event = inbuf;

		do {

			total_events_read++;
			// 0. read one value out of buffer
			
			msw = current_event->msw; // most significant word
			lsw = current_event->lsw; // least significant word
			full_word = (msw << 32) | (lsw);

			clock_value = (full_word & clock_bitmask) >> (64 - clock_bitwidth);

			detector_value = full_word & detector_bitmask; /* get detector pattern */

		    t_new = clock_value; /* get event time */

			// 2. timestamp compression
	    	t_diff = t_new - t_old; /* time difference */
			t_old = t_new;

			if ((long long) t_diff >= 0){
				// ll_to_bin(t_diff);

				// ll_to_bin(detector_value);
				encode_bitstring(detector_value, detector_bitwidth, outbuf3, &detector_bits_written, &detector_sendword, detector_fd, total_events_read, num_events);

				if (t_diff + 1 > ((unsigned long long) 1 << t_diff_bitwidth)){

					// if t_diff is too large to contain
					unsigned char large_t_diff_bitwidth = log2(t_diff) + 1;

					encode_large_bitstring(t_diff, t_diff_bitwidth, large_t_diff_bitwidth, outbuf2, &timestamp_bits_written, &timestamp_sendword, timestamp_fd, total_events_read, num_events);

					t_diff_bitwidth = large_t_diff_bitwidth;

				} else {
					encode_bitstring(t_diff, t_diff_bitwidth, outbuf2, &timestamp_bits_written, &timestamp_sendword, timestamp_fd, total_events_read, num_events);

					if (t_diff < ((unsigned long long) 1 << (t_diff_bitwidth - 1))){
						// if t_diff can be contained in a smaller bitwidth
						t_diff_bitwidth--;
						// printf("t_diff_bitwidth--;");

					} else {
						// printf("stay the same\n");
					}
				}

			}

			current_event++;

		} while(--events_read);	
		// } while(--events_read + 1);	

		// fflush (stdout);
	
	}

	// when inbuf is full, call processor()

		// when done processing inbuf:
		// - store value of inbuf in tmp
		// - set value of inbuf to be current_event
		// - set value of current_event to be tmp

			// call processor()

	// printf("\n");
	// fflush (stdout);

	return 0;

}


