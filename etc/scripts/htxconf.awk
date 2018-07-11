#!/bin/awk -f  

# IBM_PROLOG_BEGIN_TAG
#
# Copyright 2003,2016 IBM International Business Machines Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#                http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# IBM_PROLOG_END_TAG
############################################################################### 
#  awk scripts can be broken down into three main routines 
 
# BEGIN         This routine is executed once before any input is read 
# MAIN PROGRAM  This routine is executed for each line of input 
# END           This routine is executed once after all input has been read 
 
#  example of htxconf.awk stanza to mdt stanza 
# 
#           1          2       3       4       5          6         7 
# mkstanza("hxesfmba","audio","baud0","baud0","hxesfmba","default","default"); 
# start_halted("y"); 
# 
# mkstanza args: 1 = HE_name (Hardware Exerciser program name) 
#                2 = adapt_desc 
#                3 = device_desc 
#                4 = stanza name (in upper left hand corner appended by ":") 
#                5 = rules file directory 
#                6 = reg_rules 
#                7 = emc_rules 
#                start_halted("y") = function to gen 'start_halted = "y"' line 
# 
# creates mdt stanza: 
# 
# baud0: 
#         HE_name = "hxesfmba"              * Hardware Exerciser name, 14 char 
#         adapt_desc = "audio"              * adapter description, 11 char max. 
#         device_desc = "baud0"             * device description, 15 char max. 
#         reg_rules = "hxesfmba/default"    * reg 
#         emc_rules = "hxesfmba/default"    * emc 
#         start_halted = "y"                * exerciser halted at startup 
 
 
############################################################################### 
###################     Start of the BEGIN Routine    ######################### 
############################################################################### 
 
