#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>

// #include "compression_helper.h"
#include "compression_structs.h"
#include "compression_config.h"

/*

usage: 

OPTIONS:
	-i input_stream: stream to read raw events from
	-o output_file: file to write compressed chunks to
	-p protocol: string indicating protocol to be used

*/

int main(int argc, char *argv[]){

	int input, output; // in and out file descriptors
	char input_file[FNAMELENGTH] = "";
	char output_file[FNAMELENGTH] = "";
	int protocol_idx;

	// parse options
	int opt;
	while((opt = getopt(argc, argv, "i:o:p:")) != EOF){
		switch(opt){
		case 'i':
			sscanf(optarg, FNAMEFORMAT, input_file);
			input = open(input_file, O_RDONLY|O_NONBLOCK);
			if (input == -1) perror(errno);
			break;

		case 'o':
			sscanf(optarg, FNAMEFORMAT, output_file);
			output = open(output_file, O_WRONLY|O_CREAT|O_TRUNC,FILE_PERMISSIONS);
			if (output == -1) perror(errno);
			break;

		case 'p':
			if (sscanf(optarg, "%d", &protocol_idx) != 1) perror(errno);
			break;
		}
	}

	// specify bitmask for detector
	struct protocol *protocol = &protocol_list[protocol_idx];
	int bitmask = protocol->detector_entries - 1;

	// initialise active_buffer and queue_buffer to parsed size
	struct raw_event *active_buffer, *queue_buffer;
    fd_set fd_poll;  /* for polling */
    int ignorecount = DEFAULT_IGNORECOUNT; /* how many to ignore initially */

	active_buffer = (struct raw_event *) malloc(INBUFENTRIES * sizeof(struct raw_event));
	if (!active_buffer) exit(0);
	char *active_pointer = (char *) active_buffer;
	char *active_free_pointer;

	queue_buffer = (struct raw_event *) malloc(INBUFENTRIES * sizeof(struct raw_event));
	if (!queue_buffer) exit(0);

	int i1;
	int inbytesread = 0, inelements = 0;
	struct raw_event *inpointer, *inbuffer;

	// start adding raw events to active_buffer
	while (1) {

		/* rescue leftovers from previous read */

		i1 =  inbytesread/sizeof(struct raw_event);
		i1 *= sizeof(struct raw_event);
		for (int i = 0 ; i<inbytesread - i1 ; i++) active_pointer[i] = active_pointer[i+i1];
		i1 = inbytesread - i1;  /* leftover from last time */
		active_free_pointer = &active_pointer[i1]; /* pointer to next free character */

		// wait for data on input

		FD_ZERO(&fd_poll); FD_SET(input, &fd_poll);
		timeout.tv_usec = RETRYREADWAIT; timeout.tv_sec = 0;

		int retval = select(input + 1, &fd_poll, NULL, NULL, &timeout);
		if (retval == -1) {
			fprintf(stderr,"error on select: %d\n",errno);
			break;
		}

		if (!FD_ISSET(input, &fd_poll)) {
			fprintf(stderr,"error on FD_ISSET: %d\n",errno);
			break;
		}

		inbytesread = read(input, active_buffer, INBUFENTRIES*sizeof(struct raw_event)-i1);
		if (!inbytesread) continue; /* wait for next event */
		if (inbytesread == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		inbytesread += i1; /* add leftovers from last time */
		inelements = inbytesread/sizeof(struct raw_event);
		inpointer = active_buffer;

		if (ignorecount) { /* dirty trick to eat away the first few events */
			ignorecount--;
			inpointer++;
			continue;
		}
		do {

			printf("%d\n",inpointer->_clock_value);

		} while(--inelements);		
	}

	// when active_buffer is full, call processor()

		// when done processing active_buffer:
		// - store value of active_buffer in tmp
		// - set value of active_buffer to be queue_buffer
		// - set value of queue_buffer to be tmp

			// call processor()

	return 0;

}


