## 0. make all C executables
`make`

## 1. set up pseudo terminals
these will spoof serial input (originally from the USB timestamp card reader)
`socat PTY,link=/tmp/ttyS10,raw,echo=0 PTY,link=/tmp/ttyS11,raw,echo=0 &`

## 2. link readevents4.c to first address, redirect output to named pipe
`./readevents4 -U /tmp/ttyS10 -a 1 -s ./readevents_pipe -X &`

## 3. pipe simulated events to second address
(in a new terminal)
`./simulator -c 5000000 -d 3 -r 2 -o /tmp/ttyS11`

## 4. read simulated events from readevents4 into compression
(in a new terminal)
`./compress -i readevents_pipe -o compressed_time_stream -O compressed_detector_stream -c 45 -d 4 -e 14 -p 1`