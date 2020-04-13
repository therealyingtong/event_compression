all: compress decompress readevents4 simulator

compress: compress.c write_utils.h
	gcc -w -O3 -o compress compress.c -lm

decompress: decompress.c read_utils.h
	gcc -w -O3 -o decompress decompress.c -lm

readevents4: readevents4.c
	gcc -w -O3 -o readevents4 readevents4.c -lm

simulator: simulator.c
	gcc -w -O3 -o simulator simulator.c -lm

read_binary: read_binary.c
	gcc -w -O3 -o read_binary read_binary.c -lm
