/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

#include "hxediag.h"


volatile unsigned int exit_flag = 0;
/******************************************************************
 * Entry Point : 
 * Main program for testing NICs in diagnostic mode using ethtool
 * infrastructure 
 *****************************************************************/
int 
main(int argc, char ** argv) {  

	/*********************************************************************
	 * Command line arguments passed to exerciser 
     ********************************************************************/ 
	char exer_name[MAX_STRING_SIZE];									/* Exerciser binary name */ 
	char dev_name[MAX_STRING_SIZE]; 									/* Device to exerciser */ 
	char run_type[MAX_STRING_SIZE];										/* Invocation type : OTH - commandline, REG -HTX SUP invoked exerciser */ 
	char rules[MAX_STRING_SIZE];										/* Rules file to read testcase parameters */ 

	/********************************************************************** 
	 * Generic exerciser variables .. 
	 *********************************************************************/ 
	int rc = 0;											/* Captures return code */ 
	int i = 0, tc = 0;									/* Loop counter 		*/ 
	char msg[MSG_TEXT_SIZE], stat_msg[MSG_TEXT_SIZE];
	unsigned long long failed_tc = 0, passed_tc = 0, total_tc = 0; 
	float percent_pass = 0, percent_fail = 0; 
	
	
	char device[128], port_state[128];;	
	char command[MSG_TEXT_SIZE], server[MSG_TEXT_SIZE], client[MSG_TEXT_SIZE], hostname[MSG_TEXT_SIZE], ber_test[MSG_TEXT_SIZE];
	char hca_selftest[MSG_TEXT_SIZE];
	int num_errors = 0; 
	int retry ;

	/**********************************************************************
	 * file descriptor for device to be exercised 
	 *********************************************************************/ 
	int fd = -1; 

	/**********************************************************************
     * HTX data structure : used to communicate with HTX Supervisor 
     *********************************************************************/ 
	struct 	htx_data htx_d; 

	/*********************************************************************
 	 * Rules file stuff .. 
 	 * ******************************************************************/ 
	unsigned int num_stanzas = 0; 
	struct rule_info rf_info[MAX_TC];
	struct rule_info * current_stanza;

	/*********************************************************************
	 * Signal handler register mechanism 
	 * ******************************************************************/ 
	struct sigaction sigvector;

	/********************************************************************* 
	 * Struct ethtool_drvinfo - general driver and device information
 	 * @cmd: Command number = %ETHTOOL_GDRVINFO
 	 * @driver: Driver short name.  This should normally match the name
 	 *      in its bus driver structure (e.g. pci_driver::name).  Must
 	 *      not be an empty string.
 	 * @version: Driver version string; may be an empty string
 	 * @fw_version: Firmware version string; may be an empty string
 	 * @bus_info: Device bus address.  This should match the dev_name()
 	 *      string for the underlying bus device, if there is one.  May be
 	 *      an empty string.
 	 * @n_priv_flags: Number of flags valid for %ETHTOOL_GPFLAGS and
 	 *      %ETHTOOL_SPFLAGS commands; also the number of strings in the
 	 *      %ETH_SS_PRIV_FLAGS set
 	 * @n_stats: Number of u64 statistics returned by the %ETHTOOL_GSTATS
 	 *      command; also the number of strings in the %ETH_SS_STATS set
 	 * @testinfo_len: Number of results returned by the %ETHTOOL_TEST
 	 *      command; also the number of strings in the %ETH_SS_TEST set
 	 * @eedump_len: Size of EEPROM accessible through the %ETHTOOL_GEEPROM
 	 *      and %ETHTOOL_SEEPROM commands, in bytes
 	 * @regdump_len: Size of register dump returned by the %ETHTOOL_GREGS
 	 *      command, in bytes
	 ********************************************************************/ 
	struct ethtool_drvinfo drvinfo; 

	/*********************************************************************
	 * Structure ethtool_gstrings:  string set for data tagging
 	 * @cmd: Command number = %ETHTOOL_GSTRINGS
 	 * @string_set: String set ID; one of &enum ethtool_stringset
 	 * @len: On return, the number of strings in the string set
 	 * @data: Buffer for strings.  Each string is null-padded to a size of
 	 *      %ETH_GSTRING_LEN.
 	 *
	 *********************************************************************/ 
	struct ethtool_gstrings * strings; 

	/*********************************************************************
	 * Structure ethtool_test : device self-test invocation 
 	 * @cmd: Command number = %ETHTOOL_TEST
 	 * @flags: A bitmask of flags from &enum ethtool_test_flags.  Some
 	 *      flags may be set by the user on entry; others may be set by
 	 *      the driver on return.
 	 * @len: On return, the number of test results
 	 * @data: Array of test results
 	 *
	 *********************************************************************/ 
	struct ethtool_test * test; 

	 /********************************************************************
 	 * keep track of test failure ratio 
 	 *********************************************************************/
	struct self_test result;
	char supported_test[MAX_TEST][MSG_TEXT_SIZE]; 

	char htx_path[MSG_TEXT_SIZE];

	/*********************************************************************
	*Glacier park client device details
	*********************************************************************/
	char *string_pos;
	char client_device[10];	
	int client_device_len;

	if(argc < 4) { 
		printf("Usage : hxediag <dev_name> <run_type> <rules_file>\n"); 
		printf("Ex : <hxediag_binary_name> <Device_name> <REG/OTH> <rules_file> \n"); 
		return(0); 
	}
	/**********************************************************************
	 * Get commandline arguments in local variables 
	 *********************************************************************/ 
	if(argv[0]) strncpy(exer_name, argv[0], MAX_STRING_SIZE); 
	if(argv[1]) strncpy(dev_name, argv[1], MAX_STRING_SIZE); 
	if(argv[2]) strncpy(run_type, argv[2], MAX_STRING_SIZE); 
	if(argv[2]) strncpy(rules, argv[3], MAX_STRING_SIZE); 
	
	/*********************************************************************
	 * Intialize HTX data structure .. 
	 ********************************************************************/ 
	memset(&htx_d, 0, sizeof(struct  htx_data)); 
	strcpy(htx_d.HE_name, exer_name); 
	strcpy(htx_d.sdev_id, dev_name); 
	strcpy(htx_d.run_type, run_type); 

	hxfupdate(START, &htx_d);

	/**********************************************************************
     * Dump start infomation in htxmsg 
	 *********************************************************************/ 
	sprintf(htx_d.msg_text, "argc %d  argv[0] %s argv[1] %s argv[2] %s argv[3] %s \n", argc, exer_name, dev_name, run_type, rules);
    hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text);

    /*********************************************************************
 	 * Register SIGTERM handler 
 	 * *******************************************************************/
    sigemptyset((&sigvector.sa_mask));
    sigvector.sa_flags = 0;
    sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

    /****************************************************************
     * Parse rules file arguments ....
     * *************************************************************/
    rc = rf_read_rules(rules, rf_info, &num_stanzas, &htx_d);
    if (rc) {
        sprintf(htx_d.msg_text, " Rule file parsing  failed! rc = %d", rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, htx_d.msg_text);
        return (rc);
    }

	if(device_type == ETHERNET || device_type == ROCE) { 
		/*********************************************************************
	 	 *  Open device to be queried 
	 	 * *******************************************************************/  
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(fd < 0) { 
			sprintf(htx_d.msg_text, "Cannot open device, rc = %d, errno = %s \n", fd, strerror(errno)); 
			hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text); 
			return(rc); 
		}	
		/*********************************************************************
	 	 * Verify if driver supports Diagnostic test 
	 	 ********************************************************************/
		memset(&drvinfo, 0, sizeof(struct ethtool_drvinfo)); 
		drvinfo.cmd = ETHTOOL_GDRVINFO;
		rc = issue_ioctl(fd, &drvinfo, &htx_d); 
		if(rc <0) { 
			sprintf(htx_d.msg_text, "ETHTOOL_GDRVINFO failed, exiting !! \n"); 
			hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
			close(fd); 
        	return(rc);
    	}

		sprintf(htx_d.msg_text, "Device=%s, driver = %s, bus = %s, supports diag test=%s \n",basename(htx_d.sdev_id), drvinfo.driver, drvinfo.bus_info, (drvinfo.testinfo_len ? "yes" : "no"));  
		hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text); 

		if(drvinfo.testinfo_len == 0) { 
			sprintf(htx_d.msg_text, "Appears you cannot run diagnostic test for this device, as device driver doesnot supports self test. \nThis instance would exit !! \n"); 
			hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text); 
			return(ENOSYS);
		}   

		strings = get_stringset(ETH_SS_TEST, offsetof(struct ethtool_drvinfo, testinfo_len), fd, &htx_d); 
		if(strings == NULL) { 
			sprintf(htx_d.msg_text, "Querying Self testypes failed !! \n"); 
			hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text); 
			return(-1); 
		}
		if(strings->len > MAX_TEST) { 
			sprintf(htx_d.msg_text, "This program needs to be updated with newly supported Diag Tests. \n current=%#x, supported=%#x \n", strings->len, MAX_TEST); 
			hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
			exit(-EOPNOTSUPP); 
		}

		sprintf(htx_d.msg_text, "Adapter Device driver Supports following diagnostic tests : \n"); 
    	for (i = 0; i < strings->len; i++) {
			strncpy(supported_test[i], strupr((char *)(strings->data + i * ETH_GSTRING_LEN)), 4096); 
        	sprintf(msg, "%s\n", (char *)supported_test[i]);
			strcat(htx_d.msg_text, msg); 
    	}
		hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text); 

    	/********************************************************************
 	 	 * Close device  descriptor
 	 	 *******************************************************************/
    	rc = close(fd);
    	if(fd < 0) {
        	sprintf(htx_d.msg_text, "Cannot close device, rc = %d, errno = %s \n", fd, strerror(errno));
        	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
        	return(rc);
    	}

    	fd = -1;

	    test = malloc(sizeof(struct ethtool_test) + strings->len * sizeof(unsigned long long));
    	if(test == NULL) {
        	sprintf(htx_d.msg_text, " malloc failed for test. errno = %s \n", strerror(errno));
        	hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
        	return(errno);
    	}

	} else if (device_type == INFINIBAND) { 

		/* 
		 * GLACIERPARK_1PORT is single port adapter, MFG is not expected to loop back connect
		 * So opensm is expected to fail 
		 */ 
		if(device_name != GLACIERPARK_1PORT) {  
			strncpy(device, basename(htx_d.sdev_id), 128); 

			if(getenv("HTXSCRIPTS") == NULL){
	    	    sprintf(htx_d.msg_text, "HTXSCRIPTS Environment variable is not set.Exiting code..\n");
				hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, htx_d.msg_text);
				exit(1);
			}else {
			    strcpy(htx_path, getenv("HTXSCRIPTS"));
			}

			for(i = 1; i <= num_ports; i ++) { 
                if((device_name == GLACIERPARK) && (i != 1)){
                    i = 1; /*For Glacier park 2 port device port number remains always 1.*/
					strcpy(client_device, device);
                    client_device_len = strlen(client_device);
					string_pos = client_device + client_device_len - 1;
					++*string_pos;
					sprintf(command, "%sib_info.pl -q -d %s -p %d 2>&1 \n", htx_path, client_device, i);
					if(DEBUG) printf("command = %s \n", command);

					/* Check if command can be successfully executed */
					rc = get_cmd_rc(command, &htx_d);
					if(rc == -1 || rc == 127 ) {
						sprintf(htx_d.msg_text, "cannot execute command = %s, rc = %#x\n", command, rc);
						hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
						exit(rc);
					} else if(rc) {
						sprintf(htx_d.msg_text, "command=%s failed with exit code %#x\n", command, rc);
						hxfmsg(&htx_d, rc,  HTX_HE_SOFT_ERROR, htx_d.msg_text);
						/* Run command with verbosity and dump it to user */
						sprintf(command, "%sib_info.pl -d %s -p %d 2>&1 \n",htx_path,client_device, i);
						if(DEBUG) printf("command = %s, rc = %#x \n", command, rc);
						get_cmd_result(command, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
						hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
						exit(rc);
					} else {
							/* Command successfully ran, parse results */
						get_cmd_result(command, port_state, MSG_TEXT_SIZE, &htx_d);
						if(DEBUG) printf("port_state = %s \n", port_state);
						if(strcmp(trim(port_state), "ACTIVE") == 0) {
							sprintf(htx_d.msg_text, "Device = %s, PortState=%s Starting MMIO, DMA Test .. on Port=%d \n", client_device, port_state, i);
							hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text);
						} else {
							sprintf(htx_d.msg_text, "Device = %s, Port state=%s for port=%d. No test running \n", client_device, port_state, i);
							hxfmsg(&htx_d, 0, HTX_HE_SOFT_ERROR, htx_d.msg_text);
							num_errors ++;
						}
					}
					break;
				}
				else {	
					sprintf(command, "%sib_info.pl -q -d %s -p %d 2>&1 \n",htx_path,device, i); 

					if(DEBUG) printf("command = %s \n", command); 
	
					/* Check if command can be successfully executed */ 	
					rc = get_cmd_rc(command, &htx_d); 
					if(rc == -1 || rc == 127 ) { 
						sprintf(htx_d.msg_text, "cannot execute command = %s, rc = %#x\n", command, rc); 
						hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
						exit(rc); 
					} else if(rc) { 
						sprintf(htx_d.msg_text, "command=%s failed with exit code %#x\n", command, rc); 
						hxfmsg(&htx_d, rc,  HTX_HE_SOFT_ERROR, htx_d.msg_text);
						/* Run command with verbosity and dump it to user */  
						sprintf(command, "%sib_info.pl -d %s -p %d 2>&1 \n",htx_path,device, i); 
						if(DEBUG) printf("command = %s, rc = %#x \n", command, rc); 
						get_cmd_result(command, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d); 	
						hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text); 
						exit(rc); 
					} else { 
						/* Command successfully ran, parse results */ 
						get_cmd_result(command, port_state, MSG_TEXT_SIZE, &htx_d); 
						if(DEBUG) printf("port_state = %s \n", port_state); 	
						if(strcmp(trim(port_state), "ACTIVE") == 0) { 
							sprintf(htx_d.msg_text, "Device = %s, PortState=%s Starting MMIO, DMA Test .. on Port=%d \n", device, port_state, i); 
							hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text); 
						} else { 
							sprintf(htx_d.msg_text, "Device = %s, Port state=%s for port=%d. No test running \n", device, port_state, i); 	
							hxfmsg(&htx_d, 0, HTX_HE_SOFT_ERROR, htx_d.msg_text);
							num_errors ++; 
						}
					}
				}	
			}
			if(num_errors == num_ports) { 
				sprintf(htx_d.msg_text, "Exiting exerciser as all ports in bad state \n"); 		
				hxfmsg(&htx_d, 0, HTX_HE_SOFT_ERROR, htx_d.msg_text);
				return(-1); 
			} 
		}
		sprintf(command, "cat /proc/sys/kernel/hostname 2>&1"); 
		memset(hostname, 0, MSG_TEXT_SIZE); 
		get_cmd_result(command, hostname, MSG_TEXT_SIZE, &htx_d); 
		if(DEBUG) printf(" hostname = %s \n", hostname); 
	}
	
	/****************************************************************
 	 * Intialize results .. -1 is treated as test not supported  
 	 ***************************************************************/ 

	do { /* LOOP infinitely if exer started with REG or EMC cmd line argument */
		for(tc = 0; tc < num_stanzas; tc++) {   /* Loop for each stanza in rules file */
			current_stanza = &rf_info[tc] ;

			htx_d.test_id = tc + 1;
            hxfupdate(UPDATE, &htx_d);

			if(exit_flag)
	            break;
			if(device_type == ETHERNET || device_type == ROCE) { 
		    	/*********************************************************************
 			 	 **  Open device to be exercised ... 
             	 ********************************************************************/
 		   		fd = socket(AF_INET, SOCK_DGRAM, 0);
    			if(fd < 0) {
        			sprintf(htx_d.msg_text, "Cannot open device, rc = %d, errno = %s \n", fd, strerror(errno));
        			hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
        			return(rc);
    			}

				/********************************************************************
 			 	 * Reset Test pass/fail statistics 
 			 	 ********************************************************************/
				memset(&result, 0, sizeof(struct self_test)); 
				failed_tc = 0, passed_tc = 0, total_tc = 0;

				memset(test->data, 0, strings->len * sizeof(unsigned long long));
				test->cmd = ETHTOOL_TEST;
    			test->len = strings->len;
    			test->flags = current_stanza->test;
			} else if(device_type == INFINIBAND) { 
				if(num_ports == 1 && current_stanza->test == IB_DMA_TEST) { 
					sprintf(htx_d.msg_text, "Cannot run DMA test on single port adapter \n"); 
					hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
                    return(-1);
				}
				if (device_name == HYDEPARK ) { 
					sprintf(server, "ib_send_bw -a -d %s -i 1 2>&1 &\n", device);  		
					if(DEBUG) printf("Server command = %s \n", server); 
					sprintf(client, "ib_send_bw -a -d %s -i 2 %s 2>&1 \n", device, hostname);
					if(DEBUG) printf("Client command = %s \n", client); 
					sprintf(ber_test, "ibdiagnet -i %s -lw 4x -ls 14 -P all=1 -ber_test --ber_thresh 1000000000000 -pm_pause_time 10 --skip nodes_info --screen_num_errs 1000 -o /var/tmp/ibdiagnet2/%s 2>&1\n" , device , device);
					if(DEBUG) printf("ber_test command = %s \n", ber_test); 
					sprintf(htx_d.msg_text, " Following IB Tests would be done for HydePark: \nDMA Server : %s\nDMA Client : %s\nBER TEST : %s\n", server, client, ber_test); 	
					hxfmsg(&htx_d, 0, HTX_HE_INFO,  htx_d.msg_text);
				} else if(device_name == GLACIERPARK) { 
					if(current_stanza->test == IB_DMA_TEST) {
						sprintf(server, "ib_send_bw -a -d %s -i 1 2>&1 &\n", device);
                        if(DEBUG) printf("Server command = %s \n", server);
						sprintf(client, "ib_send_bw -a -d %s -i 1 %s 2>&1 \n", client_device, hostname);
                	    if(DEBUG) printf("Client command = %s \n", client);
                        sprintf(htx_d.msg_text, " Following IB Tests would be done for GlacierPark: \nDMA Server : %s\nDMA Client : %s\n", server, client);    
	                    hxfmsg(&htx_d, 0, HTX_HE_INFO,  htx_d.msg_text);
					}
					if (current_stanza->test == IB_BER_TEST) { 
						sprintf(ber_test, "ibdiagnet  -i %s -lw 4x -ls 25 -P all=1 -ber_test --ber_thresh 1000000000000 -pm_pause_time 10 --skip nodes_info --screen_num_errs 1000 -o /var/tmp/ibdiagnet2/%s 2>&1\n" , device, device);
						if(DEBUG) printf("ber_test command = %s \n", ber_test);

						sprintf(htx_d.msg_text, " Following IB Tests would be done for GlacierPark: \nBER TEST : %s\n", ber_test); 
						hxfmsg(&htx_d, 0, HTX_HE_INFO,  htx_d.msg_text);
					}
                    if(current_stanza->test == IB_HCA_SELFTEST) {

                        /*
                         * Load mst modules ..
                         */
                        sprintf(command, "mst start \n");
                        rc = get_cmd_rc(command, &htx_d);
                        if(rc == -1 || rc == 127 ) {
                            sprintf(htx_d.msg_text, "cannot execute command = %s. Is Mellanox OFED installed ? \n", command);
                            hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                            exit(rc);
                        } else if(rc) {
                             /* Run command with verbosity and dump it to user */
                            if(DEBUG) printf("command = %s \n", command);
                            sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(command), rc);
                            hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

                            get_cmd_result(command, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
                            hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
                            exit(rc);
                        }
                        /*
                         * All well, populate hca self test command
                         */
                        sprintf(hca_selftest, "/usr/bin/hca_self_test.ofed \n");
                        if(DEBUG) printf("hca_self_test command = %s \n", hca_selftest);
						sprintf(htx_d.msg_text, " Following IB Tests would be done for GlacierPark: \nHCA SELF TEST : %s\n", hca_selftest);
                        hxfmsg(&htx_d, 0, HTX_HE_INFO,  htx_d.msg_text);

                    }
				} else if(device_name == GLACIERPARK_1PORT) { 
					if(current_stanza->test == IB_DMA_TEST) {
                        sprintf(htx_d.msg_text, "GlacierPark_1Port doesnot supports DMA Test. This stanza would be skipped \n");
                        hxfmsg(&htx_d, 0, HTX_HE_SOFT_ERROR, htx_d.msg_text);
                        continue;
                    }
					if(current_stanza->test == IB_HCA_SELFTEST) { 
			
						/*
						 * Load mst modules .. 
						 */
				 		sprintf(command, "mst start \n"); 
						rc = get_cmd_rc(command, &htx_d); 
						if(rc == -1 || rc == 127 ) {
                            sprintf(htx_d.msg_text, "cannot execute command = %s. Is Mellanox OFED installed ? \n", command);
                            hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                            exit(rc);
                        } else if(rc) {
						     /* Run command with verbosity and dump it to user */
                            if(DEBUG) printf("command = %s \n", command);
                            sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(command), rc);
                            hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

                            get_cmd_result(command, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
                            hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
                            exit(rc);
                        }
						/* 
						 * All well, populate hca self test command 
						 */ 
						sprintf(hca_selftest, "/usr/bin/hca_self_test.ofed \n"); 
						if(DEBUG) printf("hca_self_test command = %s \n", hca_selftest); 
				 	}	
					if (current_stanza->test == IB_BER_TEST) {
						sprintf(ber_test, "ibdiagnet -i %s -lw 4x -ls 25 -P all=1 -ber_test --ber_thresh 1000000000000 -pm_pause_time 10 --skip nodes_info --screen_num_errs 1000 -o /var/tmp/ibdiagnet2/%s 2>&1\n", device, device);
            	        if(DEBUG) printf("ber_test command = %s \n", ber_test);
					}	
                	sprintf(htx_d.msg_text, " Following IB Tests would be done for GlacierPark 1PORT Adapter: \nBER TEST : %s\n, HCA_SELT TEST : %s \n", ber_test, hca_selftest);
                    hxfmsg(&htx_d, 0, HTX_HE_INFO,  htx_d.msg_text);
				} else { 
					sprintf(htx_d.msg_text, " Hxediag doesnot recognize this device, device_name=%#x\n",device_name); 
					hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
					return(device_name);		
				}
			} else { 
				sprintf(htx_d.msg_text, " Hxediag doesnot recognize this device, device_type=%#x \n", device_type); 
				hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
                return(device_type); 
			}
			/*******************************************************************
 			 * Start the test .. 
 			 * ****************************************************************/
			for(i = 0; i < current_stanza->num_oper; i++) { 

				if(exit_flag)
		            break;

				if(device_type == ETHERNET || device_type == ROCE) {
    				rc = issue_ioctl(fd, test, &htx_d);
    				if (rc < 0 ) {
        				sprintf(htx_d.msg_text, "ETHTOOL_TEST ioctl failed with rc = %d and errno=%s \n", rc, strerror(errno));
						hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
        				close(fd);
        				free (test);
        				free(strings);
        				return(rc);
    				}		
	                rc = update_result(test, strings->len, supported_test, &result, &htx_d);
    	            if(rc == -EOPNOTSUPP) {
        	            sprintf(htx_d.msg_text, "Unrecongnized test, exiting test... \n");
            	        hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                	    close(fd);
                    	free (test);
                    	free(strings);
                    	exit(rc);
                	} else if(rc) {
                    	htx_d.bad_others += rc;
                   	 	htx_d.good_writes += (strings->len - rc);
                    	htx_d.bad_writes  += rc;
                    	passed_tc += (strings->len - rc);
                    	failed_tc += rc;
						sprintf(htx_d.msg_text, "Exiting Diag Test for %s, rc = %d \n", htx_d.sdev_id, rc); 
						hxfmsg(&htx_d, rc,  HTX_HE_HARD_ERROR, htx_d.msg_text);
                    	exit(rc);
                	} else {
                    	htx_d.good_others += rc;
                    	htx_d.good_writes +=  strings->len;
                    	passed_tc += strings->len;
                	}
                	total_tc += strings->len ;

				} else if(device_type == INFINIBAND) { 

					if(current_stanza->test == IB_DMA_TEST) { 
						rc = get_cmd_rc(server, &htx_d);						
            			if(rc == -1 || rc == 127 ) {
                			sprintf(htx_d.msg_text, "cannot execute command = %s \n", server);
                			hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                			exit(rc);
            			} else if(rc) {
                			/* Run command with verbosity and dump it to user */
                			if(DEBUG) printf("command = %s \n", server);
							sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(server), rc); 
							hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

                			get_cmd_result(server, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
                			hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
							exit(rc);
						}	
						sleep(5); 
						rc = get_cmd_rc(client, &htx_d);
                    	if(rc == -1 || rc == 127 ) {
                        	sprintf(htx_d.msg_text, "cannot execute command = %s \n", client);
                        	hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                        	exit(rc);
             	       } else if(rc) {
                	        /* Run command with verbosity and dump it to user */
							sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(client), rc);
                        	hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

	                        if(DEBUG) printf("command = %s \n", client);
    	                    get_cmd_result(client, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
        	                hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
							exit(rc);
                	    }
						htx_d.good_writes += 1; 	
					} else if (current_stanza->test == IB_BER_TEST) { 
						rc = get_cmd_rc(ber_test, &htx_d); 
                    	if(rc == -1 || rc == 127 ) {
                        	sprintf(htx_d.msg_text, "cannot execute command = %s \n", ber_test);
                        	hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                        	exit(rc);
                    	} else if(rc) {
			                sprintf(htx_d.msg_text, "Command : %s, Failed with rc = %d \n", trim(ber_test), rc);
                            hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text);	
				            retry = MAX_RETRY; 
				            do { 
					            rc = get_cmd_rc(ber_test, &htx_d); 
					            if(rc == -1 || rc == 127 ) {
        	                        sprintf(htx_d.msg_text, "cannot execute command = %s \n", ber_test);
		                            hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
        	                        exit(rc);
                	        	} else if(rc) {
						            sprintf(htx_d.msg_text, "Command Retry: %s, Failed with rc = %d \n Retry delay=%d\n", trim(ber_test), rc,(MAX_RETRY - retry + 1));
					                hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text); 
						            sleep(MAX_RETRY - retry + 1); 
						            retry --; 
					            } else {
						            break;
					            } 
				            } while(retry); 
				
				            if(retry == 0){
	                        	/* Run command with verbosity and dump it to user */
					            sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(ber_test), rc);
                        		hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

                	        	if(DEBUG) printf("command = %s \n", ber_test);
        	               		get_cmd_result(ber_test, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
	                        	hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, htx_d.msg_text);
					            exit(rc);
				            }
               		    } 
						htx_d.good_writes += 1; 	
					} else if(current_stanza->test == IB_HCA_SELFTEST) { 
						rc = get_cmd_rc(hca_selftest, &htx_d);
                        if(rc == -1 || rc == 127 ) {
                            sprintf(htx_d.msg_text, "cannot execute command = %s \n", hca_selftest);
                            hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                            exit(rc);
                        } else if(rc) {
                            /* Run command with verbosity and dump it to user */
                            sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(hca_selftest), rc);
                            hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

                            if(DEBUG) printf("command = %s \n", hca_selftest);
                            get_cmd_result(hca_selftest, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
                            hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
                            exit(rc);
                        }
                        htx_d.good_writes += 1;
					} 
				}
				hxfupdate(UPDATE, &htx_d); 
				
				if(current_stanza->sleep && !exit_flag) { 
					sleep(current_stanza->sleep); 
				}
			}  /* Off for(i = 0; i < current_stanza->num_oper; i++)  */ 
		
			if(device_type == ETHERNET || device_type == ROCE) { 
				/****************************************************************
 			 	 * Close fd 
 			 	 * *************************************************************/ 
				rc = close(fd); 
            	if(fd < 0) {
                	sprintf(htx_d.msg_text, "Cannot close device, rc = %d, errno = %s \n", fd, strerror(errno));
                	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
                	return(rc);
            	}
				fd = -1; 

				if(rc) { 	
					memset(stat_msg, '\0', MSG_TEXT_SIZE); 
					sprintf(msg, "Stanza=%s, Detailed Pass/Fail statistics :  \n", current_stanza->rule_id); 	
					strcat(stat_msg, msg);
					sprintf(msg, "TestType \t\tPass\t\tFail\t\tTotal\n"); 	
					strcat(stat_msg, msg);
					if(result.nvram_test.total) { 
						sprintf(msg,"NVRAM \t\t\t%#llx\t\t%#llx\t\t%#llx\n", result.nvram_test.pass, result.nvram_test.fail, result.nvram_test.total);   
						strcat(stat_msg, msg); 
					}
					if(result.link_test.total) { 
						sprintf(msg,"LINK \t\t\t%#llx\t\t%#llx\t\t%#llx\n", result.link_test.pass, result.link_test.fail, result.link_test.total); 
						strcat(stat_msg, msg);
    	            }
					if(result.register_test.total) { 
						sprintf(msg,"REGISTER \t\t%#llx\t\t%#llx\t\t%#llx\n", result.register_test.pass, result.register_test.fail, result.register_test.total); 
						strcat(stat_msg, msg);
                	}	
					if(result.memory_test.total) { 
						sprintf(msg,"MEMORY \t\t\t%#llx\t\t%#llx\t\t%#llx\n", result.memory_test.pass, result.memory_test.fail, result.memory_test.total); 
						strcat(stat_msg, msg);
                	}
					if(result.mac_loopback.total) { 
						sprintf(msg,"MAC LOOPBACK \t\t%#llx\t\t%#llx\t\t%#llx\n", result.mac_loopback.pass, result.mac_loopback.fail, result.mac_loopback.total); 
						strcat(stat_msg, msg);
            	    }
					if(result.phy_loopback.total) { 
						sprintf(msg,"PHY LOOPBACK \t\t%#llx\t\t%#llx\t\t%#llx\n", result.phy_loopback.pass, result.phy_loopback.fail, result.phy_loopback.total); 
						strcat(stat_msg, msg);
                	}
					if(result.int_loopback.total) { 
						sprintf(msg,"INTERNAL LOOPBACK \t%#llx\t\t%#llx\t\t%#llx\n", result.int_loopback.pass, result.int_loopback.fail, result.int_loopback.total); 
						strcat(stat_msg, msg);
                	}
					if(result.ext_loopback.total) { 
						sprintf(msg,"EXTERNAL LOOPBACK \t%#llx\t\t%#llx\t\t%#llx\n", result.ext_loopback.pass, result.ext_loopback.fail, result.ext_loopback.total); 
						strcat(stat_msg, msg);
                	}
					if(result.interrupt_test.total) { 
						sprintf(msg,"INTERRUPT \t\t%#llx\t\t%#llx\t\t%#llx\n", result.interrupt_test.pass, result.interrupt_test.fail, result.interrupt_test.total); 	
						strcat(stat_msg, msg);
                	}
					if(result.speed_test.total) { 
						sprintf(msg,"SPEED \t\t%#llx\t\t%#llx\t\t%#llx\n", result.speed_test.pass, result.speed_test.fail, result.speed_test.total); 
						strcat(stat_msg, msg);
            	    }
					if(result.loopback_test.total) { 
						sprintf(msg,"LOOPBACK \t\t%#llx\t\t%#llx\t\t%#llx\n", result.loopback_test.pass, result.loopback_test.fail, result.loopback_test.total); 
						strcat(stat_msg, msg);
	                }
					hxfmsg(&htx_d, 0, HTX_HE_INFO, stat_msg);
					rc = 0;
					/*********************************************************
 			 		 * Failure percent .. 
 			 	 	 ********************************************************/
					percent_fail = ((float) failed_tc / (float)total_tc) * 100; 	
					percent_pass = ((float)passed_tc / (float)total_tc) * 100; 
					if(percent_fail > (float)current_stanza->threshold) { 
						sprintf(htx_d.msg_text, "Stanza=%s test_fail_prcnt=%f, test_pass_prcnt=%f, total_test=%#llx. Too many fails. Hence Exiting test ... \n", 
								current_stanza->rule_id, percent_fail, percent_pass, total_tc);
						hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, htx_d.msg_text);
						exit(1); 
					} else { 
						sprintf(htx_d.msg_text, "Stanza=%s test_fail_prcnt=%f, test_pass_prcnt=%f, total_test=%#llx \n", current_stanza->rule_id, percent_fail, percent_pass, total_tc);  
						hxfmsg(&htx_d, 0, HTX_HE_INFO, htx_d.msg_text);
					}
				}
			} else if(device_type == INFINIBAND) {
				if(current_stanza->test == IB_HCA_SELFTEST)  { 
					/*
					 * Unload mst modules .. 
					 */
					sprintf(command, "mst stop --force \n");
       				rc = get_cmd_rc(command, &htx_d);
                    if(rc == -1 || rc == 127 ) {
                    	sprintf(htx_d.msg_text, "cannot execute command = %s. Is Mellanox OFED installed ? \n", command);
                        hxfmsg(&htx_d, ENOSYS, HTX_HE_HARD_ERROR, htx_d.msg_text);
                        exit(rc);
                    } else if(rc) {
                    	/* Run command with verbosity and dump it to user */
                        if(DEBUG) printf("command = %s \n", command);
                        sprintf(htx_d.msg_text, " Command : %s, Failed with rc = %d \n", trim(command), rc);
                        hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);

                        get_cmd_result(command, htx_d.msg_text, MSG_TEXT_SIZE, &htx_d);
                        hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, htx_d.msg_text);
                        exit(rc);
                   }
				}
			}
           	if(exit_flag)
               	break;

		} /* Off for(tc = 0; tc < num_stanzas; tc++)  */ 
		/************************************************************************
 		 * hxfupdate call with FINISH arg indicates one more rule file pass done
 		 ************************************************************************/
        hxfupdate(FINISH, &htx_d);
        if(exit_flag)
            break;

	} while( (rc = strcmp(htx_d.run_type, "REG") == 0) || (rc = strcmp(htx_d.run_type, "EMC") == 0) );

    free(test);
    free(strings);
	close(fd); 	
	return(0); 

}

