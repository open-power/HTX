#!/bin/bash

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

# Functions to print message in proper format


# Source htx_env.sh to get all exports/alias/functions
  HTX_HOME_DIR=`cat /var/log/htx_install_path`
  export HTX_HOME_DIR=${HTX_HOME_DIR}/
  if [ -f $HTX_HOME_DIR/etc/scripts/htx_env.sh ]; then
      . $HTX_HOME_DIR/etc/scripts/htx_env.sh
  else
      echo "/var/log/htx_install_path file is supposed to have HTX install directoy path."
      echo "Either file is empty or does not contain correct install path. Hence exiting..."
      exit 1
  fi

print_htx_log "exported HTX_HOME_DIR=$HTX_HOME_DIR"
print_htx_log "exported PATH=$PATH"

# Check if this script is invoked by root/equivalent. 
# If not, then return with error
  if [[ $EUID -ne 0 ]]; then
    print_htx_log "Must be root to run HTX"
    exit 1
  fi

# HTXNoise is a repository for HTX screen activity during login and runsup commands.
# This is an attempt to capture errors that may otherwise be lost

  # 1st message is invoked directly rather than using print_htx_log since we needed
  # to create a new file (clear content). Leave it like this. 
  echo "[`date`]: 1st HTX message." | tee $HTXNoise

# Check whether HTX is already running on system
  [ "`ps -ef | grep hxssup | grep -v grep`" ] && {
     print_htx_log "HTX is already running..." 
     exit 1
  }

  print_htx_log "exporting PATH=$PATH"

# Setting HTX environment specific alias
  alias logout=". ${HTXSCRIPTS}/htxlogout"
  alias exit=". ${HTXSCRIPTS}/htxlogout"
  alias htx=". runsup"
  alias stx="$HTXSCRIPTS_STX/runstx"
  alias stopstx="$HTXSCRIPTS_STX/stopstx"


  os_distribution=`grep ^NAME /etc/os-release | cut -f2 -d= | sed s/\"//g`
  if [ $os_distribution == "Ubuntu" ]
  then
    export HTX_RELEASE=htxubuntu
  else
    export HTX_RELEASE=`rpm -qa | grep ^htx | cut -f1 -d-`
  fi

  export CMVC_RELEASE=$HTX_RELEASE
  print_htx_log "exporting HTX_RELEASE=$HTX_RELEASE"

# All ulimit related updates
# Limit the stack to 8MB, if lesser as it's not done in some distros.
  ulimit -s 8192
  ret_val=$?
  print_htx_log "Setting ulimit -s(stack size) to 8MB" $ret_val
  
# Set core file limit to be unlimited.
  ulimit -c unlimited
  ret_val=$?  
  print_htx_log "Setting ulimit -c(core limit) to 'unlimited'" $ret_val

# Reset virtual mem ulimit, to unlimited.
  ulimit -v unlimited
  ret_val=$?
  print_htx_log "Setting ulimit -v(virtual memory)  to 'unlimited'" $ret_val

# Increase open files limit.
  num_proc=`cat /proc/cpuinfo  | grep -i processor | wc -l`
  open_files_limit=`expr $num_proc \* 4`
  if [[ $num_proc -lt 1024 ]]
  then
      open_files_limit=1024
  fi

  ulimit -n $open_files_limit
  ret_val=$?
  print_htx_log "Setting ulimit -n(# open files) to '$open_files_limit'" $ret_val

# Creating a link for /bin/ksh
  if [ ! -f /bin/ksh ]
  then
    if [ -f /usr/bin/ksh ]
      then
        ln -s /usr/bin/ksh /bin/ksh
      else
        ln -s /bin/bash /bin/ksh
      fi
  fi

# Set core files to use pid.
  echo 1 >/proc/sys/kernel/core_uses_pid
  

# Source global definitions
  [ -f /etc/bashrc ] && . /etc/bashrc

# Write to /proc/sys/kernel/sem to allow more semop operations.This will overwrite default values
# [ -f /proc/sys/kernel/sem ] && echo "550 64000 500 1024" >/proc/sys/kernel/sem
# [ -f /proc/sys/kernel/sem ] && echo "250 32000 32 1024" >/proc/sys/kernel/sem

#sleep 5

