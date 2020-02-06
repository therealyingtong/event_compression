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

	int input, output; // input and output file descriptors
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

	// initialise active_buffer and current_event to parsed size
	struct raw_event *active_buffer;
    fd_set fd_poll;  /* for polling */

	active_buffer = (struct raw_event *) malloc(INBUFENTRIES * sizeof(struct raw_event));
	if (!active_buffer) exit(0);
	char *active_pointer = (char *) active_buffer;
	char *active_free_pointer;

	struct raw_event *current_event; //pointer to raw_event currently being read

	int bytes_leftover;
	int bytes_read = 0, elements_read = 0;

	// start adding raw events to active_buffer
	while (1) {

		// /* rescue leftovers from previous read */

		// bytes_leftover =  bytes_read/sizeof(struct raw_event);
		// bytes_leftover *= sizeof(struct raw_event);
		// for (int i = 0 ; i<bytes_read - bytes_leftover ; i++) active_pointer[i] = active_pointer[i + bytes_leftover];
		// bytes_leftover = bytes_read - bytes_leftover;  /* leftover from last time */
		// active_free_pointer = &active_pointer[bytes_leftover]; /* pointer to next free character */

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

		bytes_read = read(input, active_buffer, INBUFENTRIES*sizeof(struct raw_event)-bytes_leftover);
		if (!bytes_read) continue; /* wait for next event */
		if (bytes_read == -1) {
			fprintf(stderr,"error on read: %d\n",errno);
			break;
		}

		bytes_read += bytes_leftover; /* add leftovers from last time */
		elements_read = bytes_read/sizeof(struct raw_event);
		current_event = active_buffer;

		do {
			printf("bytes_read: %d\n", bytes_read);

			printf("bytes_leftover %d\n", bytes_leftover);
			printf("%p\n", &current_event);
			printf("%d\n",current_event->_clock_value);
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