/**************************************************************************
 * Function that eventually issues SIOCETHTOOL 
 * This functions forms the command structure as expected by ioctl and
 * then issues the IOCTL to query driver 
 *************************************************************************/ 
int
issue_ioctl(int fd, void  * command , struct htx_data * htx_d) { 
	
	int rc = 0 ;				/* catures return code */  
	struct ifreq ifr;			/* Common structure to send command through SIOCETHTOOL infrastructure */  

	/***********************************************************
	 * Copy the command information to be send to driver 
	 **********************************************************/ 
	memset(&ifr, 0, sizeof(struct ifreq)); 
	strcpy(ifr.ifr_name, basename(htx_d->sdev_id)); 
	ifr.ifr_data = command; 

	/***********************************************************
	 * Issue IOCTL SIOCETHTOOL ..
	 **********************************************************/ 
	rc = ioctl(fd, SIOCETHTOOL, &ifr); 
	if(rc < 0) { 
		sprintf(htx_d->msg_text, "SIOCETHTOOL ioctl failed with rc = %d, errno = %s  for device = %s\n", rc, strerror(errno), htx_d->sdev_id); 	
		hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, htx_d->msg_text); 
		return(rc); 	
	} 
	return(rc); 
} 

/***************************************************************************
 * This funciton queries device driver to figure out which self test 
 * are currently supported. 
 **************************************************************************/
