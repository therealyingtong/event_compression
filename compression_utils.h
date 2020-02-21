#ifndef _COMPRESSION_UTILS_H
#define _COMPRESSION_UTILS_H 1

#include "compression_config.h"
#include "compression_structs.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

int encode_t_diff(long long t_diff, int t_diff_bitwidth, unsigned int *outbuf2, int *outbuf2_free, int64_t *sendword2){

	// left-append t_diff to outbuf2 at outbuf2_free
	*outbuf2 = *outbuf2 | (t_diff << *outbuf2_free);

	// decrease outbuf2 free pointer
	*outbuf2_free -= t_diff_bitwidth;

	// returns 1 if sendword2 is full
	// returns 0 otherwise

	return 0;

}

int encode_large_t_diff(long long t_diff, int t_diff_bitwidth, unsigned int *outbuf2, int *outbuf2_free, int64_t *sendword2){

	// pad t_diff to t_diff_bitwidth size
	t_diff = t_diff << (64 - t_diff_bitwidth);

	// write padded_t_diff at outbuf2_free
	*outbuf2 = *outbuf2 | (t_diff >> *outbuf2_free);

	// returns 1 if sendword2 is full
	// returns 0 otherwise



	return 0;

}


/* helper for name. adds a slash, hex file name and a termial 0 */
char hexdigits[]="0123456789abcdef";
void atohex(char* target,unsigned int v) {
    int i;
    target[0]='/';
    for (i=1;i<9;i++) target[i]=hexdigits[(v>>(32-i*4)) & 15];
    target[9]=0;
}

int ll_to_bin(long long n)
{
  int c, k;
  printf("%lld in binary is:\n", n);
  for (c = 63; c >= 0; c--)
  {
    k = n >> c; 
    if (k & 1) printf("1");
    else printf("0");
  }
  printf("\n");
  return 0;
}

int int_to_bin(int n)
{
  int c, k;
  printf("%d in binary is:\n", n);
  for (c = 31; c >= 0; c--)
  {
    k = n >> c;
    if (k & 1) printf("1");
    else printf("0");
  }
  printf("\n");
  return 0;
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

/* prepare first epoch information; should lead to a stale file in the past;
   the argument is the delay from the current time in seconds */
unsigned int make_first_epoch(int delay) {
    unsigned long long aep;
    aep = ((time(NULL)-delay)*1953125)>>20; /* time in epocs */
    return (unsigned int) (aep & 0xffffffff);
}

/* opening routine to target files & epoch construction */
int open_epoch(
	unsigned int t_epoch, 
	struct header_2 *head2, 
	struct header_3 *head3,
	int type2bitwidth,
	int type2datawidth,
	int type3datawidth,
	int protocol_idx) {

    unsigned int final_epoch = t_epoch;

    /* populate headers preliminary */

    head2 -> tag = TYPE_2_TAG; 
	head2 -> length = 0;
    head2 -> time_order = type2bitwidth; 
	head2 -> base_bits = type2datawidth;
    head2 -> epoch = final_epoch;
    head2 -> protocol = protocol_idx;

    head3 -> tag = TYPE_3_TAG; 
	head3 -> length = 0;
    head3 -> epoch = final_epoch; 
	head3 -> bits_per_entry = type3datawidth;

    return 0;
}


#endif