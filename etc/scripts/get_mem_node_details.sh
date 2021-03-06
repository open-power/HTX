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
no_of_instances=`ls -d /sys/devices/system/node/node[0-9]* 2>/dev/null | sort -n -t : -k 1.30 | wc -l`
for node in $(ls -d /sys/devices/system/node/node[0-9]* 2>/dev/null | sort -n -t : -k 1.30)
do
	if [ -d $node ]; then
		cpulist=" "
		cpulist=`cat $node/cpulist`
		if [ -z "$cpulist" ];
		then
			no_of_instances=`expr $no_of_instances - 1`
		fi
	fi
done

echo "num_nodes=$no_of_instances" 

for node in $(ls -d /sys/devices/system/node/node[0-9]* 2>/dev/null | sort -n -t : -k 1.30)
do
	#printf "node=$node\n"
	if [ -d $node ]; then
		node_num=${node##*/node/node}
		cpulist=" "
		cpulist=`cat $node/cpulist`
#               printf "$cpulist,"
        	if [ -n "$cpulist" ];
        	then
            		i=0
            		k=0
            		cpus_in_node=0
            		for cpus in $(cat $node/cpulist | awk -F"," '{for(i=1;i<=NF;i++) print $i }')
            		do
            			#echo "cpu:$cpus"
            			j=0
            			for cpu_range in $(echo $cpus | awk -F"-" '{for(i=1;i<=NF;i++) print $i}')
            			do
            				#echo "cpus:$cpu_range"
            				cpu_no[$j]=$cpu_range
            				#echo "cpu_ran[$j]:${cpu_ran[$j]}"
            				j=`expr $j + 1`
            			done
            			if [ $j == 1 ];
            			then
            				no_cpus=$j
            			else
            				no_cpus=`expr ${cpu_no[1]} - ${cpu_no[0]} + 1`
            			fi
            			
            			for ((a=0; a < $no_cpus; a++))
            			do
            				lcpu[$k]=${cpu_no[0]}
            				cpu_no[0]=`expr ${cpu_no[0]} + 1`
            				#echo "lcpu:${lcpu[$k]}"
            				k=`expr $k + 1`
					cpus_in_node=`expr $cpus_in_node + 1`
            			done
            		done
            		mem_avail=`cat /sys/devices/system/node/node$node_num/meminfo | grep MemTotal | awk '{print $4}'`
            		mem_free=`cat /sys/devices/system/node/node$node_num/meminfo | grep MemFree | awk '{print $4}'`
            		printf "node_num=$node_num,mem_avail=$mem_avail,mem_free=$mem_free,cpus_in_node=$cpus_in_node,cpus" 
            		for ((a=0; a < $k; a++))
            		do
            			printf ":%d" ${lcpu[$a]}
            		done
            		printf "\n" 
            		i=`expr $i + 1`
		fi
	fi
done
sync
