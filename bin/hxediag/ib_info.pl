#!/usr/bin/perl 
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


use Getopt::Long;

#Preliminary checks for opensm and Mellanox OFED 
$opensm_path="/usr/sbin/opensm"; 
$opensm_service_path="/etc/init.d/opensmd"; 
$opensm_pid="/var/run/opensm.pid"; 
$opensm = 0; 
$opensmd = 0; 

$quiet = 0; 
$device=""; 
$port=0; 
$all = 0;
GetOptions("q"=>\$quiet,  # Be quiet, only relevent info dumped.  
		"a"=>\$all, 	  # query all Mellanox device. 
	   "d=s"=>\$device,   # if specific device needs to be queried. 
	   "p=s"=>\$port);    # if specific device's port state needs to be queried.

print("Command line invocation : quiet=$query, device=$device, port=$port \n") if(!$quiet);

if(-e $opensm_pid) { #OpenSM Already running  
	$opensmd = 1; 
} elsif (-e $opensm_service_path) { # Try starting OpenSM Service 
	print("OpenSMD Daemon present, will start in daemon mode \n") if(!$quiet);
    $rc = `service opensmd start`;
    if( -e $opensm_pid) {
    	print("opensm daemon successfully started \n") if(!$quiet);
        $opensmd = 1;
    } else {
    	print("opensm daemon failed to start, Refer /var/log/opensm.log for more information. \n") if(!$quiet);
        $opensmd = 0;
    }
} else {
	$opensmd = 0;
   	print("OpenSM service not present   \n") if(!$quiet);
}
if ( -e $opensm_path) { # Check if open sm binary is present. 
	$opensm = 1;  
} else {  # Cannot do anything w/o opensm, exit ... 
    $opensm = 0;
    print("OpenSM not installed \n") if(!$quiet);
    exit(1);

}
#Now start quering devices 
@ib_devices=&get_ib_devices();
if($all) { 
	exit(0); 
}
foreach $ib_device (@ib_devices) { 
	$ib_device=&trim($ib_device); 
	local($i); 

	$path="/sys/class/infiniband/$ib_device";
    if(!(-d $path)) {
		print("Device not configured ib_device=$ib_device \n") if(!$quiet); 
		exit(2);
    }
	
	@port = &get_port_numbers($ib_device); 

	for($i=0; $i < &num_ports($ib_device); $i++ ) { 
		$port[$i] = &trim($port[$i]);
		print("Querying for dev=$ib_device, port=$port[$i] \n") if(!$quiet); 
		$state=&get_port_state($ib_device, $port[$i]); 	
		#Get GID for  
		$guid=&get_node_guid($ib_device); 
		if($state =~/DOWN/i) { 
			print("No port to port connection seems to be presnet. Device=$ib_device, port=$port[$i], state=$state \n") if(!$quiet); 	
			exit(3); 	
		} elsif($state =~/INIT/) { 
			if($opensm == 1) { # if OpenSMD is not running then this is expected
				#try running opensm binary to get port to active state. 
				$rc = &run_opensm_on_ports($ib_device, $port[$i], $guid); 
				if(rc != 0) { 
					print(" Check for incompatible cables. Device=$ib_device, port=$port[$i], state=$state\n") if(!$quiet);
					exit(4); 
				}		
			} else { 
				print("Port to port connection detected, but incompatible  cables used. Device=$ib_device, port=$port[$i],  state=$state \n") if(!$quiet);	
			}
		} elsif($state =~ /ACTIVE/) { 
			print("Ports ACTIVE, good to run DMA & MMIO's. Device=$ib_device, port=$port[$i], state=$state \n") if(!$quiet); 
		} else { 
			print("Unknown port state.  Device=$ib_device, port=$port[$i], state=$state \n") if(!$quiet);	
		}	
	} 
}
exit(0); 

############################################################################################################################
# Function Definition 
############################################################################################################################

