import sys
import numpy as np 
from bitstring import BitArray

# parameters
decompressed_timestamp_file = sys.argv[1]
decompressed_detector_file = sys.argv[2]
clock_bitwidth = sys.argv[3]
detector_bitwidth = sys.argv[4]
recombined_file = sys.argv[5]
num_events = int(sys.argv[6])

# process timestamps
decompressed_timestamp = open(decompressed_timestamp_file, "r")
time_diffs = np.fromfile(decompressed_timestamp, dtype=np.uint64)
time_diffs = time_diffs[0 : num_events] 
timestamps = np.empty(len(time_diffs))

timestamps[0] = time_diffs[0] # first timestamp

for i in range(1, len(time_diffs)):
	timestamps[i] = time_diffs[i] + timestamps[i - 1]

# process detectors
decompressed_detectors = open(decompressed_detector_file, "r")
detectors = np.fromfile(decompressed_detectors, dtype=np.uint64)
detectors = detectors[0 : num_events]

# combine timestamps and detectors
combined_words = np.empty(len(time_diffs))
print("len(combined_words)", len(combined_words))
print("len(detectors): ", len(detectors))

with open(recombined_file, 'wb') as file:

	for i in range(0, len(combined_words)):
		# shift timestamps[i] left by (64 - clock_bitwidth) bits
		# print("timestamp before shift: ", timestamps[i])
		shifted_timestamp = np.uint64(timestamps[i]) << np.uint64(64 - int(clock_bitwidth))
		# print("timestamp after shift: ", shifted_timestamp)

		# no need to shift detectors[i] since they will be lsb

		# OR timestamps[i] and detectors[i]
		recombined_word = shifted_timestamp | (detectors[i])
		# print("recombined_word: ", recombined_word)

		byte_array = int(recombined_word).to_bytes(8, byteorder='little', signed=False)
		rearranged_byte_array = byte_array[4:8] + byte_array[0:4]


		file.write(rearranged_byte_array)

		# print(i)
		# # print(byte_array)
		# print(bin(int.from_bytes(rearranged_byte_array, byteorder='big', signed=False)) )

