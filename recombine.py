import sys
import numpy as np 

# parameters
decompressed_timestamp_file = sys.argv[1]
decompressed_detector_file = sys.argv[2]
clock_bitwidth = sys.argv[3]
detector_bitwidth = sys.argv[4]
recombined_file = sys.argv[5]

# process timestamps
decompressed_timestamp = open(decompressed_timestamp_file, "r")
time_diffs = np.fromfile(decompressed_timestamp, dtype=np.uint64)
time_diffs = time_diffs[0: len(time_diffs) - 2] # ignore last time_diff

timestamps = np.empty(len(time_diffs))

timestamps[0] = time_diffs[0] # first timestamp

for i in range(1, len(time_diffs)):
	timestamps[i] = time_diffs[i] + timestamps[i - 1]

# process detectors
decompressed_detectors = open(decompressed_detector_file, "r")
detectors = np.fromfile(decompressed_detectors, dtype=np.uint64)
print("detectors[0:100]", detectors[0:100])

# combine timestamps and detectors
combined_words = np.empty(len(time_diffs))
print("len(combined_words)", len(combined_words))
print("len(detectors): ", len(detectors))

with open(recombined_file, 'wb') as file:

	for i in range(0, len(combined_words)):
		# shift timestamps[i] left by (64 - clock_bitwidth) bits
		shifted_timestamp = (timestamps[i]) << (64 - int(clock_bitwidth))

		# no need to shift detectors[i] since they will be lsb

		# OR timestamps[i] and detectors[i]
		recombined_word = shifted_timestamp | (detectors[i])

		file.write(int(recombined_word).to_bytes(8, byteorder='big', signed=False))
