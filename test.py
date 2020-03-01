import test_helper
import sys

# parameters
compressed_timestamp_file = "./compressed_timestamp"
compressed_detector_file = "./compressed_detector"
decompressed_timestamp_file = "./decompressed_timestamp"
decompressed_detector_file = "./decompressed_detector"
recombined_file = "./recombined"

readevent_file = sys.argv[1]

clock_bitwidth = "49"
detector_bitwidth = "4"
protocol = "1"

# 0. run raw data through readevents to get file
# execution time

# 1. compress file

print("original file checksum: ", test_helper.md5sum(readevent_file))

original_file_size = test_helper.get_file_size(readevent_file)
print("original file size (in bytes): ", original_file_size)

# execution time
test_helper.compress(readevent_file, compressed_timestamp_file, compressed_detector_file, clock_bitwidth, detector_bitwidth, protocol)

compressed_timestamp_file_size = test_helper.get_file_size(compressed_timestamp_file)
print("compressed timestamp file size (in bytes): ", compressed_timestamp_file_size)

compressed_detector_file_size = test_helper.get_file_size(compressed_detector_file)
print("compressed detector file size (in bytes): ", compressed_detector_file_size)

# compressed total filesize
compressed_file_size = compressed_timestamp_file_size + compressed_detector_file_size
print("compressed total file size: ", compressed_file_size)

# compression ratio
print("compression ratio: ", original_file_size / compressed_file_size)

# 2. decompress file
# decompress timestamp differences
test_helper.decompress(compressed_timestamp_file, decompressed_timestamp_file, clock_bitwidth, protocol, 1)

# decompress detector
test_helper.decompress(compressed_detector_file, decompressed_detector_file, detector_bitwidth, protocol, 0)

# recombine decompressed timestamp and detector
test_helper.recombine(decompressed_timestamp_file, decompressed_detector_file, clock_bitwidth, detector_bitwidth, recombined_file)

# recombined file checksum
print("recombined file checksum: ", test_helper.md5sum(recombined_file))

# 3. test
# assert original file checksum == recombined file checksum