struct ethtool_gstrings *
get_stringset(enum ethtool_stringset set_id, size_t drvinfo_offset, int fd, struct htx_data * htx_d) { 

    unsigned int len, i;
	int rc = 0; 
   	struct ethtool_sset_info hdr;
    struct ethtool_drvinfo drvinfo;
    struct ethtool_gstrings *strings;

	memset(&hdr, 0, sizeof(struct ethtool_sset_info)); 
    hdr.cmd = ETHTOOL_GSSET_INFO;
    hdr.reserved = 0;
    hdr.sset_mask = 1ULL << set_id;

	rc = issue_ioctl(fd, &hdr, htx_d); 
	if(rc == 0) { 
		len = hdr.sset_mask ? hdr.data[0] : 0;
	} else if (errno == EOPNOTSUPP && drvinfo_offset != 0) { 
		/* Fallback for old kernel versions */
		memset(&drvinfo, 0, sizeof(struct ethtool_drvinfo));
        drvinfo.cmd = ETHTOOL_GDRVINFO;
        rc = issue_ioctl(fd, &drvinfo, htx_d); 	
		if(rc < 0) { 
			sprintf(htx_d->msg_text, "get_stringset:ETHTOOL_GDRVINFO failed with rc=%d, errno=%s \n", rc, strerror(errno)); 
			hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, htx_d->msg_text);
            return NULL;
		}
        len = *(unsigned int *)((char *)&drvinfo + drvinfo_offset);
	} else { 
		sprintf(htx_d->msg_text, "ETHTOOL_GSSET_INFO failed with rc=%d, errno=%s \n", rc, strerror(errno)); 
		hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, htx_d->msg_text); 
		return(NULL); 
	}

    strings = malloc(sizeof(struct ethtool_gstrings) + len * ETH_GSTRING_LEN);
    if (strings == NULL) { 
		sprintf(htx_d->msg_text, "get_stringset:malloc failed with errno = %s \n", strerror(errno)); 
		hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, htx_d->msg_text);
        return NULL;
	}

    strings->cmd = ETHTOOL_GSTRINGS;
    strings->string_set = set_id;
    strings->len = len;
    if (len != 0 && issue_ioctl(fd, strings, htx_d)) {
        free(strings);
        return NULL;
    }

    for (i = 0; i < len; i++) { 
    	strings->data[(i + 1) * ETH_GSTRING_LEN - 1] = 0;
	}
    return strings;
}

                                                                        
int 
dump_drvinfo(struct ethtool_drvinfo *info) {

    fprintf(stdout,
        "driver: %.*s\n"
        "version: %.*s\n"
        "firmware-version: %.*s\n"
        "bus-info: %.*s\n"
        "supports-statistics: %s\n"
        "supports-test: %s\n"
        "supports-eeprom-access: %s\n"
        "supports-register-dump: %s\n"
        "supports-priv-flags: %s\n",
        (int)sizeof(info->driver), info->driver,
        (int)sizeof(info->version), info->version,
        (int)sizeof(info->fw_version), info->fw_version,
        (int)sizeof(info->bus_info), info->bus_info,
        info->n_stats ? "yes" : "no",
        info->testinfo_len ? "yes" : "no",
        info->eedump_len ? "yes" : "no",
        info->regdump_len ? "yes" : "no",
        info->n_priv_flags ? "yes" : "no");

    return 0;
}



