all: compression.c
	gcc -Wall -O3 -o compression compression.c -lm