#ifndef _COMPRESSION_STRUCTS_H
#define _COMPRESSION_STRUCTS_H 1

typedef struct raw_event {
	unsigned int cv; /* most significant word */
    unsigned int dv; /* least significant word */
};

/* protocol definitions */
typedef struct protocol {
    int bits_per_entry_2;
    int bits_per_entry_3;
    int detector_entries; /* number of detectorentries; 16 for 4 detectors;
			    this value -1 is used as bitmask for status */
    int num_detectors; /* to cater logging purposes for more than
			      4 detectors */
    int pattern2[16];
    int pattern3[16];
};

struct protocol bb84 = {
/* standard BB84. assumed sequence:  (LSB) V,-,H,+ (MSB);
      HV basis: 0, +-basis: 1, result: V-: 0, result: H+: 1 */
    1,1,16,4,
    {0,0,1,0, 0,0,0,0, 1,0,0,0, 0,0,0,0}, /* pattern 2 : basis */
    {0,0,0,0, 1,0,0,0, 1,0,0,0, 0,0,0,0} /* pattern 3 : result */	
};

#endif