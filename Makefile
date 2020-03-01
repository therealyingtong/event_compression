all: compress decompress

compress: compress.c write_utils.h
	gcc -w -O3 -o compress compress.c -lm

decompress: decompress.c read_utils.h
	gcc -w -O3 -o decompress decompress.c -lm
