#ifndef _COMPRESSION_HELPER_H
#define _COMPRESSION_HELPER_H 1

#include "compression_config.h"
#include "compression_structs.h"
#include <stdlib.h>
#include <stdio.h>

/* helper for name. adds a slash, hex file name and a termial 0 */
char hexdigits[]="0123456789abcdef";
void atohex(char* target,unsigned int v) {
    int i;
    target[0]='/';
    for (i=1;i<9;i++) target[i]=hexdigits[(v>>(32-i*4)) & 15];
    target[9]=0;
}

/* helper to fill protocol bit tables  */
void fill_protocol_bit_tables(
	int protocol_idx,
	int *type2datawidth, 
	int *type3datawidth,
	int *type2patterntable, 
	int *type3patterntable,
	int *statemask,
	int *num_detectors
){
	type2datawidth = &protocol_list[protocol_idx].bits_per_entry_2;
    type3datawidth= &protocol_list[protocol_idx].bits_per_entry_3;
    type2patterntable = (int*)malloc(sizeof(int)* 
				    protocol_list[protocol_idx].detector_entries);
    type3patterntable = (int*)malloc(sizeof(int)*
				    protocol_list[protocol_idx].detector_entries);
    statemask = &protocol_list[protocol_idx].detector_entries-1; /* bitmask */
    if (!type2patterntable | !type3patterntable) fprintf(stderr, "!type2patterntable | !type3patterntable");
    /* fill pattern tables */
    for (int i=0;i<protocol_list[protocol_idx].detector_entries;i++) {
		type2patterntable[i]=protocol_list[protocol_idx].pattern2[i];
		type3patterntable[i]=protocol_list[protocol_idx].pattern3[i];
    };
	
    /* set number of detectors in case we are not in service mode */
    if (protocol_idx) 
	num_detectors = &protocol_list[protocol_idx].num_detectors;
}


// takes in a stream of raw timestamps, outputs a stream of compressed timestamps
int process(struct raw_event *buffer, int bitmask, struct raw_event *in_pointer){

	// parse events in active_buffer
	parse(buffer, bitmask, in_pointer);

	// compress events in active_buffer
	compress();

	return 0;
}

int listen(){
	// add raw events to queue_buffer
	input_bytes = read(buffer, free_pointer, INBUFENTRIES * sizeof(struct raw_event));

	return 0;
}

int open_epoch(unsigned int timestamp_epoch){

	return 0;
}

int parse(
	struct raw_event *buffer, 
	int bitmask, 
	struct raw_event *in_pointer,
	unsigned long long *t_new, 
	unsigned long long *t_old

){
	int clock_value, detector_value;
	unsigned int t_epoch, t_state, t_fine;

	// read one value out of buffer
	clock_value = buffer->_clock_value;
	detector_value = buffer->_detector_value;
	t_epoch = clock_value>>15; // take most significant 17 bits of timer
	t_state = detector_value & bitmask; // get detector pattern
	t_fine = (clock_value<<17) | (detector_value>>15); // fine time unit

	/* trap weird time differences */
	t_new = &( (((unsigned long long)t_epoch)<<32) + t_fine ); /* get event time */
	if (*t_new < *t_old ) { /* negative time difference */
	if ((*t_new - *t_old) & 0x1000000000000ll) { /* check rollover */
		in_pointer++;
		continue; /* ...are ignored */
	}

	t_old = t_new;

	return 0;
}

int compress(){

	// type 2 stream

	// type 3 stream

	return 0;
}

#endif