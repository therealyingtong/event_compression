#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "compression_helper.h"
#include "compression_structs.h"
#include "compression_config.h"

/*

usage: 

OPTIONS:
	-i input_stream: stream to read raw events from
	-o output_file: file to write compressed chunks to

*/

int main(int argc, char *argv[]){

	int input, output; // in and out file handles
	struct raw_event *active_buffer, *queue_buffer;

	// parse options
	int opt;
	while((opt = getopt(argc, argv, "i:o")) != EOF){
		switch(opt){
			case 'i':
				sscanf(optarg, "%d", input);
				break;

			case 'o':
				sscanf(optarg, "%d", output);
				break;
		}
	}

	// initialise active_buffer and queue_buffer to parsed size
	active_buffer = (struct raw_event *) malloc(events_per_chunk * sizeof(struct raw_event));
	if (!active_buffer) exit(0);
	queue_buffer = (struct raw_event *) malloc(events_per_chunk * sizeof(struct raw_event));
	if (!queue_buffer) exit(0);

	// start adding raw events to active_buffer

	// when active_buffer is full, call processor()

		// when done processing active_buffer:
		// - store value of active_buffer in tmp
		// - set value of active_buffer to be queue_buffer
		// - set value of queue_buffer to be tmp

			// call processor()

	return 0;

}


