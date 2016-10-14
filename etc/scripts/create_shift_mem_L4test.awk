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

# Get HTX_LOG_DIR export variable
LOG_DIR = snarf("echo $HTX_LOG_DIR");

# Determine no. of chips on the system
cmd = sprintf("cat %s/htx_syscfg | grep \"Number of chips\" | awk -F: '{print $2}'", LOG_DIR);
no_of_chips = snarf(cmd);
#printf("No. of chips: %d\n", no_of_chips);
rule_file = sprintf("%s", ARGV[1]);

#Below command will create default stanza
system("cat ${HTXMDT}mdt.all | create_mdt_with_devices.awk");

for (i=0; i < no_of_chips; i++) {
    mem_inst = sprintf("mem%d", i);
    mkstanza("hxemem64","64bit","memory",mem_inst,"hxemem64",rule_file,rule_file);
    cont_on_err("NO");
    printf("\n");
}
}

function string_stanza(a,b,c) {
    len=(length(a) + length(b) + 8 + 3 + 2) - 40;
    printf("\t%s = \"%s\" %" len "s * %s\n",a,b,"",c);
}

function HE_name(x) {
    string_stanza("HE_name",x,"Hardware Exerciser name, 14 char");
}

function adapt_desc(x) {
    gsub(" ","_",x);
    string_stanza("adapt_desc",x,"adapter description, 11 char max.");
}

function device_desc(x) {
    gsub(" ","_",x);
    string_stanza("device_desc",x,"device description, 15 char max.");
}

function cont_on_err(x) {
    string_stanza("cont_on_err",x,"continue on error (YES/NO)");
}

function mkstanza(hxe,a,d,dev,rfdir,reg,emc) {
    printf("%s:\n",dev);
    if (hxe) HE_name(hxe);
    if (a) adapt_desc(a);
    if (d) device_desc(d);
    if(reg) {
        string_stanza("reg_rules",sprintf("%s/%s",rfdir,reg),"reg");
    }
    if(emc) {
        string_stanza("emc_rules",sprintf("%s/%s",rfdir,emc),"emc");
    }
}

function snarf(cmd) {
    snarf_input="";
    cmd | getline snarf_input; close(cmd); return snarf_input;
}
