#!/bin/bash

INSTALL_LOG_FILE=/var/log/htx_install_log

if [ "$(id -u)" != "0" ]; then
   echo -e "\nThis script must be run as root..exiting."
   exit 1;
fi

>${INSTALL_LOG_FILE}

usage()
{
        echo -e "Usage: ./installer.sh\n"
        echo -e "./installer.sh -p <installation_path> [-f]\n"
        echo -e "\t-f Force installation"
        echo -e "\n\tEg. ./installer.sh -p /home/"
        echo -e "\n\tEg. ./installer.sh -p /home/ -f"
}

while getopts ":p:f" opt
do
        case $opt in
                p)
                        INSTALL_PATH=$OPTARG;
                        FORCE=0;
                        LOCATION=1;
                ;;
                f)
                        FORCE=1;
                ;;
                \?)
                        echo -e "Invalid option: -$OPTARG"
                        usage
                        exit 1;
                ;;
                :)
                        echo -e "Option -$OPTARG requires an argument"
                        usage
                        exit 1;
                ;;
        esac
done

if [ "$LOCATION" == "" ];
then
        INSTALL_PATH=/usr/lpp;
fi

if [ "$INSTALL_PATH" == "" ];
then
        echo -e "Option: -p is missing"
        usage
        exit 1;
fi

mkdir -p /var/log >/dev/null 2>/dev/null
echo "HTX intall started on `date` >>>>>>>>>>>>" >> ${INSTALL_LOG_FILE}
if type "lsb_release" >> ${INSTALL_LOG_FILE} 2>&1; then
        echo -e "\nYour Install System:"
        if [ -f /etc/redhat-release ] ; then
                OS="RedHat"
        elif [ -f /etc/SuSE-release ] ; then
                OS="SUSE"
        elif [ -f /etc/debian_version ] ; then
                OS="Ubuntu"
        fi        
        ARCH=$(uname -m | sed 's/x86_//;s/i[3-6]86/32/')
        VER=$(lsb_release -sr)
        echo -e "Distro:$OS"
        echo -e "Version:$VER"
        echo -e "Arch:$ARCH"
else
        echo -e "\nYour Install System:"
        if [ -f /etc/redhat-release ] ; then
                OS="RedHat"
        elif [ -f /etc/SuSE-release ] ; then
                OS="SUSE"
        elif [ -f /etc/debian_version ] ; then
                OS="Ubuntu"
        fi
        ARCH=$(uname -m)
        echo -e "Distro:$OS"
        echo -e "Arch:$ARCH"
fi

CURRENT_OS="Compiledon:$OS"
if [ $CURRENT_OS != $(cat os_build | grep Compiledon) ];
then
        echo -e "\nWarning:It is observed that HTX is $(cat os_build | grep Compiledon) and you are trying to install on top of $OS..." | tee  -a ${INSTALL_LOG_FILE}
fi
if [ -f /var/log/htx_install_path ]; then
HTX_INSTALL=$(cat /var/log/htx_install_path)
echo -e "\nHTX already installed in: $HTX_INSTALL" | tee -a ${INSTALL_LOG_FILE}
if [ "$FORCE" == "" ];
then
        echo -e "Exiting..."
        exit 0;
fi
if [ $FORCE -eq 1 ];
then
echo -ne "\nRemoving installed HTX..." | tee -a ${INSTALL_LOG_FILE}
./uninstaller.sh >> ${INSTALL_LOG_FILE}
if [ $? -eq 0 ]; then
echo -e "[OK]" | tee -a ${INSTALL_LOG_FILE}
else
echo -e "[FAILED]" | tee -a ${INSTALL_LOG_FILE}
echo -e "\n Uninstall failed..Please uninstall manually and restart installation.  Refer '${INSTALL_LOG_FILE}' file for logs" | tee -a ${INSTALL_LOG_FILE}
exit 1;
fi
else
exit 0;
fi
fi
if [ "$LOCATION" == "" ];
then
        echo -e "\nInstalling HTX in default path:/usr/lpp/htx..." | tee -a ${INSTALL_LOG_FILE}
else
        echo -e "\nInstalling HTX in $INSTALL_PATH/htx..." | tee -a ${INSTALL_LOG_FILE}
