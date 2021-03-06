#ifndef _MATH_UTILS_H
#define _MATH_UTILS_H 1

#include <stdio.h>

unsigned long long read_bits_from_word(unsigned long long word, unsigned char bitwidth, unsigned char start_idx){
	// assume lsb on left, msb on right

	unsigned long long bitmask = ((unsigned long long) 1 << bitwidth) - 1; // make bitmask for specified bitwidth
	unsigned char shift = 64 - bitwidth - start_idx;
	unsigned long long shifted_bitmask = bitmask << shift;
	unsigned long long bitstring = (word & shifted_bitmask) >> shift;
	return bitstring;
}


/* helper for name. adds a slash, hex file name and a termial 0 */
unsigned char hexdigits[]="0123456789abcdef";
void atohex(unsigned char* target,unsigned int v) {
    int i;
    target[0]='/';
    for (i=1;i<9;i++) target[i]=hexdigits[(v>>(32-i*4)) & 15];
    target[9]=0;
}

int ll_to_bin(unsigned long long n)
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

#endif