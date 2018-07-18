#!/usr/bin/perl

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

$linux_dist = "Unknown Linux distribution";
$htx_ver = "Unknown HTX Level";
$kern_ver = "Unknown kernel version";

$kern_ver = `uname -r 2> /dev/null`;
$linux_dist = `grep ^NAME /etc/os-release | cut -f2 -d= | sed 's/\"//g'`;
$os_ver=`grep ^VERSION_ID /etc/os-release | cut -f2 -d= | sed 's/\"//g'`;

$CMVC_RELEASE = `echo \$CMVC_RELEASE`;
if ( $CMVC_RELEASE =~ /htxltsbml/ ) {
	@arr_htx_ver = `cat \$HTXETC/version`;
}
if ( $CMVC_RELEASE =~ /htxubuntu/ ) {
        @arr_htx_ver = `dpkg-query --show "htx*"`;
}
else {
	@arr_htx_ver = `(rpm -qa | grep ^htx) 2> /dev/null | grep htx |cut -d- -f1,2`;
}
$htx_ver=@arr_htx_ver[$#arr_htx_ver];

if( $linux_dist =~ /Fedora Core release 4.9/ )                   { $linux_dist = "FC5";    }
if( $linux_dist =~ /Fedora Core release 6/ )                     { $linux_dist = "FC6";    }
if( $linux_dist =~ /SUSE LINUX Enterprise Server 9/ )            { $linux_dist = "SLES9";  }
if( $linux_dist =~ /SUSE Linux Enterprise Server 10/ )           { $linux_dist = "SLES10"; }
if( $linux_dist =~ /SUSE Linux Enterprise Server 11/ )           { $linux_dist = "SLES11"; }
if( $linux_dist =~ /Red Hat Enterprise Linux AS release 4/ )     { $linux_dist = "RHEL4";  }
if( $linux_dist =~ /Red Hat Enterprise Linux Server release 5/ ) { $linux_dist = "RHEL5";  }
if( $linux_dist =~ /Red Hat Enterprise Linux Server release 6/)  { $linux_dist = "RHEL6";  }

chomp ($kern_ver);
chomp ($htx_ver);
chomp ($linux_dist);
chomp ($os_ver);

$out = sprintf(" HTX:%s OS:%s %s Kernel: %s", $htx_ver, $linux_dist, $os_ver, $kern_ver);
print $out;