int dump_test(struct ethtool_test *test,
             struct ethtool_gstrings *strings)
{
    int i, rc;

    rc = test->flags & ETH_TEST_FL_FAILED;
    fprintf(stdout, "The test result is %s\n", rc ? "FAIL" : "PASS");

    if (test->flags & ETH_TEST_FL_EXTERNAL_LB)
        fprintf(stdout, "External loopback test was %sexecuted\n",
            (test->flags & ETH_TEST_FL_EXTERNAL_LB_DONE) ?
            "" : "not ");

    if (strings->len)
        fprintf(stdout, "The test extra info:\n");

    for (i = 0; i < strings->len; i++) {
        fprintf(stdout, "%s\t %d\n", (char *)(strings->data + i * ETH_GSTRING_LEN), (unsigned int ) test->data[i]);
    }

    fprintf(stdout, "\n");
    return rc;
}

char * strupr(char * str) { 

	char *s;

  	for(s = str; *s; s++)
    	*s = toupper((unsigned char)*s);

  	return str;

}	

int update_result(struct ethtool_test *test, int num_tests,  char supported_test[][MSG_TEXT_SIZE], struct self_test * self_test, struct htx_data * htx_d) { 


	int i = 0, rc = 0; 
	char message[MSG_TEXT_SIZE] = {'\0'}, message1[MSG_TEXT_SIZE] = {'\0'}; 
	
	for(i = 0; i < num_tests; i++) { 
		
		if(strstr(supported_test[i], "NVRAM")) { 
			if(test->data[i]) { 
				self_test->nvram_test.fail++;  
				/* 
				 * NVRAM Test failing on Shiner-T adapter, Broadcom debugging 
				 * Hence ignore failures for NVRAM Test 
				 */ 
				/* rc ++; */ 
			} else { 
				self_test->nvram_test.pass++; 
			}
			self_test->nvram_test.total++; 
		} else if(strstr(supported_test[i], "LINK")) {  
			/*******************************************************************
 			 * if Link test is reported as failure, then i could be 
 			 * expected coz MFG generally runs with wraps on .... 
 			 * So we donto report hard failures for Link test, just ignore if link
 			 * test fails 
 			 * ********************************************************************/
			if(test->data[i]) { 
				self_test->link_test.fail ++; 
			} else { 
				self_test->link_test.pass ++; 
			} 
			self_test->link_test.total++; 
		} else if(strstr(supported_test[i], "REGISTER")) { 
			if(test->data[i]) { 
				self_test->register_test.fail ++; 
				rc++; 
			} else { 
				self_test->register_test.pass ++; 
			}
			self_test->register_test.total++; 
		} else if(strstr(supported_test[i], "MEMORY")) { 
			if(test->data[i]) {
                self_test->memory_test.fail ++; 
				rc++; 
			} else { 
				self_test->memory_test.pass ++; 
			} 
			self_test->memory_test.total++; 
		} else if(strstr(supported_test[i], "MAC")) { 
			if(test->data[i]) {
				self_test->mac_loopback.fail ++; 
				rc++;
			} else { 
				self_test->mac_loopback.pass++; 
			} 
			self_test->mac_loopback.total++; 
		} else if(strstr(supported_test[i], "PHY")) { 
			if(test->data[i]) {
				self_test->phy_loopback.fail ++; 
				rc++; 
			} else { 
				self_test->phy_loopback.pass ++; 
			} 
			self_test->phy_loopback.total++; 	
		} else if(strstr(supported_test[i], "INT_LOOP")) { 
			/******************************************************
 			 * failure woudl be ignored because internal 
 			 * loopback can fail if there is no loopback on ports 
 			 * ***************************************************/
			if(test->data[i]) {
				self_test->int_loopback.fail ++; 
				rc++; 
			} else { 
				self_test->int_loopback.pass ++; 
			}
			self_test->int_loopback.total++; 
		} else if(strstr(supported_test[i], "EXT")) {	/* External LoopBack test requested */ 
            if(test->flags & ETH_TEST_FL_EXTERNAL_LB_DONE) { /* Adapter did External Loop back test */
             	/**************************************************************
 				 * This test may be reported as fail if we have only wrap plugs
 				 * on cards. MFG generally test with wraps ON !!
 				 **************************************************************/
				if(test->data[i]) {
					self_test->ext_loopback.fail ++; 
				} else { 
					self_test->ext_loopback.pass ++; 
				}
			} else { 
				self_test->ext_loopback.fail ++;
			}
			self_test->ext_loopback.total++; 
		} else if(strstr(supported_test[i], "INTERRUPT")) { 
			if(test->data[i]) {
				self_test->interrupt_test.fail ++; 
				/**************************************************************************
				 * Ignore Interrupt test for now, its failing on Broadcom-Austin adapter
				 * due to device driver - BJKing needs to come back when this is fixed in driver
				 * Uncomment below line if we want to flag hard error for Interrupt test. 
				 ******************************************************************************/
				/*  rc++;  */  
			} else { 
				self_test->interrupt_test.pass ++; 
			}
			self_test->interrupt_test.total++; 
		} else if(strstr(supported_test[i], "SPEED")) { 
			/*********************************************
 			 * Same behaviour as link test 
 			 * ******************************************/
			if(test->data[i]) {
                self_test->speed_test.fail ++; 
			} else { 
				self_test->speed_test.pass ++; 	
			} 
			self_test->speed_test.total ++; 
		} else if(strstr(supported_test[i], "LOOPBACK")) { 
			/**********************************************
 			* Same behavior as external loopback 
 			* ********************************************/
			if(test->data[i]) {
				self_test->loopback_test.fail ++; 
			} else { 
				self_test->loopback_test.pass ++; 
			} 
			self_test->loopback_test.total ++; 
		} else { 
		    sprintf(htx_d->msg_text, "Unrecognized test=%s, this program needs to be updated \n", supported_test[i]);
        	hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, htx_d->msg_text);
			return(-EOPNOTSUPP); 
		} 
		if(rc && test->data[i]) { 	
			sprintf(message, "\n%s ",supported_test[i]); 
			strcat(message1, message); 
		}
	}
	if(rc) { 
		sprintf(htx_d->msg_text, "Following test failed :  %s\n", message1); 
		hxfmsg(htx_d, rc, HTX_HE_SOFT_ERROR, htx_d->msg_text); 	
	}
	return(rc); 
} 

