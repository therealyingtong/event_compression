// define chunk size

// initialise buffer1 

// initialise buffer2 

// initialise active_buffer as buffer1

// initialise queue_buffer as buffer2

// start adding raw events to active_buffer

// when active_buffer is full, call
// processor(): 
// - start compressing events in active_buffer
// - start adding raw events to queue_buffer

	// when done processing active_buffer:
	// - store value of active_buffer in tmp
	// - set value of active_buffer to be queue_buffer
	// - set value of queue_buffer to be tmp

		// call processor()

