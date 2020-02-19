import qcrypto_helper
import config
import sys
import os
import subprocess

# Alice is on satellite (right next to the source)
# Bob is on groundstation

alice_readevent_file =  sys.argv[1]
bob_readevent_file =  sys.argv[2]
remotecrypto_folder = config.remotecrypto_folder
errorcorrection_folder = config.errorcorrection_folder
alice_ip = config.alice_ip 
alice_port = config.alice_port
bob_ip = config.bob_ip
bob_port = config.bob_port

# global variables
process_time_limit = 5 # seconds
processed_data_root = config.processed_data_root

# folders for processed data
alice_folder = processed_data_root + "/alice"
bob_folder = processed_data_root + "/bob"

alice_t1 = alice_folder + "/t1"
alice_t3 = alice_folder + "/t3"
alice_t4 = alice_folder + "/t4"
alice_t7 = alice_folder + "/t7"

bob_t2 = bob_folder + "/t2"
bob_t3_outcome = bob_folder + "/t3_outcome"
bob_t5 = bob_folder + "/t5"
bob_t3_rawkey = bob_folder + "/t3_rawkey"
bob_t7 = bob_folder + "/t7"

# clear former folders, pipes, ports
clear_folder = "rm -rf " + processed_data_root
os.system(clear_folder)
clear_pipes = "find . -type p -delete"
os.system(clear_pipes)
kill_ports = "fuser -k (" + alice_port + "/tcp)" + " && " + "fuser -k (" + bob_port + "/tcp)"
os.system(kill_ports)

# create folders
qcrypto_helper.create_data_folders(
	processed_data_root, 
	[alice_folder, alice_t1, alice_t3, alice_t4, alice_t7, 
	bob_folder, bob_t2, bob_t3_outcome, bob_t5, bob_t3_rawkey, bob_t7]
)

# log commands
qcrypto_helper.log(processed_data_root)

# chopper on Bob's data, saves private outcome in t3, saves public basis and timestamp in t2
qcrypto_helper.chop_bob(bob_readevent_file, bob_t2, bob_t3_outcome, remotecrypto_folder)

# chopper2 on Alice's data, saves info in t1
qcrypto_helper.chop2_alice(alice_readevent_file, alice_t1, remotecrypto_folder)

# p_find using Alice's data and Bob's transmitted public data
first_epoch = str(sorted(os.listdir(alice_t1))[0])
num_epochs = min(len(os.listdir(alice_t1)), len(os.listdir(bob_t2))) - 2
print('num_epochs', num_epochs)

pfind_output = qcrypto_helper.p_find(first_epoch, num_epochs, alice_t1, bob_t2, remotecrypto_folder) 
diff = 0

for line in pfind_output:
	print(str(line, 'utf-8')) 
	if (str(line, 'utf-8').find("difference") >= 0):
		diff = ((str(line).split(": ")[1]).split()[0]).strip()

diff = int(diff)

# costream to get Alice's raw key (t3) and coincidence events (t4)
qcrypto_helper.co_stream(first_epoch, diff, num_epochs - 1, alice_t1, alice_t3, alice_t4, bob_t2, remotecrypto_folder)

# splicer on Bob's side
qcrypto_helper.splicer(bob_t3_outcome, alice_t4, bob_t5, bob_t3_rawkey, first_epoch, num_epochs - 1, remotecrypto_folder)

# transferd on Alice's side
alice_command = "alice_command"
alice_ec_in = "alice_ec_in"
alice_ec_out = "alice_ec_out"
alice_transferd_log = alice_folder + "/alice_transferd_log"

qcrypto_helper.transferd(
	alice_folder, alice_command, bob_ip, bob_folder, alice_transferd_log, alice_ec_in, alice_ec_out, alice_port, bob_port, remotecrypto_folder
)

# transferd on Bob's side
bob_command = "bob_command"
bob_ec_in = "bob_ec_in"
bob_ec_out = "bob_ec_out"
bob_transferd_log = bob_folder + "/bob_transferd_log"

qcrypto_helper.transferd(
	bob_folder, bob_command, alice_ip, bob_folder, bob_transferd_log, bob_ec_in, bob_ec_out, bob_port, alice_port, remotecrypto_folder
)

# ecd2 on Alice's side
alice_first_epoch = str(os.listdir(alice_t3)[0])
alice_num_blocks = len(os.listdir(alice_t3)) - 2
alice_ec_commandpipe = "alice_ec_commandpipe"

command = "echo '" + str(alice_first_epoch) + " " + str(alice_num_blocks) + "\n' > " + alice_ec_commandpipe

sendpipe = alice_ec_in
receivepipe = alice_ec_out
rawkeydirectory = alice_t3
finalkeydirectory = alice_t7
notificationpipe = "alice_notif"
querypipe = "alice_query"
respondpipe = "alice_respond"

qcrypto_helper.ecd2(alice_ec_commandpipe, sendpipe, receivepipe, rawkeydirectory, finalkeydirectory, notificationpipe, querypipe, respondpipe, errorcorrection_folder)

# ecd2 on Bob's side
bob_first_epoch = str(os.listdir(bob_t3_rawkey)[0])
bob_num_blocks = len(os.listdir(bob_t3_rawkey)) - 2
bob_ec_commandpipe = "bob_ec_commandpipe"

command = "echo '" + str(bob_first_epoch) + " " + str(bob_num_blocks) + "\n' > " + bob_ec_commandpipe

sendpipe = bob_ec_in
receivepipe = bob_ec_out
rawkeydirectory = bob_t3_rawkey
finalkeydirectory = bob_t7
notificationpipe = "bob_notif"
querypipe = "bob_query"
respondpipe = "bob_respond"

qcrypto_helper.ecd2(bob_ec_commandpipe, sendpipe, receivepipe, rawkeydirectory, finalkeydirectory, notificationpipe, querypipe, respondpipe, errorcorrection_folder)