/*******************************************************************
 * Exerciser specific SIGTERM handler, intiates exerciser exit ..
 * ****************************************************************/
void
SIGTERM_hdl (int sig) {

	exit_flag = 1 ; 

}	
/*********************************************************************
 * This function sets default values for every test case in rule file.
 *********************************************************************/
void
set_defaults(struct rule_info h_r[]) { 

	int i = 0; 
	memset(h_r, 0, (sizeof(struct rule_info) * MAX_TC));

	for(i = 0; i < MAX_TC ; i++) { 
		strncpy(h_r[i].rule_id, "DEFAULT", MAX_STRING_SIZE); 
		h_r[i].num_oper = 8; 
		h_r[i].test = 0; 
		h_r[i].sleep = 5; 
		h_r[i].threshold = 0; 
	}
}

int
get_line( char s[], FILE *fp) {

    int c,i;
    i=0;
    while (((c = fgetc(fp)) != EOF) && c != '\n') {
        s[i++] = c;
    }
    if (c == '\n')
        s[i++] = c;

    s[i] = '\0';
    return(i);
}
	  
int
parse_line(char s[]) {

    int len, i = 0, j = 0;

    while(s[i] == ' ' || s[i] == '\t') {
        i++;
    }
    if(s[i] == '*') {
        return(0);
    }
    len = strlen(s);
    for(; i < len && s[i] != '\0'; i++) {
        if (s[i] == '=')
            s[i] = ' ';
        s[j++] = s[i];
    }
    s[j] = '\0';
    return((s[0] == '\n')? 1 : j);
}


