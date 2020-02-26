all: compression decompression

compression: compression.c compression_utils.h
	gcc -W -O3 -o compression compression.c -lm

decompression: decompression.c compression_utils.h
	gcc -w -O3 -o decompression decompression.c -lm