#ifndef _COMPRESSION_HELPER_H
#define _COMPRESSION_HELPER_H 1

#include "compression_config.h"

// takes in a stream of raw timestamps, outputs a stream of compressed timestamps

int processor(int input){

	// - compress events in active_buffer

	// - add raw events to queue_buffer
	input_bytes = read(input, free_pointer, events_per_chunk * sizeof(struct raw_event));

	return 0;
}

int open_epoch(unsigned int timestamp_epoch){

	return 0;
}

int compress(){

	return 0;
}

#endif