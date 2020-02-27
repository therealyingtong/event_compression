#include <stdio.h>

long long read_bits_from_word(long long word, char bitwidth, char start_idx){
	// assume lsb on left, msb on right

	long long bitmask = ((long long) 1 << bitwidth) - 1; // make bitmask for specified bitwidth
	char shift = 64 - bitwidth - start_idx;
	long long shifted_bitmask = bitmask << shift;
	long long bitstring = (word & shifted_bitmask) >> shift;
	return bitstring;
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

int main(){
	long long word = ((long long) 1 << 62) - 9;
	ll_to_bin(word);
	long long bitstring = read_bits_from_word(word, 10, 0);
	ll_to_bin(bitstring);
}
