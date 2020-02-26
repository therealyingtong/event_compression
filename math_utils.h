#ifndef _MATH_UTILS_H
#define _MATH_UTILS_H 1

long long read_bits_from_buffer(int64_t *current_word, int *bits_read, char bitwidth){
	char word_offset = *bits_read % 64; // start_idx in word
	long long bitstring;
	if (word_offset + bitwidth > 64){
		//overlap into next word
		char overlap_bitwidth = word_offset + bitwidth - 64;
		long long first_part_bitstring = read_bits_from_word(*current_word, 64 - word_offset, word_offset);

		*current_word++; // read next word

		long long second_part_bitstring = read_bits_from_word(*current_word, overlap_bitwidth, 0);

		bitstring = (first_part_bitstring << overlap_bitwidth) | second_part_bitstring;

	} else {
		// directly read bitstring
		bitstring = read_bits_from_word(*current_word, bitwidth, word_offset);
	}

	*bits_read += bitwidth;
	return bitstring;
}

long long read_bits_from_word(int64_t word, char bitwidth, char start_idx){
	// assume lsb on left, msb on right

	long long bitmask = ((long long) 1 << bitwidth) - 1; // make bitmask for specified bitwidth
	char shift = 64 - bitwidth - start_idx;
	long long shifted_bitmask = bitmask << shift;
	long long bitstring = (word & shifted_bitmask) >> shift;
	return bitstring;
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

#endif