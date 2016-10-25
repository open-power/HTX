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
#
#
################################################################
# htx.d script is to enable htxd service
#
# System startup script for htx deamon
#
### BEGIN INIT INFO
# Provides: htxd
# Required-Start:
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start htx daemon
# Description:       Start htx daemon
### END INIT INFO
#
# chkconfig: 2345 20 80
# description: Runs htxd startup script htxd_run 
# processname: htxd 
#


prog=htxd

HTX_HOME_DIR=`cat /var/log/htx_install_path`

start() 
{
	return_code=$(rh_status)
	if [[ ${return_code} = 0 ]] ; then
        	echo "htx daemon has been already running..." 
		exit 1 
	fi
	echo "Starting $prog: ..."
	${HTX_HOME_DIR}/etc/scripts/htxd_run > /tmp/htxd_starup_log 2>&1	
}

stop() 
{
	return_code=$(rh_status)
	if [[ ${return_code} != 0 ]] ; then
		echo "htx daemon was already stopped..."
		exit 0
	fi
	echo "Stopping $prog: ..."
	${HTX_HOME_DIR}/etc/scripts/htxd_shutdown > /tmp/htxd_starup_log 2>&1
}

restart() 
{
	stop
	start
}

reload() 
{
	restart
}

rh_status() 
{
    # run checks to determine if the service is running or use generic status
	pgrep htxd > /dev/null 2>&1
	return_code=$?
	echo ${return_code}
}

rh_status_q() 
{
    rh_status >/dev/null 2>&1
}


### program starts here ###

case "$1" in
    start)
        $1
        ;;
    stop)
        $1
        ;;
    restart)
        $1
        ;;
    reload)
        $1
        ;;
    status)
        return_code=$(rh_status)
	if [[ ${return_code} = 0 ]] ; then
		echo "htx daemon is running"
	else
		echo "htx daemon is stopped"
	fi
        ;;
    *)
        echo $"Usage: $0 {start|stop|status|restart|reload}"
        exit 2
esac

exit 0
