#!/bin/awk -f

# IBM_PROLOG_BEGIN_TAG
# 
# Copyright 2003,2016 IBM International Business Machines Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# 		 http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# IBM_PROLOG_END_TAG


BEGIN {
	log_dir=ENVIRON["HTX_LOG_DIR"]
	
	# Do not create sctu stanzas on lpars having shared processors.

	cmd = sprintf("awk -F : '/shared_processor_mode/ {print $2}' %s/htx_syscfg", log_dir);
	shared_processor_mode = snarf(cmd);
	if (shared_processor_mode == " yes")
		exit 0;
	
	# Get the number of nodes in the system
	cmd = sprintf("awk -F : '/Number of nodes/ {print $2}' %s/htx_syscfg", log_dir);
	num_nodes = snarf(cmd);

	# Get number of cores in system
	cmd = sprintf("awk -F : '/Number of Cores/ {print $2}' %s/htx_syscfg", log_dir);
	num_cores = snarf(cmd);

	# Determine system SMT.
	cmd = sprintf("awk -F: '/Smt threads/ {print $2}' %s/htx_syscfg", log_dir);
	hw_smt = snarf(cmd);

	# Determine PVR.
	cmd = sprintf("awk -F: '/Processor Version/ {print $2}' %s/htx_syscfg", log_dir);
	hw_pvr = snarf(cmd);

	sctu_gang_size = 8;
	num_core_gangs = int(num_cores/sctu_gang_size);

	if((num_cores % (sctu_gang_size)) > 1 ){
		num_core_gangs = num_core_gangs + 1;
	}

	# Create sctu stanza for Inter Node config.
	if ( ARGV[1] ~ /INTERNODE/ ) {
		if( num_nodes < 2 ) {
			exit 0;
		}

		system("grep 'Cores in node' ${HTX_LOG_DIR}/htx_syscfg > ${HTX_LOG_DIR}/node_config");

		cmd = sprintf("cat %s/node_config | cut -d '(' -f 2 | cut -d ')' -f 1 | sort -n -r | awk '(NR==2)'", log_dir);
		min_sctu_cores = snarf(cmd);
		num_sctu_cores = min_sctu_cores * num_nodes;
		num_core_gangs = int(num_sctu_cores/sctu_gang_size);

		if((num_sctu_cores % (sctu_gang_size)) > 1 ){
			num_core_gangs = num_core_gangs + 1;
		}
		cmd = sprintf("awk -F : '/Min smt threads/ {print $2}' %s/htx_syscfg", log_dir);
		min_smt = snarf(cmd);

		for(g=0; g < num_core_gangs; g++) {
			for(s=0; s < min_smt; s++) {
				dev_name=sprintf("sctu%d", s + g*hw_smt);
				mkstanza("hxesctu","cache","coherence test",dev_name,"hxesctu","rules.InterNode","rules.InterNode");
			}
		}
	}


	# Create sctu gangs within a chip. 
	else if ( ARGV[1] ~ /CHIP/ ) {
		cmd = sprintf("cat %s/htx_sctu_chip_dev | wc -l", log_dir);
		num_dev = snarf(cmd);

		for(i=0; i<num_dev; i++ ) {
			dev_name=sprintf("sctu%d", i);
			mkstanza("hxesctu","cache","coherence test",dev_name,"hxesctu","rules.chip","rules.chip");
		}
	}
	

	# Create sctu gangs within a node. 
	else if ( ARGV[1] ~ /NODE/ ) {
		cmd = sprintf("cat %s/htx_sctu_node_dev | wc -l", log_dir);
		num_dev = snarf(cmd);

		for(i=0; i<num_dev; i++ ) {
			dev_name=sprintf("sctu%d", i);
			mkstanza("hxesctu","cache","coherence test",dev_name,"hxesctu","rules.node","rules.node");
		}
	}

	# Create sctu stanzas half in number of sctu stanzas in mdt.bu.
	if(ARGV[1] ~ /MEM_NEST/) {
		if ( hw_pvr == " 0x4e" || hw_pvr == " 0x4f" ) {
			sctu_gang_size = 4;
		} else {
			sctu_gang_size = 2;
		}
		num_core_gangs = int(num_cores/sctu_gang_size);

		system("grep 'CPUs in core' ${HTX_LOG_DIR}/htx_syscfg > ${HTX_LOG_DIR}/core_config");
		for(g=0; g < num_core_gangs; g++) {
			line1 = g*2 + 1;
			line2 = g*2 + 2;
			cmd = sprintf("awk '(NR >= %d && NR <= %d)' %s/core_config | cut -d '(' -f 2 | cut -c 1 | sort -n -r | awk '(NR==2)'",line1,line2, log_dir);
			smt_per_gang = snarf(cmd);
			for(s=0; s < smt_per_gang; s++) {
				if(s%2 == 0) {
					dev_name=sprintf("sctu%d", s + g*hw_smt);
					mkstanza("hxesctu","cache","coherence test",dev_name,"hxesctu","rules.default","rules.default");
				}
			}
		}
	}


	# Create only one sctu device stanza for each sctu gang.
	if(ARGV[1] ~ /CHARACTERIZATION/) {

		if ( hw_pvr == " 0x4e" || hw_pvr == " 0x4f" ) {
			sctu_gang_size = 4;
		} else {
			sctu_gang_size = 2;
		}
		num_core_gangs = int(num_cores/sctu_gang_size);
		for(g=0; g < num_core_gangs; g++) {
		        # skip logical CPU0
			dev_name=sprintf("sctu%d", g*hw_smt + 1);
			mkstanza("hxesctu","cache","coherence test",dev_name,"hxesctu","rules.default","rules.default");
		}

	}

}