int
rf_read_rules(const char rf_name[], struct rule_info rf_info[], unsigned int * num_stanzas, struct htx_data * htx_d) { 

    char line[200], keywd[200];
    struct rule_info * current_ruleptr = NULL;
    int eof_flag = 0, num_tc = 0, rc, change_tc = 1;
    FILE * rf_ptr;
	char msg[MSG_TEXT_SIZE]; 
 	char tmp[MAX_STRING_SIZE];

    if ( (rf_ptr = fopen(rf_name, "r")) == NULL ) {
        sprintf(msg,"error open %s ",rf_name);
        hxfmsg(htx_d, errno, HTX_HE_HARD_ERROR, msg);
        return(-1);
    }
    set_defaults(rf_info);

    current_ruleptr = &rf_info[0];
    do {
        rc = get_line(line, rf_ptr);
        /****************************************************************
 		 * rc = 0 indicates End of File.
 		 * rc = 1 indicates only '\n' (newline char) on the line.
 		 * rc > 1 more characters.
 		 ***************************************************************/
        if(rc == 0) {
            eof_flag = 1;
            break;
        }
        if(DEBUG) printf("\n%s: line - %s rc - %d",__FUNCTION__, line, rc);
        /******************************************************************
 		 * rc = 1 indicates a newline which means end of current testcase.
 		 ******************************************************************/
        if(rc == 1) {
            change_tc = 1;
            continue;
        }
        /******************************************************************
 		 * rc = 0 indicates comment line in rule file.
 		 * rc = 1 indicates some white spaces and newline.
 		 * rc > 1 indicates there may be valid test case parameter.
 		 *****************************************************************/
        rc = parse_line(line);
        if(DEBUG) printf("\n%s: line - %s rc - %d",__FUNCTION__, line, rc);
        if(rc == 0 || rc == 1) {
            if(rc == 1)
                change_tc = 1;
                continue;
        } else {
            if(rc > 1 && change_tc == 1) {
                current_ruleptr = &rf_info[num_tc];
                if(num_tc >= MAX_TC) {
                    sprintf(msg,"\n Max num of test cases allowed are %d", MAX_TC);
                    hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
                    return(-1);
                }
                num_tc++;
                if(DEBUG) printf("\n Next rule - %d", num_tc);
                change_tc = 0;
            }
        }
        sscanf(line,"%s",keywd);
        if(DEBUG) printf("\n%s: keywd - %s",__FUNCTION__, keywd);
        if ( (strcmp(keywd, "rule_id")) == 0 ) {
            sscanf(line, "%*s %s", tmp);
            if (((strlen(tmp)) < 1) || ((strlen(tmp)) > MAX_STRING_SIZE) ) {
                sprintf(msg, "\n rule_id string (%s) length should be in the range"
                         " 1 < length < %d", current_ruleptr->rule_id, MAX_STRING_SIZE);
                hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
                strncpy(current_ruleptr->rule_id, tmp, MAX_STRING_SIZE);
            } else {
                strncpy(current_ruleptr->rule_id, tmp, MAX_STRING_SIZE);
                if(DEBUG) printf("\n rule_id - %s ", current_ruleptr->rule_id);
            }
		} else if ((strcmp(keywd, "num_oper")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->num_oper);
            if(DEBUG) printf("\n num_oper - %d ", current_ruleptr->num_oper);
            if (current_ruleptr->num_oper < 0) {
                sprintf(msg, "\n num_oper must be either 0 or +ve integer");
                hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                if(DEBUG) printf("\n num_oper - %d ", current_ruleptr->num_oper);
            }
		} else if ((strcmp(keywd, "test")) == 0) { 
			sscanf(line, "%*s %s", tmp); 			 
			if((strcmp(tmp, "online")) == 0) { 
				current_ruleptr->test = 0; 
			} else if((strcmp(tmp, "offline")) == 0) { 
				current_ruleptr->test = ETH_TEST_FL_OFFLINE; 
			} else if((strcmp(tmp, "external_lb")) == 0) { 
				current_ruleptr->test = ETH_TEST_FL_EXTERNAL_LB; 
			} else if((strcmp(tmp, "ber")) == 0) { 
				current_ruleptr->test = IB_BER_TEST; 
			} else if((strcmp(tmp, "dma")) == 0) { 
				current_ruleptr->test = IB_DMA_TEST; 
			} else if((strcmp(tmp, "selftest")) == 0) { 
				current_ruleptr->test = IB_HCA_SELFTEST;
			} else { 
				sprintf(msg, "\n test should be one of these values : ONLINE, OFFLINE, EXTERNAL_LB, BER or DMA value specified = %s \n", tmp); 
				hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg); 
				return(-1); 
            }
			if(DEBUG) printf("\n test - %d \n", current_ruleptr->test); 
		} else if ((strcmp(keywd, "sleep")) == 0) { 
			sscanf(line, "%*s %d", &current_ruleptr->sleep); 
			if(DEBUG) printf("\n sleep - %d ", current_ruleptr->sleep);  
			if (current_ruleptr->sleep < 0) { 
				sprintf(msg, "\n sleep must me +ive integer, sleep = %d\n", current_ruleptr->sleep); 
				hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
				return(-1); 
			} else { 
				if(DEBUG) printf("\n num_oper - %d ", current_ruleptr->num_oper);
            }
        } else if ((strcmp(keywd, "threshold")) == 0) {
            sscanf(line, "%*s %d", &current_ruleptr->threshold);
            if(DEBUG) printf("\n sleep - %d ", current_ruleptr->threshold);
            if (current_ruleptr->sleep < 0) {
                sprintf(msg, "\n sleep must me +ive integer, sleep = %d\n", current_ruleptr->threshold);
                hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                if(DEBUG) printf("\n num_oper - %d ", current_ruleptr->threshold);
            }
		} else if ((strcmp(keywd, "device_type")) == 0) { 
			sscanf(line, "%*s %s", tmp);
			if((strcmp(tmp, "ethernet")) == 0) { 
				device_type = ETHERNET; 
			} else if((strcmp(tmp, "roce")) == 0) { 
				device_type = ROCE; 
			} else if((strcmp(tmp, "infiniband") == 0) || (strcmp(tmp, "ib")) == 0) {
				device_type = INFINIBAND; 
			} else { 
				sprintf(msg, "\n DEVICE_TYPE can be ethernet, roce, infiniband/ib, input = %s \n", tmp);
                hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
			}
		} else if((strcmp(keywd, "device_name")) == 0) { 
			sscanf(line, "%*s %s", tmp);
			if(strcmp(tmp, "hydepark") == 0) { 
				device_name = HYDEPARK; 
			} else if(strcmp(tmp, "glacierpark") == 0) { 
				device_name = GLACIERPARK;  
			} else if((strcmp(tmp, "travis") == 0)) { 
				device_name= TRAVIS_3EN; 
			} else if(strcmp(tmp, "shiner") == 0) { 
				device_name= SHINER; 
			} else if(strcmp(tmp, "glacierpark_1port") == 0) { 
				device_name = GLACIERPARK_1PORT; 
			} else { 
				sprintf(msg, "\n DEVICE_NAME can be HYDEPARK, GLACIERPARK, TRAVIS_3EN, SHINER, input = %s \n", tmp); 
				hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
			} 
		} else if ((strcmp(keywd, "num_ports")) == 0) { 
			sscanf(line, "%*s %d", &num_ports);
			if(num_ports <= 0) { 
				printf(msg, "\n Wrong num_ports in rule file, input = %s \n", num_ports); 
				hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
            if(DEBUG) printf("\n num_ports = %d \n", num_ports); 
				
        } else {
            sprintf(msg, "\n Wrong keyword - %s specified in rule file. Exiting !!\n", keywd);
            hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }

    } while(eof_flag == 0);
    *num_stanzas = num_tc;
    fclose(rf_ptr);
    return(0);
}

