# HTX README

The Hardware Test Executive (HTX) is a suite of test tools for hardware
validation of OpenPOWER system.

The HTX stress tests the system by exercising all or selected hardware
components concurrently to uncover any hardware design flaws and
hardware-hardware or hardware-software interaction issues.

The HTX consists of test programs (a.k.a exercisers) to test processor, memory
and IO subsystem of POWER system.

The HTX runs on PowerKVM guests (Linux distributions like Ubuntu, SuSE and
Redhat), and OPAL-based distributions (like Ubuntu) and on Linux host machine.
HTX supports two types of control interfaces to exercisers, user interactive
and command line.

## Documentation
Please refer `Documentation` dir.

## Building HTX from source tree

Download the source from github.

Edit htx.mk file to point `TOPDIR` to downloaded HTX source.

Also set `HTX_RELEASE` variable to appropriate distro for which you are
building HTX source for. Possible values are:

 * htxubuntu
 * htxrhel72le
 * htxrhel7
 * htxsles12,
 * htxfedora
 * htxfedorale

Build instructions:  
  
In Debian/Ubuntu  
  1. install git to download source code  
apt-get install git  
  2. download HTX source code  
git clone https://www.github.com/open-power/HTX  
  3. install other packages needed to compile HTX  
apt-get install gcc make libncurses5 g++ libdapl-dev libcxl1 
    3.1 for Ubuntu 14.04 there is no libdapl-dev package, so have to download and compile dapl separately from https://www.openfabrics.org/downloads/dapl/  
    3.2 install following packages needed to compile dapl  
      apt-get install libibverbs-dev librdmacm-dev  
    3.3 cd into dapl directory and "./configure" "make" and then "make install"  
  4. For libcxl dependency, try following:
    4.1 apt-get install libcxl1.
    4.2 If unable to install, download libcxl source from "https://github.com/ibm-capi/libcxl" 
	compile and place libcxl1.so under /usr/lib (for ubuntu) or under /lib64/power8/
	(for RHEL7 LE)
  5. back in HTX directory do a "make all" to compile
    5.1 After compiling to create debian package do "make deb" which will create a htxubuntu.deb package in top level dir. 
    5.2 To create HTX tarball with installer, do "make tar". HTX tarball by name "htx_package.tar.gz" will be created in top dir. 

Note that HTX assumes you are building on a PowerPC distribution.  

Other targets:

 * To clean: make clean

## Install
Please refer HTX Users manual at `Documentation/HTX_user_manual.txt`
for details.

## License
Apache License, Version 2.0. For full details see LICENSE.

## Contributing
If you want to add your own test program as part of HTX. Please refer HTX
Programmer's manual at `Documentation/HTX_developers_manual.txt`.
