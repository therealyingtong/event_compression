#ifndef _COMPRESSION_STRUCTS_H
#define _COMPRESSION_STRUCTS_H 1

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

/* structures for output buffer headers */
typedef struct header_2 { /* header for type-2 stream packet */
    int tag;  
    unsigned int epoch;
    unsigned int length;
    int time_order;
    int base_bits;
    int protocol;
};

typedef struct header_3 {/* header for type-3 stream packet */
    int tag;
    unsigned int epoch;
    unsigned int length;
    int bits_per_entry; 
};


#define TYPE_1_TAG 1
#define TYPE_1_TAG_U 0x101
#define TYPE_2_TAG 2
#define TYPE_2_TAG_U 0x102
#define TYPE_3_TAG 3
#define TYPE_3_TAG_U 0x103
#define TYPE_4_TAG 4
#define TYPE_4_TAG_U 0x104

#define TYPE2_ENDWORD 1  /* shortword to terminate a type 2 stream */

/* lookup table for correction */
#define PL2 0x20000  /* + step fudge correction for epoc index mismatch */
#define MI2 0xfffe0000 /* - step fudge correction */
unsigned int overlay_correction[16]= {
	0,0,0,PL2,  
	0,0,0,0,
	MI2,0,0,0,  
	MI2,MI2,0,0
};

#endif