BEGIN { 
 
######################## CONFIGURATION DETERMINATION ##########################

# CMVC_RELEASE is exported from htx_setup.sh
    CMVC_RELEASE = snarf("echo $CMVC_RELEASE"); 
    
# Get HTX_LOG_DIR export variable
    LOG_DIR = snarf("echo $HTX_LOG_DIR");
    
    
# Get HTX_LOG_DIR export variable
    HOME_DIR = snarf("echo $HTX_HOME_DIR");    
    
# Get HTXREGRULES export variable
    REG_RULES = snarf("echo $HTXREGRULES");
    
# Get HTXREGRULES export variable
    SCRIPTS_DIR = snarf("echo $HTXSCRIPTS");
    
# Get HTXnoise file to prints logs from thie script
    HTXnoise_file = snarf("echo $HTXNoise");

# Get the current excutable script name 
    script_name = basename(ENVIRON["_"]);
    
# Get PVR of processor 
    cmd = sprintf("grep Version %s/htx_syscfg | awk -F: '{print $2}' | awk -Fx '{print $2}'", LOG_DIR);
    proc_ver = snarf(cmd);
    proc_os = snarf("cat /proc/cpuinfo | grep cpu | sort -u | awk '{ print $3 }'");

    cmd =  sprintf("grep 'TRUE PVR ver' %s/htx_syscfg | awk -F: '{print $2}' | awk -Fx '{print $2}'", LOG_DIR);
    true_pvr = snarf(cmd);

# Get P8CompatMode
    cmd = sprintf("grep Power8Compatmode %s/htx_syscfg | awk -F: '{print $2}'", LOG_DIR);
    p8_compat_mode = snarf(cmd);

# Get number of cores in system
    cmd = sprintf("awk -F : '/Number of Cores/ {print $2}' %s/htx_syscfg", LOG_DIR);
    num_cores = snarf(cmd);

# Get the number of chips in the system
    cmd = sprintf("awk -F : '/Number of chips/ {print $2}' %s/htx_syscfg", LOG_DIR);
    num_chips = snarf(cmd);

# Determine CPU type. 
    proc=snarf("uname -m");
    power7=snarf("cat /proc/cpuinfo | grep POWER7 | wc -l");

# Determine system SMT.
    cmd = sprintf("awk -F: '/Smt threads/ {print $2}' %s/htx_syscfg", LOG_DIR);
    smt_threads = snarf(cmd);
    hw_smt = smt_threads;

# Determine logical cpus.
    num_cpus = snarf("grep -i processor /proc/cpuinfo | wc -l");
    pcpus = snarf("ls -l /proc/device-tree/cpus | grep ^d | awk '($NF ~ /POWER/)' | wc -l");
   
# Determine Processor mode (Shared or Dedicated).
    spm = 1;
    cmd = sprintf("awk -F : '/shared_processor_mode/ {print $2}' %s/htx_syscfg", LOG_DIR);
    shared_processor_mode = snarf(cmd)
    if (shared_processor_mode == " no") 
	 spm = 0;
	
# Determine system Endianness
    cmd = sprintf("cat %s/htx_syscfg | grep -i Endianness | cut -f2 -d:", LOG_DIR);
    endian = snarf(cmd);

    printf ("[%s]: Running part.pl. All the logs for this will go to %s/part.pl.log\n", script_name, LOG_DIR) >> HTXnoise_file;
    system("${HTXSCRIPTS}/part.pl >/dev/null"); 
    system("${HTXSCRIPTS}/htxinfo.pl > ${HTX_HOME_DIR}/htxlinuxlevel ");
    system("(echo 1 > /proc/sys/kernel/kdb) 2>> $HTXNoise");
 
######################## MDTs CREATION LOGIC START ############################
# always generate these mdt entries 
    printf("default:\n");
    HE_name(""); 
    adapt_desc(""); 
    device_desc(""); 
    string_stanza("reg_rules","","reg rules"); 
    string_stanza("emc_rules","","emc rules"); 
    dma_chan(0); 
    idle_time(0); 
    intrpt_lev(0) 
    load_seq(32768); 
    max_run_tm(7200); 
    port("0");
    priority(19);
    slot("0");
    max_cycles("0");
    hft(0); 
    cont_on_err("NO"); 
    halt_level("1"); 
    start_halted("n");
    dup_device("n");
    log_vpd("y"); 

    create_memory_stanzas(); 

	num_lines = ""; 
	cmd = sprintf("cat %s/rawpart | wc | awk 'NR==1 {print $1}'", LOG_DIR);
	num_lines = snarf(cmd); 
	num_cd_lines = ""; 
	cmd = sprintf("cat %s/cdpart | wc | awk 'NR==1 {print $1}'", LOG_DIR);
	num_cd_lines = snarf(cmd); 
	num_dvdr_lines = ""; 
	cmd = sprintf("cat %s/dvdrpart 2> /dev/null| wc | awk 'NR==1 {print $1}'", LOG_DIR);
	num_dvdr_lines = snarf(cmd); 
	num_dvdw_lines = ""; 
	cmd = sprintf("cat %s/dvdwpart 2> /dev/null | wc | awk 'NR==1 {print $1}'", LOG_DIR);
	num_dvdw_lines = snarf(cmd); 
	num_td_lines = ""; 
	cmd = sprintf("cat %s/tdpart | wc | awk 'NR==1 {print $1}'", LOG_DIR);
	num_td_lines = snarf(cmd); 
	num_mp_lines = ""; 
	cmd = sprintf("cat %s/mpath_parts  2> /dev/null | wc | awk 'NR==1 {print $1}'", LOG_DIR);
	num_mp_lines = snarf(cmd);	
	
	tmp = ""; 
	dev = ""; 
	dev1 = ""; 
	dev2 = ""; 
	cdrom = ""; 
	command =""; 
	v_driver = "";
	ret = "";
	vios_setup = "";
 
# Stanza for scsi cd and dvd 
# Check if it's a VIOS setup
# First check if ibmvscsic module is loaded

	command = sprintf("lsmod | awk '$1 ~ /ibmvscsic/ {print $1 }'");
	v_driver = snarf(command);
	if ( v_driver == "ibmvscsic" ) {
	    command = sprintf("cat /proc/scsi/scsi | grep -A1 VOPTA | grep Type | grep CD-ROM > /dev/null; echo $?");
	    ret = snarf(command);
	    if ( ret == "0" )
		vios_setup = "1";
	}
# Check if CDROM drive is found on system 
	for(count=0; count < num_cd_lines; count++) { 
	    command = sprintf("cat %s/cdpart 2> /dev/null | awk 'NR==%d {print $1}'", LOG_DIR, (count+1)); 
	    cdrom = snarf(command); 
		if ( vios_setup == "1" )
	    	mkstanza("hxecd","vscsi","CD",cdrom,"hxecd","cdrom.vios");
		else
	    	mkstanza("hxecd","scsi","CD",cdrom,"hxecd","cdrom.mm");
	} 
 
# Check if DVDROM drive is found on system 
	for(count=0; count < num_dvdr_lines; count++) { 
	    command = sprintf("cat %s/dvdrpart 2> /dev/null | awk 'NR==%d {print $1}'", LOG_DIR, (count+1)); 
	    dvd = snarf(command); 
	if ( vios_setup == "1" )
	    mkstanza("hxecd","vscsi","DVD ROM",dvd,"hxecd","dvdrom.p1");
	else
	    mkstanza("hxecd","scsi","DVD ROM",dvd,"hxecd","dvdrom.p1");
	}
# Check if DVDRAM drive is found on system 
	for(count=0; count < num_dvdw_lines; count++) { 
	    command = sprintf("cat %s/dvdwpart 2> /dev/null | awk 'NR==%d {print $1}'", LOG_DIR, count+1); 
	    dvd = snarf(command);
		snarf("rm -rf /tmp/htx_chk_disk >/dev/null 2>&1");
		command = sprintf("%s/check_disk /dev/%s > /dev/null 2>&1; echo $? > /tmp/htx_chk_disk", SCRIPTS_DIR, dvd);
		snarf(command);
		exit_code=snarf("cat /tmp/htx_chk_disk");
		snarf("rm -rf /tmp/htx_chk_disk >/dev/null 2>&1");
		if (exit_code == "1") { 
			continue; 
		}
		mkstanza("hxestorage","scsi","DVD RAM",dvd,"hxestorage","default.dvd"); 
	}
 
	if(num_mp_lines) { 
		for(i=1; i<(num_mp_lines+1); i++) { 
			tmp=sprintf("cat %s/mpath_parts 2> /dev/null | awk 'NR==%d {print $1}'",LOG_DIR, i);
			path=snarf(tmp);  
			dev=sprintf("%s", path); 
			disk_type="";
			tmp=sprintf("cat /sys/block/%s/queue/rotational 2> /dev/null", dev);
			disk_type=snarf(tmp);

			# if disk_type is 1, means HDD. Otherwise, SSD
			if(disk_type == 1) { 
				mkstanza("hxestorage","scsi","mpaths",dev,"hxestorage","default.hdd","default.hdd"); 
			} else {
				mkstanza("hxestorage","scsi","mpaths",dev,"hxestorage","default.ssd","default.ssd");
			}
		}
	}  
	
	for(i=1;i<(num_lines+1);i++) { 
		tmp=sprintf("cat %s/rawpart 2> /dev/null | awk 'NR==%d {print $1}'",LOG_DIR, i); 
		dev=snarf(tmp); 
		tmp=sprintf("cat %s/rawpart | awk 'NR==%d {print $1}'",LOG_DIR, i); 
		dev2=snarf(tmp);
		snarf("rm -rf /tmp/htx_chk_disk >/dev/null 2>&1");
		command = sprintf("%s/check_disk /dev/%s > /dev/null 2>&1; echo $? > /tmp/htx_chk_disk", SCRIPTS_DIR, dev);
		snarf(command);
		exit_code=snarf("cat /tmp/htx_chk_disk");
		snarf("rm -rf /tmp/htx_chk_disk >/dev/null 2>&1");
		if (exit_code == "1") { 
			continue;
		}
		if(dev2 == cdrom) continue;  
 
		if(dev ~ "sd") { 
			tmp=sprintf("echo %s | tr -d '[0-9]'", dev);
		} else { 
			tmp=sprintf("echo %s", dev);
		}
		disk_name=snarf(tmp);
		disk_type="";
		tmp=sprintf("cat /sys/block/%s/queue/rotational 2> /dev/null", disk_name);
		disk_type=snarf(tmp);
		
		# if disk_type is 1 means HDD , if disk_type is 0 then its SSD.  Otherwise, lagacy IDE-Volumes, so associate HDD
		if(disk_type == 1) {
			mkstanza("hxestorage","scsi","scsi-vols",dev2,"hxestorage","default.hdd","default.hdd"); 
		} else if(disk_type == 0) {  
			mkstanza("hxestorage","scsi","scsi-vols",dev2,"hxestorage","default.ssd","default.ssd");
		} else {
			mkstanza("hxestorage","ide","ide-vols",dev2,"hxestorage","default.hdd","default.hdd"); 
		} 
 	} 
 
	command = ""; 
	result = ""; 
	devname = ""; 
	dvname = ""; 
	tmp = ""; 

	for(count=0;count <num_td_lines ;count++) { 
		tmp = sprintf("cat %s/tdpart | awk 'NR==%d {print $1}'",LOG_DIR, count+1); 
		dvname = snarf(tmp); 

		if( dvname != "" ) { 
			mkstanza("hxetape","scsi","scsi-tape",dvname,"hxetape","scsd","scsd"); 
		} 
		else { 
			break; 
		} 
	} 

# Detect normal serial ports 

	command = ""; 
	result = ""; 
	devname = ""; 

	for(count=0; ;count++) { 
	    command = sprintf("setserial /dev/ttyS%d -b 2>/dev/null | wc -l | awk 'NR==1 {print $1}'",count); 
	    result = snarf(command); 
	    devname = sprintf("ttyS%d",count); 

	    if(result != "0" ) { 
			mkstanza("hxeasy","ASY","unknown",devname,"hxeasy","default","default"); 
	    } 
	    else { 
			break; 
	    }
	} 

# Detect Jasmine serial port 

	command = ""; 
	result = ""; 
	devname = ""; 
	cmd=sprintf("lspci 2> /dev/null |grep 'Serial controller'"); 
	adapter=snarf(cmd); 
	if(adapter) { 
		cmd=sprintf("lsmod | awk '($1 == \"jsm\")'"); 
		driver=snarf(cmd); 
		if(driver) { 
			for(count=0; ;count++) { 
				command = sprintf("setserial /dev/ttyn%d -b 2>/dev/null | wc -l | awk 'NR==1 {print $1}' ",count); 
				result = snarf(command); 
				devname = sprintf("ttyn%d",count); 
				if(result != "0" ) { 
					mkstanza("hxeasy","ASY","unknown",devname,"hxeasy","default","default"); 
				} 
				else { 
					break; 
				} 
			} 
		} 
	} 


# Detect Hibiscus serial port using ditty command

	command = "";
	result = "";
	devname = "";
	cmd=sprintf("lsmod | awk '($1 == \"dgrp\")'");
	driver=snarf(cmd);
	if(driver) {
	    for(i=0; i<=7; i++) {
			for(count=0; count<=15; count++) {
		    	if(count < 10) {
					command = sprintf("/usr/bin/ditty -a ttyr%d0%d 2>/dev/null | grep speed",i,count);
			    } 
			    else {
					command = sprintf("/usr/bin/ditty -a ttyr%d%d 2>/dev/null | grep speed",i,count);
				}
		    	result = snarf(command);
			    if(count < 10) {
					devname = sprintf("ttyr%d0%d",i,count);
			    }
			    else {
					devname = sprintf("ttyr%d%d",i,count);
				}
			    if(result != "" ) {
					mkstanza("hxeasy","ASY","unknown",devname,"hxeasy","default","default");
		    	}
			    else {
					break;
			    }
			}
	    }
	}

# SCTU Stanza creation.  
# Create SCTU Stanzas only for lpars with dedicated cpus.
# Effective smt_per_gang is calculated per gang of 2 cores and servers created accordingly.

	if ( spm == 0 ) {
		sctu_gang_size = 2;
		rule_file = "rules.default"
        if ( proc_ver == "4e" || proc_ver == "4f" ) {
            # /* P9 Nimbus and Cumulus */
            sctu_gang_size = 4;
            rule_file = "default.p9"
        }

		num_core_gangs = int(num_cores/sctu_gang_size);
		# below check is for gang_size > 2
		if((num_cores % (sctu_gang_size)) > 1 ){
			num_core_gangs = num_core_gangs + 1;
		}
		system("cat ${HTX_LOG_DIR}/htx_syscfg | grep 'CPUs in core' > ${HTX_LOG_DIR}/core_config");
		for(g=0; g < num_core_gangs; g++) {
			line1 = g*2 + 1;
			line2 = g*2 + 2;
			cmd = sprintf("awk '(NR >= %d && NR <= %d)' %s/core_config | cut -d '(' -f 2 | cut -c 1 | sort -n -r | awk '(NR==2)'", line1, line2, LOG_DIR);
			smt_per_gang = snarf(cmd);
			for(s=0; s < smt_per_gang; s++) {
				dev_num = s + g*hw_smt;
                # escape creating device for CPU 0... 
                if ((dev_num % smt_per_gang) == 0) {
                    continue;
                }
                dev_name=sprintf("sctu%d", dev_num);

				mkstanza("hxesctu","cache","coherence test",dev_name,"sctu", rule_file, rule_file);
			}
		}
	} else {
		printf("[%s]: System is in shared processor mode, No sctu devices will be created\n", script_name) >> HTXnoise_file;
	}
# NOTE: the rfdir field (#5) will not be used to create the relative path for the rules file. 
#       the he_name field(#1) is used. not sure why this was designed like this....  

# HXECACHE Stanza

	y = 0 ;
	# Set number of pages required per instance of cache, depending on P6/P7
	if(proc_ver=="3e") {
		no_pages_reserved_per_instance = 16;
		rule_file_name=sprintf("default.p6");
	}
	else if(proc_ver=="3f") {
		no_pages_reserved_per_instance = 4;
		rule_file_name=sprintf("default.p7");
	}
	else if(proc_ver == "4a") {
		no_pages_reserved_per_instance = 64;
		rule_file_name=sprintf("default.p7");
	}
	else if(proc_ver == "4b" || proc_ver == "4d" || proc_ver == "4c" ) {
        if(true_pvr == "4e" || true_pvr == "4f" ) {
            rule_file_name=sprintf("default.p9");
        } else {
		    no_pages_reserved_per_instance = 32;
		    rule_file_name=sprintf("default.p8");
        }
	}
    	else if(proc_ver == "4e" || proc_ver == "4f" ) {
	            rule_file_name=sprintf("default.p9");
        }

	if ( proc_os == "POWER6" ) {
		rule_file_name=sprintf("default.p6");
	}

	# If P6 & htxltsml, then hxecache is not supported (miscex module reqd),else create stanzas
	if( !(proc_ver=="3e" && CMVC_RELEASE == "htxltsbml")) {
 
		# Check if sufficient hugepages available and whether the system is not running
		# shared_processor_mode, then create stanza's
		# spm = Shared Processor Mode
		# In case of BML, hardcode spm to 0

		cmd = sprintf("cat %s/freepages | grep cache_16M | awk -F= '{print $2}'", LOG_DIR);
		cache_16M=snarf(cmd);

		if(cache_16M != 0 && spm==0) {
			if( proc_ver == "3e" || ( proc_ver == "3f" && proc_os == "POWER6" ) ) {
				CpuSet_cache = 16 * smt_threads;
				for(x=0; x<num_cpus; x+=CpuSet_cache) {
					dev_name=sprintf("cache%d",y++);
					mkstanza("hxecache_p6","","Processor Cache",dev_name,"hxecache_p6",rule_file_name,rule_file_name);
				}
				cmd = sprintf("ln -sf %s/hxecache %s/hxecache_p6", REG_RULES, REG_RULES);
				snarf(cmd);
			}
			else {
				for(x=0;x<num_chips;x+=1) {
					dev_name=sprintf("cache%d",y++);
					mkstanza("hxecache","","Processor Cache",dev_name,"hxecache",rule_file_name,rule_file_name);
				}
			}
		}
	}

    # Determine rule file for fpu  and cpu devices.

	if ( (proc_ver == "3e") || (proc_ver == "3f" && proc_os == "POWER6") ) {
		# /* P6 || P6 Compat mode */
		fpu_rule_file = "default.p6";
		cpu_rule_file = "default.p6";
	}
	else if ( proc_ver == "3f" || proc_ver == "4a" ) {
		# /* P7 and P7+*/
		fpu_rule_file = "default.p7";
		cpu_rule_file = "default.p7";
	}
	else if ( proc_ver == "4b" || proc_ver == "4d" || proc_ver == "4c" ) {
		# /* P8 Murano and Venice */
		fpu_rule_file = "default.p8";
		cpu_rule_file = "default.p8";
	}
	else if ( proc_ver == "4e" || proc_ver == "4f" ) {
		# /* P9 Nimbus and Cumulus */
		fpu_rule_file = "default.p9";
		cpu_rule_file = "default.p9";
	}

    # hxefpu64 and hxecpu stanza creation.
    if (spm == 1) {
        num_devices = num_cpus;
    } else {
        num_devices = num_cores;
    }

    for(i = 0; i < num_devices; i++) {
        dev_name = sprintf("fpu%d", i);
		mkstanza("hxefpu64", "core", "floating_point", dev_name, "fpu", fpu_rule_file, fpu_rule_file);
    }

    for(i = 0; i < num_devices; i++) {
        dev_name = sprintf("cpu%d", i);
		mkstanza("hxecpu", "processor", "processor", dev_name, "cpu", cpu_rule_file, cpu_rule_file);
    }


	# RNG exerciser stanza creation.
	if ( proc_ver == "4a" || proc_ver == "4b" || proc_ver == "4c" || proc_ver == "4d" ) {
		rng_present = snarf("ls /dev/hwrng 2>/dev/null | wc -l");
		if(rng_present) {
			mkstanza("hxerng", "chip", "Misc", "rng", "hxerng", "rules.default", "");
		}
	}
	else {
		mkstanza("hxerng", "chip", "Misc", "rng", "hxerng", "rules.default", "");
	}

    # Corsa exerciser stanza creation.

    corsa_present = snarf("ls /dev/genwqe0_card 2>/dev/null | wc -l");
    if (corsa_present) {
		if((CMVC_RELEASE == "htxrhel72le") || (CMVC_RELEASE == "htxrhel7") || (CMVC_RELEASE == "htxrhel6" )) {
			mkstanza("hxecorsa", "chip", "Misc", "genwqe0_card", "hxecorsa", "default", "");
		}
    }

    corsa_capi_present = snarf("ls /dev/cxl/afu0.0s 2> /dev/null | wc -l");
    corsa_dev_type = snarf("cat /sys/class/cxl/afu0.0s/device/cr0/device 2> /dev/null");
    if ( corsa_capi_present == "1" && corsa_dev_type == "0x0602" ) {
                if((CMVC_RELEASE == "htxrhel72le") || (CMVC_RELEASE == "htxrhel7") || (CMVC_RELEASE == "htxubuntu")) {
                        mkstanza("hxecorsa", "chip", "Misc", "afu0.0s", "hxecorsa", "default.capi", "" );
                }
    }

	cmd = "";
	temp_cmd = "";
	cmd = "ls /dev/cxl/ 2> /dev/null | awk '/afu[0-9].[0-9]d/ { print $1 }'";

	while(cmd | getline afu_device) {
		temp_cmd = sprintf("cat /sys/class/cxl/%s/device/mode",afu_device);
		if(snarf(temp_cmd) == "dedicated_process") {
			 mkstanza("hxecapi", "AFU MEMCOPY", "Corsa A5", afu_device, "hxecapi", "default","default");
		}
	}
	close(cmd);

    cmd = "";
	DIR = snarf("echo $HTX_HOME_DIR");

	ip=snarf("hostname -i");
	split(ip, ip_array, " ");
	for (cnt in ip_array) {
		dev=sprintf("ip -o -4 addr show | awk ' /%s/ { print $2 }' ", ip_array[cnt]);
		primary_iface[cnt]=snarf(dev);
	}

	cmd="ip link show | cut -d : -f 2 | awk '/eth[0-9]/ || /en[aA-zA]/' |tr -d ' '";
	while( cmd | getline intface ) {
		found = 0;
		for(cnt in primary_iface) {
			if(intface ~ primary_iface[cnt]) {
				found = 1;
                break;
			}
		}
		if(found == 0) {
			num_lines = "";
			cmd1 = sprintf("cat %s/htx_diag.config | wc | awk 'NR==1 {print $1}'", DIR);
			num_lines = snarf(cmd1);
			for(count=0; count < num_lines; count++) {
				command = sprintf("cat %s/htx_diag.config 2> /dev/null | awk -F':' 'NR==%d {print $1}'", DIR, (count+1));
				driver_name = snarf(command);
				command = sprintf("cat %s/htx_diag.config 2> /dev/null | awk -F':' 'NR==%d {print $2}'", DIR, (count+1));
				driver_desc=snarf(command);
				cmd1=sprintf("ethtool -i %s | awk -F : ' {print $2}' \n", intface);
				driver=snarf(cmd1);

				if(driver ~ driver_name) {
					mkstanza("hxediag", driver_desc,"NIC",intface,"hxediag", "default", "default" );
				}
			}
		}
	}
	close(cmd);


	#Enable hydepark diag test for ubuntu only right now ..
	cmd="ls /sys/class/infiniband/ 2>/dev/null | awk ' /mlx[0-9]_[0-9]/'";
	rules="default.ib";
	adaptdesc="IB";
	devicedesc="";
	gp_dev = 0;
	gp1p_dev = 0;
	while( cmd | getline ib_dev ) {
		# Gather some info about this device.
		cmd1=sprintf("cat /sys/class/infiniband/%s/board_id \n", ib_dev);
		dev_name=snarf(cmd1);
		close(cmd1);
		cmd1=sprintf("ls /sys/class/infiniband/%s/device/net/ | awk '/eth[0-9]/' \n", ib_dev);
		roce=snarf(cmd1);
		close(cmd1);
		cmd1=sprintf("ls /sys/class/infiniband/%s/device/net/ | awk '/ib[0-9]/' \n", ib_dev);
		ipoib=snarf(cmd1);
		close(cmd1);
		if(dev_name ~/IBM2190110032/) { # Glacier Park
			rules="default.ib_gp";
			devicedesc="Glacier Park";
			gp_dev += 1;
		} else if(dev_name ~/IBM1210111019/) { # Hyde Park
			rules="default.ib";
			devicedesc="Hyde Park";
		} else if(dev_name ~/IBM2180110032/) { # GP 1 Port
			rules="ib_gp1p";
			devicedesc="GP 1Port";
			gp1p_dev += 1;
		} else {
			continue;
		}
		# printf("ib_dev=%s, devicedesc=%s, roce=%s, ipoib=%s, dev_name=%s, rules = %s\n",ib_dev, devicedesc, roce, ipoib, dev_name, rules);
		if(roce) continue;
		if(devicedesc ~ /Glacier Park$/ && ((gp_dev % 2 ) == 0)) continue;
		if((devicedesc ~ /GP 1Port$/) && (rules ~ /ib_gp1p$/) && (gp1p_dev != 1)) continue;
		mkstanza("hxediag", adaptdesc, devicedesc, ib_dev, "hxediag", rules, rules);
	}
	close(cmd);

   	ibm_internal = snarf("ls -l ${HTX_HOME_DIR}/.internal 2> /dev/null | wc -l");
}


