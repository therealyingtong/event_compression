#ifndef _COMPRESSION_STRUCTS_H
#define _COMPRESSION_STRUCTS_H 1

typedef struct raw_event {
	unsigned int msw; /* most significant word (32 bits) */
    unsigned int lsw; /* least significant word (32 bits) */
};

/* protocol definitions */
typedef struct protocol {
    int bits_per_entry_2;
    int bits_per_entry_3;
    int detector_entries; /* number of detector entries; 16 for 4 detectors; this value -1 is used as bitmask for states */
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

#endif