import os
import subprocess
import config
import signal

def create_data_folders(processed_data_root, folder_array):
    
	if os.path.exists(processed_data_root):
		print ("conflict with existing data folder.",
				"please provide a new data root folder name")
		return False
	else:
		os.mkdir(processed_data_root)
		print ("data root created.")
		
	for folder in folder_array:
		os.mkdir(folder)

	print ("data sub directories created.")

	return True

def log(s, printlog = True):
	with open(config.processed_data_root + "/log.txt", "w") as log_file:

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

def chop_bob(bob_readevent_file, bob_t2, bob_t3_outcome, remotecrypto_folder):
    log("\n chopper on bob readevent files")
    log("at :"+ bob_readevent_file)
    chop_bob_command = remotecrypto_folder+"/chopper -i "+ bob_readevent_file+" -D "+ bob_t2 +" -d "+ bob_t3_outcome + " -U"
    run_sub_process(chop_bob_command)

def chop2_alice(alice_readevent_file, alice_t1, remotecrypto_folder):
    log("\n chopper2 on Alice readevent files")
    log("at :"+ alice_readevent_file)
    chop2_alice_command = remotecrypto_folder+"/chopper2 -i "+ alice_readevent_file+" -D " + alice_t1 + " -U "
    log(chop2_alice_command)
    #run_sub_process(chop2_alice_command,process_time_limit)
    os.system(chop2_alice_command)
    #print (chop2_alice_command)

def p_find(epoch, numepochs, alice_t1, bob_t2, remotecrypto_folder):
	if "0x" not in epoch.lower():
		xepoch = "0x"+str(epoch)
	log ("\n running pfind\n ")
	pfind_command = remotecrypto_folder + "/pfind -D " + alice_t1 + " -e "\
		+ str(xepoch)+ " -d " + bob_t2 + " -r 2 -n " + str(numepochs) + " -V 3 -q 22"
	log (pfind_command)
	diff = subprocess.Popen(pfind_command.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	return iter(diff.stdout.readline, b'')
	# os.system(pfind_command)

def p_find_blurb(epoch, alice_t1, bob_t2, remotecrypto_folder):
    if "0x" not in epoch.lower():
        xepoch = "0x"+str(epoch)
    log ("\n running pfind\n ")
    pfindB_command = remotecrypto_folder+"/pfindblurb -D "+alice_t1+" -e "\
        + str(xepoch)+ " -d " + bob_t2 + " -r 2 -n 10 -V 3 -q 20"
    log (pfindB_command)
    os.system(pfindB_command)

def co_stream(epoch, t_diff ,epochnum, alice_t1, alice_t3, alice_t4, bob_t2, remotecrypto_folder):
    if "0x" not in epoch.lower():
        xepoch = "0x"+str(epoch)
    log ("\n running costream\n ")
    costream_command = remotecrypto_folder+"/costream -D "+alice_t1 \
        + " -d "+ bob_t2 + " -F " + alice_t4+" -f " + alice_t3 \
        + " -e " + str(xepoch) + " -w 16 -u 40 -p 1 -q 10 -t " + str(t_diff) + "-T 0 -V 4 -G 3" + " -q " + str(epochnum)
    log(costream_command)
    os.system(costream_command)

def splicer(bob_t3_outcome, alice_t4, bob_t5, bob_t3_rawkey, startepoch, epochnum, remotecrypto_folder):
	if "0x" not in startepoch.lower():
		xepoch = "0x"+str(startepoch)
	log ("\n running splicer \n")
	splicer_command = remotecrypto_folder + "/splicer -d " + bob_t3_outcome + " -D " + alice_t4 + " -B " + bob_t5 + " -f " + bob_t3_rawkey + " -e " + xepoch + " -q " + str(epochnum)
	log(splicer_command)
	os.system(splicer_command)

def transferd(srcdir, commandpipe, target, destdir, notify, ec_in_pipe, ec_out_pipe, port, target_port, remotecrypto_folder):
	log("\n running transferd \n ")
	log("mkfifo " + commandpipe)
	os.system("mkfifo " + commandpipe)
	log("mkfifo " + ec_in_pipe)
	os.system("mkfifo " + ec_in_pipe)
	log("mkfifo " + ec_out_pipe)
	os.system("mkfifo " + ec_out_pipe)
	
	transferd_command = remotecrypto_folder + "/transferd -d " + srcdir + " -c " + commandpipe + " -t " + target + " -D " + destdir + " -l " + notify + " -e " + ec_in_pipe + " -E " + ec_out_pipe + " -p " + port + " -P " + target_port
	os.system('gnome-terminal -- bash -c "' + "echo '" + transferd_command + "' && " + transferd_command + '; exec bash" & ')
	

def ecd2(commandpipe, sendpipe, receivepipe, rawkey_dir, finalkey_dir, notificationpipe, querypipe, respondpipe, errorcorrection_folder):
	log("mkfifo " + commandpipe)
	os.system("mkfifo " + commandpipe)
	# log("mkfifo " + sendpipe)
	# os.system("mkfifo " + sendpipe)
	# log("mkfifo " + receivepipe)
	# os.system("mkfifo " + receivepipe)
	log("mkfifo " + notificationpipe)
	os.system("mkfifo " + notificationpipe)
	log("mkfifo " + querypipe)
	os.system("mkfifo " + querypipe)
	log("mkfifo " + respondpipe)
	os.system("mkfifo " + respondpipe)
	
	log("\n running ecd2\n ")
	ecd2_command = errorcorrection_folder + "/ecd2 -c " + commandpipe + " -s " + sendpipe + " -r " + receivepipe + " -d " + rawkey_dir + " -f " + finalkey_dir + " -l " + notificationpipe + " -Q " + querypipe + " -q " + respondpipe + " -V 5 "
	os.system('gnome-terminal -- bash -c "' + "echo '" + ecd2_command + "' && " + ecd2_command + '; exec bash" & ')