# Execute HTX setup scripts 
  [ -f ${HTXSCRIPTS}/htxtmpwatch ] && cp ${HTXSCRIPTS}/htxtmpwatch /etc/cron.d/htxtmpwatch 2>/dev/null


  if [[ "$HTXD_STARTUP_MODE" = "ON" ]] ; then
    print_htx_log "HTXD_STARTUP_MODE was '$HTXD_STARTUP_MODE'. Turning it OFF and returning."
    HTXD_STARTUP_MODE=OFF
    return
  fi

  # Work-around for autostartup problems
  if [ -f ${HTXLPP}.autostart ] && [ ! -f ${HTXLPP}.no_bootme_restart_delay ] ; then
    htx_print_log "sleeping 120 seconds for bootme with multipath"
    sleep 120
  fi

  # To execute post boot commands for bootme
	if [ -f ${HTXLPP}.autostart ] ; then
		BOOT_POST=$(grep "^BOOT_POST:" ${HTXREGRULES}bootme/default | head -n1 | awk '{ print $2 }')
		if [ -f ${BOOT_POST} ];
		then
			print_htx_log "Calling ${BOOT_POST}"
			. ${BOOT_POST}
		fi
	fi


	if [ `ls ${HTXBIN}/hxenvidia_* 2>/dev/null | wc -l` != 0 ] ; then
		if [ `grep "8.0" /usr/local/cuda/version.txt 2>/dev/null | wc -l` == 1 ] ; then	
			ln -sf ${HTXBIN}/hxenvidia_8_0 ${HTXBIN}/hxenvidia
			print_htx_log "making hxenvidia_8_0 as default."
		elif [ `grep "7.5" /usr/local/cuda/version.txt 2>/dev/null | wc -l` == 1 ] ; then
			ln -sf ${HTXBIN}/hxenvidia_7_5 ${HTXBIN}/hxenvidia
			print_htx_log "making hxenvidia_7_5 as default."
		fi
 	fi 
	   
  
  print_htx_log "Collecting LPAR configuration details ..."
  ${HTXBIN}/show_syscfg > ${HTX_LOG_DIR}/htx_syscfg 2>/dev/null
  
  for file in `/bin/ls $HTXSETUP[a-zA-Z]*.setup 2>/dev/null| sort | grep -v mem.setup | grep -v "htx\.setup"`
  do
    print_htx_log "Running $file"
    . $file  | tee -a $HTXNoise
  done

  # Now run mem.setup
  print_htx_log "Running mem.setup"
  . ${HTXSETUP}/mem.setup | tee -a $HTXNoise

  print_htx_log "Calling devconfig"
  ${HTXSCRIPTS}/devconfig | tee -a $HTXNoise
  
  [ ! -f ${HTXMDT}/mdt ] && cp ${HTXMDT}/mdt.all ${HTXMDT}/mdt
  
# Work-around for autostartup problems
  if [ -f ${HTXLPP}.autostart ];
  then
     print_htx_log "Autostart is ON. Sleeping for 20 seconds and then calling runsup."
     sleep 20
     ${HTXSCRIPTS}runsup
  fi

# Export ZLIB variables to start testing corsa hardware.
  if [ -f /dev/genwqe0_card ] # PCIe Corsa
  then
    print_htx_log "exporting ZLIB_DEFLATE_IMPL=1, ZLIB_INFLATE_IMPL=1, ZLIB_CARD=0, ZLIB_INFLATE_THRESHOLD=0"
    export ZLIB_DEFLATE_IMPL=1; export ZLIB_INFLATE_IMPL=1; export ZLIB_CARD=0; export ZLIB_INFLATE_THRESHOLD=0;
  elif [ -f /dev/cxl/afu0.0s ] # CAPI card of some form.
  then
    print_htx_log "Exporting ZLIB_ACCELERATOR=CAPI, ZLIB_CARD=-1"
    export ZLIB_ACCELERATOR=CAPI; export ZLIB_CARD=-1;
  fi

# Below logic is to increase cgroup max thread limit.
# If not done, this will cause failure of multiple exercisers
# as the default is 512 only starting ubuntu 16.04. 

print_htx_log "setting proper systemd resource limits..."
A=`echo /sys/fs/cgroup/pids$(awk -F : '/pids/{print $NF}' /proc/$$/cgroup) 2> /dev/null` 
B=$A

# Set pids.max for all levels below us.
while :; do
	test -f $B/pids.max || break
	echo max > $B/pids.max
	if [ $B ==  "/sys/fs/cgroup/pids" ]; then
		break;
	fi
	B=$(dirname $B)
done

# Set pids.max for all levels above us.

for i in $(find $A -name "pids.max" 2> /dev/null); do
	test -f $i || break
	echo max > $i
done