############################################################################### 
####################     End of the BEGIN Routine    ########################## 
############################################################################### 

############################################################################## 
#####################     END OF MAIN PROGRAM     ############################ 
############################################################################## 


############################################################################## 
###################     Start of the END Routine    ########################## 
############################################################################## 

END { printf("\n"); } 

############################################################################## 
####################     End of the END Routine    ########################### 
############################################################################## 


############################################################################## 
#####################     FUNCTION DEFINITIONS     ########################### 
############################################################################## 
function deleted(dev) { 
    if (flag[dev] == "-d") return 1 
} 
function string_stanza(a,b,c) { 
    len=(length(a) + length(b) + 8 + 3 + 2) - 50 
    printf("\t%s = \"%s\" %" len "s * %s\n",a,b,"",c);
} 
function number_stanza(a,b,c) { 
    len=(length(a) + length(b) + 8 + 3) - 50 
    printf("\t%s = %s %" len "s * %s\n",a,b,"",c);
}
function HE_name(x) { 
    string_stanza("HE_name",x,"Hardware Exerciser name, 14 char");
} 
function max_cycles(x) { 
    string_stanza("max_cycles",x,"max cycles");
} 
function adapt_desc(x) {
    gsub(" ","_",x); 
    string_stanza("adapt_desc",x,"adapter description, 11 char max.");
} 
function cont_on_err(x) { 
    string_stanza("cont_on_err",x,"continue on error (YES/NO)");
}
function device_desc(x) {
    gsub(" ","_",x); 
    string_stanza("device_desc",x,"device description, 15 char max.");
}
function halt_level(x) { 
    string_stanza("halt_level",x,"level <= which HE halts");
}
function start_halted(x) {
    string_stanza("start_halted",x,"exerciser halted at startup");
}
function dup_device(x) {
    string_stanza("dup_device",x,"duplicate the device ");
}
function log_vpd(x) {
    string_stanza("log_vpd",x,"Show detailed error log");
}
function port(x) {
    string_stanza("port",x,"port number");
}
function slot(x) {
    string_stanza("slot",x,"slot number");
}
function dma_chan(x) {
    number_stanza("dma_chan",x,"DMA channel number");
}
function hft(x) {
    number_stanza("hft",x,"hft number");
}
function idle_time(x) {
    number_stanza("idle_time",x,"idle time (secs)");
}
function intrpt_lev(x) {
    number_stanza("intrpt_lev",x,"interrupt level");
}
function load_seq(x) {
    number_stanza("load_seq",x,"load sequence (1 - 65535)");
}
function max_run_tm(x) {
    number_stanza("max_run_tm",x,"max run time (secs)");
}
function priority(x) {
    number_stanza("priority",x,"priority (1=highest to 19=lowest)");
}
function snarf(cmd) { 
    snarf_input=""; 
    cmd | getline snarf_input; close(cmd); return snarf_input; 
} 
function basename(file) {
    sub(".*/", "", file)
    return file
}