/**************************************************************************/
/* Get the output of passed shell command                                 */
/**************************************************************************/
#define MAX_LINES 16 

int   
get_cmd_result(char cmd[], char result[], int size, struct htx_data * htx_d) { 

    char buf[MAX_LINES][MSG_TEXT_SIZE], * rc, * ptr;
    FILE  * fp;
    int cnt=0, i = 0, j = 0;

	if(DEBUG) printf("%s : running cmd : %s \n", __FUNCTION__, cmd);

    do {
        fp = popen(cmd,"r");
        cnt++;
    } while ( fp == NULL && errno == EINTR && cnt < NUM_RETRIES );

    if ( fp == NULL ) {
        if ( errno == EINTR ) {
            sprintf(htx_d->msg_text, "%s:popen failed for command \"%s\" with error %d(%s) even after %d retries.\n",
                          __FUNCTION__, cmd, errno, strerror(errno), cnt);
            hxfmsg(htx_d, 0, HTX_HE_INFO, htx_d->msg_text);
            return(-1);
        }
        else {
            sprintf(htx_d->msg_text, "%s:popen failed for command \"%s\" with error %d(%s).\n",
                          __FUNCTION__, cmd, errno, strerror(errno));
            hxfmsg(htx_d, 0, HTX_HE_HARD_ERROR, htx_d->msg_text);
            return(-1);
        }
    }
    do {
        rc = fgets(buf[cnt], MSG_TEXT_SIZE, fp);
        cnt++;
    } while ( rc != NULL && cnt < MAX_LINES);

    pclose(fp);
	memset(result, 0, size); 

	ptr = result; 
	for(i = 0; i < cnt; i++) { 
		j += strlen(buf[i]); 
		if(j >= size) break; 
		strcpy(ptr, buf[i]); 
		ptr += strlen(buf[i]); 
	} 
	if(DEBUG) printf("%s:lines=%d got follwoing result \n%s\n", __FUNCTION__, cnt, result);
    return(0);
}


