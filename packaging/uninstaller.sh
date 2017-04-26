#!/bin/bash

UNINSTALL_LOG_FILE=/var/log/htx_uninstall_log

if [ "$(id -u)" != "0" ]; then
   echo -e "\nThis script must be run as root..exiting."
   exit 1;
fi

>${UNINSTALL_LOG_FILE}

echo "HTX uninstall script started on `date` >>>>>>>>>>>>" >> ${UNINSTALL_LOG_FILE}
if [ -f /etc/redhat-release ] ; then
        OS="RedHat"
elif [ -f /etc/SuSE-release ] ; then
        OS="SUSE"
elif [ -f /etc/debian_version ] ; then
        OS="Ubuntu"
    fi
if [ ! -f /var/log/htx_install_path ]; then
echo -e "\nHTX is not installed, Exiting..." | tee -a ${UNINSTALL_LOG_FILE}
exit 0;
fi
teststatus=$(hcl -query) > /dev/null 2>&1
teststatus_exit=$(echo $?)
echo -ne "\nChecking HTX daemon Status..." | tee -a ${UNINSTALL_LOG_FILE}
if pgrep -x "htxd" > /dev/null
then
    echo -e "[Running]" | tee -a ${UNINSTALL_LOG_FILE}
    echo -ne "\nChecking HTX tests Status..." | tee -a ${UNINSTALL_LOG_FILE}
    if [ $teststatus_exit -eq 0 ]; then
        echo -e "[Activated]" | tee -a ${UNINSTALL_LOG_FILE}
        echo -e "\nIt is found that HTX tests are currently running, please manually stop them and then rerun the script.. Exiting." | tee -a ${UNINSTALL_LOG_FILE}
        exit 1;
    else
        echo -e "[Not Activated]" | tee -a ${UNINSTALL_LOG_FILE}
        if [ "$OS" == "Ubuntu" ]; then
                /etc/init.d/htx.d stop >> ${UNINSTALL_LOG_FILE} 2>&1
        else
                systemctl stop htx.d >> ${UNINSTALL_LOG_FILE} 2>&1
        fi
        sleep 3
        if pgrep -x "htxd" >> ${UNINSTALL_LOG_FILE} 2>&1
        then
            echo -e "\nProblem stopping htx..Exiting" | tee -a ${UNINSTALL_LOG_FILE}
            exit 1;
        else
            echo -e "\nHTX daemon Stopped Successfully..." | tee -a ${UNINSTALL_LOG_FILE}
        fi
    fi
else
    echo -e "[Not Running]" | tee -a ${UNINSTALL_LOG_FILE}
fi

echo -ne "\nRemoving HTX Daemon(htxd) as service..." | tee -a ${UNINSTALL_LOG_FILE}
if [ "$OS" == "Ubuntu" ];
then
        update-rc.d -f htx.d remove >> ${UNINSTALL_LOG_FILE} 2>&1
else
        systemctl disable htx.d >> ${UNINSTALL_LOG_FILE} 2>&1
fi
echo -e "[OK]" | tee -a ${UNINSTALL_LOG_FILE}
echo -ne "\nRemoving binaries..." | tee -a ${UNINSTALL_LOG_FILE}
[[ -h /bin/ver ]] && rm -rf /bin/ver 2>/dev/null
[[ -h /bin/htxcmdline ]] && rm -rf /bin/htxcmdline 2>/dev/null
[[ -h /bin/hcl ]] && rm -rf /bin/hcl 2>/dev/null
[[ -h /bin/htxscreen ]] && rm -rf /bin/htxscreen 2>/dev/null
[[ -h /bin/htxmenu ]] && rm -rf /bin/htxmenu 2>/dev/null
[[ -h /usr/bin/ver ]] && rm -rf /usr/bin/ver 2>/dev/null
[[ -h /usr/bin/htxcmdline ]] && rm -rf /usr/bin/htxcmdline 2>/dev/null
[[ -h /usr/bin/hcl ]] && rm -rf /usr/bin/hcl 2>/dev/null
[[ -h /usr/bin/htxscreen ]] && rm -rf /usr/bin/htxscreen 2>/dev/null
[[ -h /usr/bin/htxmenu ]] && rm -rf /usr/bin/htxmenu 2>/dev/null
echo -e "[OK]" | tee -a ${UNINSTALL_LOG_FILE}
htx_path=$(cat /var/log/htx_install_path)
echo -ne "\nRemoving HTX from installed path: $htx_path..." | tee -a ${UNINSTALL_LOG_FILE}

[[ -d $htx_path ]] &&  rm -rf $htx_path 2>/dev/null
[[ -s /bin/set_linux_nets ]] && rm -rf /bin/set_linux_nets 2>/dev/null
[[ -s /etc/cpdir ]] && rm -rf /etc/cpdir 2>/dev/null
[[ -s /etc/gethelp ]] && rm -rf /etc/gethelp 2>/dev/null
[[ -s /etc/gethtx ]] && rm -rf /etc/gethtx 2>/dev/null
[[ -s /etc/getnet ]] && rm -rf /etc/getnet 2>/dev/null
[[ -s /etc/gettools ]] && rm -rf /etc/gettools 2>/dev/null
[[ -s /etc/tping ]] && rm -rf /etc/tping 2>/dev/null
echo -e "[OK]" | tee -a ${UNINSTALL_LOG_FILE}
echo -ne "\nRemoving htx daemon..." | tee -a ${UNINSTALL_LOG_FILE}
rm -rf /etc/init.d/htx.d 2>/dev/null
echo -e  "[OK]" | tee -a ${UNINSTALL_LOG_FILE}

echo -ne "\nDeleting user 'htx'..." | tee -a ${UNINSTALL_LOG_FILE}
if id htx > /dev/null 2>&1 ; then userdel -f htx 2>/dev/null; fi
if id -g htx > /dev/null 2>&1 ; then groupmod -g 999 htx 2>/dev/null; fi
if id -g htx > /dev/null 2>&1 ; then groupdel htx 2>/dev/null; fi
echo -e  "[OK]" | tee -a ${UNINSTALL_LOG_FILE}
echo -e "\nDeleting file /var/log/htx_install_path..." | tee -a ${UNINSTALL_LOG_FILE}
[[ -f /var/log/htx_install_path ]] && rm -f /var/log/htx_install_path > /dev/null 2>&1
echo -e "\nUninstall finished... Exiting." | tee -a ${UNINSTALL_LOG_FILE}
echo "HTX uninstall finished on `date` >>>>>>>>>>>>" >> ${UNINSTALL_LOG_FILE}
exit 0;

