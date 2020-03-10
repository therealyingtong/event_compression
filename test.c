#include <stdio.h>

unsigned char compressed_file[64] = "./compressed_detector";
int fd;

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

	FILE *fcompressed;
	unsigned long long word;
	if ((fcompressed = fopen(compressed_file, "rb")) == NULL) return 0;

	while (fread(&word, sizeof(word), 1, fcompressed) == 1) ll_to_bin(word);

}