sub run_opensm_on_ports() { 

	local($ib_device, $port, $guid) = @_;
    $retry=0;
    do {
    	print ("Running command : $opensm_path -g  $guid -o, retry=$retry \n") if(!$quiet);
    	`$opensm_path -g  $guid -o`;
        $state=&get_port_state($ib_device, $port);
        if($state =~ /ACTIVE/) {
     		print("Port ACTIVE, good to run DMA & MMIO's. Device=$ib_device, port=$port, state=$state \n") if(!$quiet);
            return(0);
         }
         $retry++;
         sleep($retry);
	} while($retry < 5);
    $state=&get_port_state($ib_device, $port);
    if($state =~ /ACTIVE/) {
        print("Port ACTIVE, good to run DMA & MMIO's. Device=$ib_device, port=$port, state=$state \n") if(!$quiet);
        return(0);
    } else {
        print("OpenSM could not bring ports to active state. Device=$ib_device, port=$port, state=$state\n") if(!$quiet);
        return(5);
    }
    return(0);  
}


sub get_port_state() { 
	local($dev, $port)=@_; 
	$dev=&trim($dev); 
	$port=&trim($port); 

	$path="/sys/class/infiniband/$dev/ports/$port/state"; 
	
	$state=`cat $path | cut -d : -f 2 `; 
	$state=&trim($state); 
	
	print("$state \n"); 
	return($state); 
} 

sub get_node_guid() { 

	local($dev) = @_; 
	$dev=&trim($dev);

	$path="/sys/class/infiniband/$dev/node_guid"; 
	$guid=`cat $path`; 
	# remove delimiter to get complete qualified string 
	@array=split(/:/,$guid); 
	$guid_string="0x". join('', @array); 	

	print("Device=$dev, GUID = $guid_string \n") if(!$quiet); 
	$guid_string=&trim($guid_string); 

	return($guid_string); 
}

sub num_ports() { 

	local($dev)=@_; 

	$dev=&trim($dev);
    $path="/sys/class/infiniband/$dev/ports/";
    $num_ports=`ls $path | wc -l`;
	if($port > 0) { 
		#port to be queried specified from command line, query for that port only 
		$num_ports = 1; 
	}
	return($num_ports); 
	
}

sub get_port_numbers() { 

	local($dev)=@_;

  	$dev=&trim($dev);
	if($port >  0) { 
		$port = &trim($port); 
    	$path="/sys/class/infiniband/$dev/ports/$port";
        if(!(-d $path)) {
            print(" Input port=$port for dev=$dev, Device doesnot exists \n") if(!$quiet);
            exit(1);
        }
		$port_number[0]=$port; 
	} else {  		
    	$path="/sys/class/infiniband/$dev/ports/";
    	@port_number=`ls $path`; 
	}
	print("Device=$dev, port_number=@port_number \n") if(!$quiet); 

	return(@port_number); 	
}		


sub get_ib_devices() { 


	@available_devices=""; 
	$num_devices = 0; 	
	if($device =~ /mlx/) {  
		$configured_ib_devices[$num_devices] = &trim($device); 
		$path="/sys/class/infiniband/$configured_ib_devices[$num_devices]"; 
		if(!(-d $path)) { 
			print(" $configured_ib_devices[$num_devices]. Device doesnot exists \n") if(!$quiet); 
			exit(1); 
		}	
	} else { 
		@configured_ib_devices = `ibstat -l`; 
	}
	$num_devices = 0; 	
	foreach $ib_dev (@configured_ib_devices) { 
		chomp($configured_ib_devices); 
		#should be of form mlx 
		if($ib_dev =~ /mlx/) { 
			$ib_dev=&trim($ib_dev); 
			$available_devices[$num_devices]=$ib_dev; 
			$num_devices = $num_devices + 1; 
		} else { 	
			print("Unknown device = $ib_dev. ignoring \n") if(!$quiet); 
		}
	}
	print("Discovered following devices = @available_devices \n") if(!$quiet || $all); 
	return(@available_devices); 

}

sub trim {

    local($string) = @_;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}


