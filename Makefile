all: compression.c
	gcc -w -O3 -o compression compression.c -lm