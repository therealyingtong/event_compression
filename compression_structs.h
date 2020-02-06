#ifndef _COMPRESSION_STRUCTS_H
#define _COMPRESSION_STRUCTS_H 1

#define DEFAULT_STATEMASK 0xf /* take last four bits of detector_value */

typedef struct raw_event {
	unsigned int _clock_value; /* most significant word */
    unsigned int _detector_value; /* least significant word */
};

/* protocol definitions */
typedef struct protocol {
    int bits_per_entry_2;
    int bits_per_entry_3;
    int detector_entries; /* number of detector entries; 16 for 4 detectors; this value -1 is used as bitmask for status */
    int num_detectors; /* to cater logging purposes for more than 4 detectors */
    int pattern2[16];
    int pattern3[16];
};

struct protocol protocol_list[] = {
	{
	/* protocol 0: all bits go everywhere */
		4,4,16,4,
		{0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf}, /* pattern 2 */
		{0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf} /* pattern 3 */
	},
	{
	/* standard BB84. assumed sequence:  (LSB) V,-,H,+ (MSB);
		HV basis: 0, +-basis: 1, result: V-: 0, result: H+: 1 */
		1,1,16,4,
		{0,0,1,0, 0,0,0,0, 1,0,0,0, 0,0,0,0}, /* pattern 2 : basis */
		{0,0,0,0, 1,0,0,0, 1,0,0,0, 0,0,0,0} /* pattern 3 : result */	
	}

};

#endif