function HE_name(x)
{ string_stanza("HE_name",x,"Hardware Exerciser name, 14 char"); }

# use mkstanza() to generate the basic stanza :

function mkstanza(hxe,a,d,dev,rfdir,reg,emc)
{
# for 4.1, rules file directory is same as exerciser name
	rfdir = hxe

# make device name entry
		printf("\n%s:\n",dev);

# make exerciser entry
	if (he[dev]) {
		HE_name(he[dev]);
# for 3.2, rules file directory is exerciser name minus leading "hxe"
		rfdir = he[dev];
		sub(/^hxe/,"",rfdir);
# for 4.1, rules file directory is same as exerciser name
		rfdir = he[dev];
	} else {
		if (hxe) HE_name(hxe);
	}

# make adapter and device description entries
	if (a) adapt_desc(a)
		if (d) device_desc(d)

# make rules file entries
			if (rf[dev]) {
				string_stanza("reg_rules",sprintf("%s/%s",rfdir,rf[dev]),"reg");
				string_stanza("emc_rules",sprintf("%s/%s",rfdir,rf[dev]),"emc");
			} else {
				if (reg) string_stanza("reg_rules",sprintf("%s/%s",rfdir,reg),"reg");
				if (emc) string_stanza("emc_rules",sprintf("%s/%s",rfdir,emc),"emc");
			}

# make start_halted entry
	if (flag[dev] == "-h")
		string_stanza("start_halted","y","exerciser halted at startup");

# add a new-line at the end. Some scripts (powermixer) doesn't add new-line before adding new device
    printf("\n");
}


function snarf(cmd) {
	snarf_input="";
	cmd | getline snarf_input; close(cmd); return snarf_input;
}

function string_stanza(a,b,c) {
	len=(length(a) + length(b) + 8 + 3 + 2) - 40
		if (create_tlbie_rules) {
			printf("\t%s = \"%s\" %" len "s * %s\n",a,b,"",c) >> tlbie_mdtfile;
		}
		else
		{ printf("\t%s = \"%s\" %" len "s * %s\n",a,b,"",c);
		}
}

function adapt_desc(x) { gsub(" ","_",x);
	string_stanza("adapt_desc",x,"adapter description, 11 char max."); }

	function device_desc(x) { gsub(" ","_",x);
		string_stanza("device_desc",x,"device description, 15 char max."); }