fi
echo -ne "\nCreating 'htx' user..." | tee -a ${INSTALL_LOG_FILE}
if ! id -g htx > /dev/null 2>&1 ; then groupadd htx -o -g 0 > /dev/null 2>&1 ; fi
if [ $? -ne 0 ]; then
groupdel htx > /dev/null 2>&1 ;
groupadd htx -o -g 0 > /dev/null 2>&1 ;
fi
sync
sleep 1
if ! id htx > /dev/null 2>&1 ; then mkdir -p $INSTALL_PATH/htx; useradd -g htx -d $INSTALL_PATH/htx -s /bin/bash -u 0 -o htx > /dev/null 2>&1 ; fi
rm -rf $INSTALL_PATH/htx
echo -e "[OK]" | tee -a ${INSTALL_LOG_FILE}
mkdir -p /var/log
mkdir -p $INSTALL_PATH
echo -ne "\nExtracting htx.tar.gz to $INSTALL_PATH..." | tee -a ${INSTALL_LOG_FILE}
tar xvzf htx.tar.gz -C $INSTALL_PATH > /dev/null 2>&1
echo -e "[OK]" | tee -a ${INSTALL_LOG_FILE}
INSTALL_PATH_HTX=$INSTALL_PATH/htx
echo $INSTALL_PATH_HTX > /var/log/htx_install_path
echo -ne "\nFinishing Installation..." | tee -a ${INSTALL_LOG_FILE}
[[ -f $INSTALL_PATH/htx/etc/scripts/htx.d ]] && cp -f $INSTALL_PATH/htx/etc/scripts/htx.d /etc/init.d
echo -e "[OK]" | tee -a ${INSTALL_LOG_FILE}
echo -ne "\nCopying HTX Binaries..." | tee -a ${INSTALL_LOG_FILE}
[[ -f /bin/perl ]] && ln -sf /usr/bin/perl /bin/perl >> ${INSTALL_LOG_FILE} 2>&1
if [ "$OS" != "RedHat" ];
then 
	ln -sf /usr/bin/awk /bin/awk >> ${INSTALL_LOG_FILE} 2>&1
fi
ln -sf $INSTALL_PATH/htx/bin/htxcmdline /bin/htxcmdline
ln -sf $INSTALL_PATH/htx/bin/htxcmdline /bin/hcl
ln -sf $INSTALL_PATH/htx/bin/htxscreen /bin/htxscreen
ln -sf $INSTALL_PATH/htx/bin/htxscreen /bin/htxmenu
ln -sf $INSTALL_PATH/htx/etc/scripts/ver /bin/ver
cp $INSTALL_PATH/htx/htx_binaries/bin/* /usr/bin/
cp $INSTALL_PATH/htx/htx_binaries/sbin/* /usr/sbin/
echo -e "[OK]" | tee -a ${INSTALL_LOG_FILE}

if [ "$OS" != "Ubuntu" ];
then
        echo -ne "\nCreating htx.d.service file..." | tee -a ${INSTALL_LOG_FILE}
        echo "[Unit]" > /usr/lib/systemd/system/htx.d.service
        echo -e "Description=htx Daemon\n" >>  /usr/lib/systemd/system/htx.d.service
        echo "[Service]" >> /usr/lib/systemd/system/htx.d.service
        echo "Type=forking" >> /usr/lib/systemd/system/htx.d.service
        echo -e "ExecStart=${INSTALL_PATH}/htx/etc/scripts/htxd_run" >> /usr/lib/systemd/system/htx.d.service
        echo -e "ExecStop=${INSTALL_PATH}/htx/etc/scripts/htxd_shutdown" >> /usr/lib/systemd/system/htx.d.service
        echo -e "TasksMax=infinity\n" >> /usr/lib/systemd/system/htx.d.service
        echo "[Install]" >> /usr/lib/systemd/system/htx.d.service
        echo "WantedBy=multi-user.target" >> /usr/lib/systemd/system/htx.d.service
        echo -e " [OK]" | tee -a ${INSTALL_LOG_FILE}
fi
echo -ne "\nAdding HTX Daemon(htxd) as service..." | tee -a ${INSTALL_LOG_FILE}
nimesis_present=`ps -ef | grep nimesis | grep -v grep | wc -l`
if [ $nimesis_present -eq 0 ] && [ ! "$DONTSTARTHTXD" = 1 ]
then
        if [ "$OS" == "Ubuntu" ];
        then
                update-rc.d htx.d defaults >> ${INSTALL_LOG_FILE} 2>&1
                /etc/init.d/htx.d start >> ${INSTALL_LOG_FILE} 2>&1
        else
                systemctl enable htx.d >> ${INSTALL_LOG_FILE} 2>&1
                systemctl start htx.d >> ${INSTALL_LOG_FILE} 2>&1
                systemctl status htx.d >> ${INSTALL_LOG_FILE} 2>&1
        fi
fi
echo -e "[OK]" | tee -a ${INSTALL_LOG_FILE}
echo -ne "\nChecking HTX daemon status..." | tee -a ${INSTALL_LOG_FILE}
if pgrep -x "htxd" >> ${INSTALL_LOG_FILE} 2>&1
then
    echo -e "[Running]" | tee -a ${INSTALL_LOG_FILE}
    echo -e "\nHTX install completed..Now exiting." | tee -a ${INSTALL_LOG_FILE}
    echo -e "HTX install completed on `date` <<<<<<<<<<<<" >> ${INSTALL_LOG_FILE} 2>&1
    echo -e "\nHTX Installed version\n"  >> ${INSTALL_LOG_FILE} 2>&1	
    ver >> ${INSTALL_LOG_FILE} 2>&1
    exit 0;
else
    echo -e "[Not Running]" | tee -a ${INSTALL_LOG_FILE}
    echo -e "\nHTX Install failed..Please check log file '${INSTALL_LOG_FILE}'."
    exit 1;
fi

