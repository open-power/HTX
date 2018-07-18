#!/bin/bash

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

# Functions to print message in proper format

# This function prints message to both terminal and HTXNoise
# Takes 1 or 2 arguments:
#	1 argument : Just the string to be printed with date.
#	2 arguments: 1st arg is the string. 2nd arg is return code.
print_htx_log()
{
	if [[ -n "$1" && -n "$2" ]]
	then
		if [[ $2 -eq 0 ]]
		then
			echo "[`date`]: $1. return=$2" | tee -a $HTXNoise
		else
			echo "[`date`]: [ERROR($2)] $1 failed. return=$2" | tee -a $HTXNoise
		fi
	else
		echo "[`date`]: $1" | tee -a $HTXNoise
	fi
}


# Same as above function but printf to HTXNoise _only_
print_htx_log_only()
{
	if [[ -n "$1" && -n "$2" ]]
	then
		if [[ $2 -eq 0 ]]
		then
			echo "[`date`]: $1. return=$2" >> $HTXNoise
		else
			echo "[`date`]: [ERROR($2)] $1 failed. return=$2" >> $HTXNoise
		fi
	else
		echo "[`date`]: $1" >> $HTXNoise
	fi
}

# function to print msg. in HTXNoise while any scripts starts executing
print_entering_script()
{
    echo "======================" >> $HTXNoise
    echo "Entering $1           " >> $HTXNoise
    echo "======================" >> $HTXNoise
}

# function to print msg. in HTXNoise while exiting any script
print_exiting_script()
{
    echo "======================" >> $HTXNoise
    echo "Exiting $1            " >> $HTXNoise
    echo "======================" >> $HTXNoise
}

# Export variables
  export USERNAME=htx
  export HOST=$(/bin/hostname -s)
  export PS1='
($?) $USERNAME @ $HOST: $PWD
# '

  
  export TERM=vt100
  
#export all HTX env. variables
  HTX_HOME_DIR=`cat /var/log/htx_install_path`
  export HTX_HOME_DIR=${HTX_HOME_DIR}/
  export HOME=$HTX_HOME_DIR
  export HTX_LOG_DIR=/tmp/
  export LESS="-CdeiM"

  export HTXLPP=$HTX_HOME_DIR
  export HTXPATH=$HTX_HOME_DIR
  export HTXLINUXLEVEL=${HTXLPP}/htxlinuxlevel # Its a file. Last / should not be there.
  export HTXPATTERNS=${HTXLPP}/pattern/
  export HTXCFGPATH=${HTXLPP}/equaliser_cfgs/
  export HTXRULES=${HTXLPP}/rules/
  export HTXREGRULES=${HTXRULES}/reg/
  export HTXEMCRULES=${HTXRULES}/emc/
  export HTXECG=${HTXLPP}/ecg/
  export HTXMDT=${HTXLPP}/mdt/
  export HTXBIN=${HTXLPP}/bin/
  export HTXETC=${HTXLPP}/etc/
  export HTXSCREENS=${HTXLPP}/etc/screens/
  export STXSCREENS=${HTXLPP}/etc/screens_stx/
  export HTXMISC=${HTXLPP}/etc/misc/
  export HTXTMP=$HTX_LOG_DIR
  export HTXSETUP=${HTXLPP}/setup/
  export HTXRUNSETUP=${HTXLPP}/runsetup/
  export HTXRUNCLEANUP=${HTXLPP}/runcleanup/
  export HTXCLEANUP=${HTXLPP}/cleanup/
  export HTXPROCESSORS=1
  export HTXSCRIPTS=${HTXLPP}/etc/scripts/
  export HTXSCRIPTS_STX=${HTXLPP}/etc/scripts_stx/
  export HTX_BOOTME_LOG_DIR=${HTX_LOG_DIR}/htx_bootme_log_dir
  export HTXKERNEXT=${HTXLPP}/etc/kernext/
  export HTXOWNERS=${HTXLPP}/exerowners/
  export HTXINFO=${HTXLPP}/exerinfo/
  export HTXFORMATS=${HTXLPP}/etc/formats/
  export HTXNoise=$HTX_LOG_DIR/HTXScreenOutput # Its a file. Last / should not be there
  export PATH=$PATH:$HTX_HOME_DIR:$HTXSCRIPTS

  export gpu_mem_numa_id_map=${HTX_HOME_DIR}/config/gpu_mem_numa_id_map
 

  #export PATH variable
  PATH=$PATH:$HOME
  PATH=$PATH:$HOME/etc/scripts
  PATH=$PATH:$HOME/etc
  PATH=$PATH:$HOME/etc/methods
  PATH=$PATH:$HOME/bin
  PATH=$PATH:/bin
  PATH=$PATH:/sbin
  PATH=$PATH:/usr/bin
  PATH=$PATH:/usr/sbin
  PATH=$PATH:/usr/ucb
  PATH=$PATH:/usr/bin/X11
  PATH=$PATH:/etc
  PATH=$PATH:/test/tools
  PATH=$PATH:/usr/local/bin
  PATH=$PATH:$HOME/test/tools
  PATH=$PATH:.
  export PATH

# Directory commands
  alias ksh="bash"
  alias mdt="cd $HTXMDT"
  alias reg="cd $HTXREGRULES"
  alias emc="cd $HTXEMCRULES"
  alias rules="cd $HTXRULES"
  alias bin="cd $HTXBIN"
  alias tvi=/bos/k/bin/vi
  alias em="emacs =100x38+0+0"
  alias e="xe"
  alias li="/bin/li -v"
  alias liv="/bin/li -v vmm*"
  alias ll="/bin/li -lv"
  alias llv="/bin/li -lv vmm*"
  alias llx="/bin/li -lv xix*"

# Screen management commands
  alias cls="tput clear"
  alias bye="tput clear; exit"
  alias win="open ksh; tput clear"

# System management commands
  alias kmake=/bos/k/bin/make
  alias lml="lm list=list"
  alias print=/bin/print
  alias pq="/bin/print -q"
  alias pspg="ps -ef | pg"
  alias rb="remsh bdslab"
  alias pnum="rexec bcroom pnum"
  alias cnum="rexec bcroom cnum"
  alias dept="rexec bcroom dept"
  alias man="man -e/bin/pg"
  alias manv3="manv3 -e/bin/pg"
  alias nmake="nmake -u"
  alias s="echo 'sync;sync;sync;sync';sync;sync;sync;sync"

# Miscellaneous commands
  alias de="daemon emacs"
  alias dx="daemon xant"

# HTX commands
  set -o ignoreeof
  set -o vi

