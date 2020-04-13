// simulates timestamp card by outputting 32-bit words to a serial port

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>


float nextTime(float rate)
{
    return -logf(1.0f - (float) random() / (RAND_MAX)) / rate;
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


// format of 32-bit word:
char clock_bitwidth = 27; // leftmost bits are clock value
char detector_bitwidth = 4; // rightmost bits are detector value
// middle bits are dummy bits

float count_rate = 5000000; // in counts per second
int resolution = 2.0; // in nanoseconds
int duration = 3; // in seconds
float time = 0;
float offset;
char detector;

char outfile[200];

int main( int argc, char *argv[] )
{
	int opt;
	while ((opt=getopt(argc, argv, "c:d:r:o:")) != EOF){

		switch(opt){
		case 'c': /* parse infile */
			if (1 != sscanf(optarg, "%f", &count_rate)) printf("failed to parse count_rate");
			break;

		case 'd': /* parse infile */
			if (1 != sscanf(optarg, "%d", &duration)) printf("failed to parse duration");
			break;

		case 'r': /* parse infile */
			if (1 != sscanf(optarg, "%d", &resolution)) printf("failed to parse resolution");
			break;

		case 'o': /* parse infile */
			if (1 != sscanf(optarg, "%200s", outfile)) printf("failed to parse filename");
			break;
		}		
	}

	int fd = open(outfile, O_RDWR);

	for (int i = 0; i < count_rate * duration; i++){

		int word = 0;

		// random time offset
		offset = nextTime(count_rate) / (float) resolution * pow(10.0, 9.0);

		// update time value
		time += offset;

		// encode time value in 27 bits at the start of word
		word |= ( (int) time << (32 - clock_bitwidth));

		// random detector value
		detector = 1 << (rand() % 4); 

		// encode detector value in 4 bits at the end of word
		word |= detector;

		// write word to output
		write(fd, &word, 4);
		// ll_to_bin(word);

	}

	return 0;
}