# use mkstanza() to generate the basic stanza, arguments are: 
#     hename,                           # "hxecd" 
#     adapt_desc,                       # "scsi" 
#     device_desc,                      # "cdrom" 
#     device_name,                      # "rcd0" 
#     rulesfile_directory,              # "cd" 
#     default_reg_rules,                # "cdrom.ibm" 
#     default_emc_rules                 # "cdrom.ibm" 
# 

function mkstanza(hxe,a,d,dev,rfdir,reg,emc) { 
# for 4.1, rules file directory is same as exerciser name 
    rfdir = hxe 
    printf("\n")
    printf("%s:\n",dev);

# make exerciser entry 
    if(he[dev]) { 
	HE_name(he[dev]); 
# for 3.2, rules file directory is exerciser name minus leading "hxe" 
	rfdir = he[dev]; 
	sub(/^hxe/,"",rfdir); 
# for 4.1, rules file directory is same as exerciser name 
	rfdir = he[dev]; 
    }
    else { 
	if (hxe) HE_name(hxe); 
    }

# make adapter and device description entries 
    if (a) adapt_desc(a) 
    if (d) device_desc(d) 

# make rules file entries 
	if(rf[dev]) { 
		string_stanza("reg_rules",sprintf("%s/%s",rfdir,rf[dev]),"reg"); 
		string_stanza("emc_rules",sprintf("%s/%s",rfdir,rf[dev]),"emc"); 
	}
	else { 
		if(reg) 
		    string_stanza("reg_rules",sprintf("%s/%s",rfdir,reg),"reg"); 
		if(emc) 
		    string_stanza("emc_rules",sprintf("%s/%s",rfdir,emc),"emc"); 
	}
# make start_halted entry 
    if(flag[dev] == "-h") 
	string_stanza("start_halted","y","exerciser halted at startup"); 
}

