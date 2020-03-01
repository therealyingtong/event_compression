all: compress decompress

compress: compress.c compression_utils.h
	gcc -w -O3 -o compress compress.c -lm

decompress: decompress.c decompression_utils.h
	gcc -w -O3 -o decompress decompress.c -lm
