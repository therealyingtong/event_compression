import os
import subprocess
import signal
from hashlib import md5

def md5sum(filename):
    hash = md5()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(128 * hash.block_size), b""):
            hash.update(chunk)
    return hash.hexdigest()

def get_file_size(filename):
	return os.stat(filename).st_size

def log(s, printlog = True):
	with open("./log.txt", "w") as log_file:

		if printlog is True:
			print(str(s))
		log_file.write(str(s)+'\n')
		log_file.flush()

def run_sub_process(command, timelimit = 5):
	log("\n running:"+ command + "\n ")

	# process = subprocess.Popen([command], shell=True)
	process = subprocess.Popen(command.split(), shell=False)

	try:
		log('\n Running in process:'+str( process.pid))
		process.wait(timeout=timelimit)
		# outs, errs = process.communicate(timeout=timelimit)
	except subprocess.TimeoutExpired:
		log('\n Timed out - killing '+ str(process.pid) )
		process.kill()
		# outs, errs = process.communicate()
	log("\n Done \n ")

def compress(readevent_file, compressed_timestamp_file, compressed_detector_file, clock_bitwidth, detector_bitwidth, protocol):
    log("\n compressing " + readevent_file)
    compress_command = "./compress -i " + readevent_file + " -o " + compressed_timestamp_file + " -O " + compressed_detector_file + " -c " + clock_bitwidth + " -d " + detector_bitwidth + " -p " + protocol
    run_sub_process(compress_command)

def decompress(compressed_file, decompressed_file, init_bitwidth, protocol, adaptive):
	log("\n decompressing " + compressed_file)
	decompress_command = "./decompress -i " + compressed_file + " -o " + decompressed_file + " -b " + init_bitwidth + " -p " + protocol
	if (adaptive): decompress_command += " -a "
	run_sub_process(decompress_command)

def recombine(decompressed_timestamp_file, decompressed_detector_file, clock_bitwidth, detector_bitwidth, recombined_file, num_events):
	log("\n recombining " + decompressed_timestamp_file + " and " + decompressed_detector_file)
	recombine_command = "python3 recombine.py " + decompressed_timestamp_file + " " + decompressed_detector_file + " " + clock_bitwidth + " " + detector_bitwidth + " " + recombined_file + " " + num_events
	run_sub_process(recombine_command)