int
get_cmd_rc(char cmd[], struct htx_data * htx_d) {

	int rc = system(cmd); 

	if(DEBUG) printf(" %s : running cmd : %s \n", __FUNCTION__,  cmd); 

	if(rc == -1) { 
		sprintf(htx_d->msg_text, "Error with system() call, Reason : child process could not be created.cmd=%s \n", trim(cmd)); 
		hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, htx_d->msg_text); 
		return(rc); 
	} else if(WEXITSTATUS(rc) == 127) { 
		sprintf(htx_d->msg_text, "command=%s,  not found, rc = %d \n", trim(cmd), WEXITSTATUS(rc)); 
		hxfmsg(htx_d, 127, HTX_HE_HARD_ERROR, htx_d->msg_text);
        return(WEXITSTATUS(rc));
	} else { 
		return(WEXITSTATUS(rc)); 
	} 

	return(0); 
}	

char *
trim(char * str) { 

    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if( str == NULL ) { return NULL; }
    if( str[0] == '\0' ) { return str; }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while( isspace(*frontp) ) { ++frontp; }
    if( endp != frontp )
    {
        while( isspace(*(--endp)) && endp != frontp ) {}
    }

    if( str + len - 1 != endp )
		*(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
		*str = '\0';

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if( frontp != str )
    {
		while( *frontp ) { *endp++ = *frontp++; }
        *endp = '\0';
    }


    return str;
}
