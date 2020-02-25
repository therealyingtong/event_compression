all: compression decompression

compression: compression.c
	gcc -w -O3 -o compression compression.c -lm

decompression: decompression.c
	gcc -w -O3 -o decompression decompression.c -lm