function create_memory_stanzas() { 

	ams=0
	if ( true_pvr == "4e" || true_pvr == "4f" )  {
		if (CMVC_RELEASE != "htxltsbml") {
			ams=snarf("cat /proc/ppc64/lparcfg | grep cmo_enabled | awk -F= '{print $2}'")
			if (ams == "1") {
				system("awk '/.*/ { if ($0 ~ /^global_alloc_mem_percent/ )printf(\"global_alloc_mem_percent = 40\\n\"); else print $0; }' ${HTXREGRULES}/hxemem64/default > ${HTXREGRULES}/hxemem64/default.ams");
				mkstanza("hxemem64","64bit","memory","mem","hxemem64","default.ams","default.ams");
			}
			else {
				mkstanza("hxemem64","64bit","memory","mem","hxemem64","default","default");
			}
		}
		else {
			mkstanza("hxemem64","64bit","memory","mem","hxemem64","default","default");
		}
	}
	else {
		if (CMVC_RELEASE != "htxltsbml") {
			ams=snarf("cat /proc/ppc64/lparcfg | grep cmo_enabled | awk -F= '{print $2}'")
			if (ams == "1") {
				system("awk '/.*/ { if ($0 ~ /^max_mem/ )printf(\"max_mem = yes\\nmem_percent = 40\\n\"); else print $0; }' ${HTXREGRULES}/hxemem64/maxmem > ${HTXREGRULES}/hxemem64/maxmem.ams");
				mkstanza("hxemem64","64bit","memory","mem","hxemem64","maxmem.ams","maxmem.ams");
			}
			else {
				mkstanza("hxemem64","64bit","memory","mem","hxemem64","maxmem","maxmem");
			}
		}
		else {
			mkstanza("hxemem64","64bit","memory","mem","hxemem64","maxmem","maxmem");
		}
	}
	load_seq(65535);
	loop_cnt=((log_proc-(log_proc%2))/2);
	if(loop_cnt==0) {
	    loop_cnt=1;
	}	

    if ( spm==0 ) {
		mem_name=sprintf("tlbie");
		mkstanza("hxetlbie","64bit","memory",mem_name,"hxetlbie","tlbie","tlbie");
		load_seq(65535);
	}
}

############################################################################## 
##################     END OF FUNCTION DEFINITIONS     ####################### 
############################################################################## 

