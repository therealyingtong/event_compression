#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>

#include "compression_helper.h"
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
unsigned int sendword2, sendword3; /* bit accumulators */
int resbits2, resbits3; // how many bits are not used in the accumulator
int type2datawidth, type3datawidth;
int this_epoch_converted_entries; // for output buffers
int detcnts[16], clock_bitwidth, detector_bitwidth; /* detector counts */
int protocol_idx;
struct header_2 head2; /* keeps header for type 2 files */
struct header_3 head3; /* keeps header for type 2 files */

int type2bitwidth = DEFAULT_BITDEPTH; /* for time encoding */
int type2bitwidth_long = DEFAULT_BITDEPTH << 8; /* adaptive filtering */

int index2, index3; // index in outbuf2, outbuf3

char type2_file[FNAMELENGTH] = "";
char type3_file[FNAMELENGTH] = "";

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

	// specify bitmask for detector
	struct protocol *protocol = &protocol_list[protocol_idx];
	int64_t clock_bitmask = ((1 << clock_bitwidth) - 1) << (64 - clock_bitwidth);
	// ll_to_bin(clock_bitmask);
	int64_t detector_bitmask = (1 << detector_bitwidth) - 1;
	// ll_to_bin(detector_bitmask);

	// initialise active_buffer and current_event 
	int64_t *active_buffer; 
    fd_set fd_poll;  /* for polling */

	active_buffer = (int64_t *) malloc(INBUFENTRIES * 8);
	if (!active_buffer) exit(0);
	char *active_pointer = (char *) active_buffer;
	char *active_free_pointer;

	// int64_t *current_event; //pointer to 64-bit event currently being read
	struct raw_event *current_event;

	int bytes_leftover, bytes_read, elements_read;

	// initialise output buffers for type2 and type3 files
	unsigned int *outbuf2, *outbuf3; // output buffer pointer
	outbuf2=(unsigned int*)malloc(TYPE2_BUFFERSIZE*sizeof(unsigned int));
    if (!outbuf2) printf("outbuf2 malloc failed");
	outbuf3=(unsigned int*)malloc(TYPE3_BUFFERSIZE*sizeof(unsigned int));
    if (!outbuf3) printf("outbuf3 malloc failed");

	// prepare first epoch information
	unsigned int epoch; // intermediate results

	unsigned int old_epoch, timestamp_old;  /* storage for old epoch */
    int epoch_init;
	unsigned long long t_new, t_old; // for consistency checks

	epoch = make_first_epoch(DEFAULT_FIRSTEPOCHDELAY);

	this_epoch_converted_entries = 0;
	for (int i = 1 << detector_bitwidth; i; i--) detcnts[i] = 0; /* clear histogram */
    old_epoch = epoch; 
	open_epoch(epoch, &head2, &head3, type2bitwidth, type2datawidth, type3datawidth, protocol_idx);

	/* initialize output buffers and temp storage*/
    index2 = 0; sendword2 = 0; resbits2 = 32;
    index3 = 0; sendword3 = 0; resbits3 = 32;

    epoch_init = 0; /* mark first epoch... */
    timestamp_old = 0;

    /* prepare input buffer settings for first read */
	bytes_read = 0;
	elements_read = 0;
	current_event = active_buffer;

    t_old = 0;

	// start adding raw events to active_buffer
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

		bytes_read = read(input_fd, active_buffer, INBUFENTRIES*8 - bytes_leftover);

		if (!bytes_read) continue; /* wait for next event */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		bytes_read += bytes_leftover; /* add leftovers from last time */
		elements_read = bytes_read/8;
		current_event = active_buffer;


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
		    t_new = (((unsigned long long) epoch)<<32) + clock_value; /* get event time */
		    if (t_new < t_old ) { /* negative time difference */
				if ((t_new-t_old) & 0x1000000000000ll) { /* check rollover */
		    		current_event++;
		    		continue; /* ...are ignored */
				}
				fprintf(stderr, "chopper: point 2, old: %llx, new: %llx\n", t_old,t_new);
			}

			t_old = t_new;

			// 2. type-2 file filling
			timestamp_old = 0;  /* storage for old epoch */
			unsigned int t_diff; /* time difference for encoding */
			unsigned int t1,t2;  /* intermediate variables for bit packing */
			unsigned int t_diff_bitmask;  /* detecting exceptopn words */
			int except_count= 0; // long diff exception counter

	    	t_diff = clock_value - timestamp_old; /* time difference */
	    	timestamp_old = clock_value;

			if (t_diff<2) { /* fudge unlikely events by 0.25 nsec */
				clock_value+=2; t_diff+=2;
			}

			t_diff_bitmask = (1<<type2bitwidth) - 1; // for packing

			if (t_diff != (t2 = (t_diff & t_diff_bitmask))) { /* long diff exception */
				except_count++;
				/* first batch is codeword zero plus a few bits from tdiff */
				t1 = t_diff >> type2bitwidth;
				/* save first part of this longers structure */
				if (resbits2 == 32) {
					outbuf2[index2++]=t1;
				} else {
					sendword2 |= (t1 >> (32-resbits2));
					outbuf2[index2++]=sendword2;
					sendword2 = t1 << resbits2;
				}
			}	

			// insert compression here

			printf("bytes_read: %d\n", bytes_read);
			printf("bytes_leftover %d\n", bytes_leftover);

			current_event++;

		} while(--elements_read);		
	}

	// when active_buffer is full, call processor()

		// when done processing active_buffer:
		// - store value of active_buffer in tmp
		// - set value of active_buffer to be current_event
		// - set value of current_event to be tmp

			// call processor()

	return 0;

}


