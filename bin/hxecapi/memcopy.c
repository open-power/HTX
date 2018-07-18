/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

#include <memcopy.h>

struct wed {
	__u16 endian;				// Always = 1
	__u16 volatile status;		// Status bits
	__u16 volatile major;		// Logic version major #
	__u16 volatile minor;		// Logic version minor #;
	__u8 *from;					// Copy from address pointer
	__u8 *to;					// Copy to address pointer
	__u64 size;					// Bytes to copy
	struct wed *__next;			// Next WED struct
	__u64 error;				// Error bits
	// Reserve entire 128 byte cacheline for WED
	__u64 reserved01;
	__u64 reserved02;
	__u64 reserved03;
	__u64 reserved04;
	__u64 reserved05;
	__u64 reserved06;
	__u64 reserved07;
	__u64 reserved08;
	__u64 reserved09;
	__u64 reserved10;
};

char msg[MSG_TEXT_SIZE];
char device[DEV_ID_MAX_LENGTH];
char afu_device[DEV_ID_MAX_LENGTH];
static uint32_t exit_flag = 0;
static unsigned int buffer_cl;
static unsigned int timeout;
static unsigned int compare;
static unsigned int num_oper;
static unsigned int aligned;

int hxfcbuf_capi(struct htx_data * htx_d, uint32_t * cnt, char * wbuf, char * rbuf, __u64 len);

/*
 *Checks memory allocation status[Failure Code, Success Code].
 */
static int alloc_test (struct htx_data * htx_d, const char *str, __u64 addr, int ret)
{
	int32_t rc = -1;
    if (ret==EINVAL) {
		sprintf(msg, "Memory alloc failed for %s, memory size not a power of 2.\n",str);
		hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, msg);
        return(rc) ;
    }
    else if (ret==ENOMEM) {
		sprintf(msg, "Memory alloc failed for %s,insufficient memory available.\n",str);
		hxfmsg(htx_d, rc, HTX_HE_HARD_ERROR, msg);
        return(rc);
    }

    if (DEBUG) {
        printf ("Allocated memory at 0x%016llx:%s\n", (long long) addr, str);
    }
    return 0;
}

void check_errors (struct htx_data * htx_d, struct wed *wed0)
{
    if (wed0->error) {
        if (wed0->error & 0x8000ull)
            sprintf (msg, "AFU detected job code parity error\n");
        if (wed0->error & 0x4000ull)
            sprintf (msg, "AFU detected job address parity error\n");
        if (wed0->error & 0x2000ull)
            sprintf (msg, "AFU detected MMIO address parity error\n");
        if (wed0->error & 0x1000ull)
            sprintf (msg, "AFU detected MMIO write data parity error\n");
        if (wed0->error & 0x0800ull)
            sprintf (msg, "AFU detected buffer write parity error\n");
        if (wed0->error & 0x0400ull)
            sprintf (msg, "AFU detected buffer read tag parity error\n");
        if (wed0->error & 0x0200ull)
            sprintf (msg, "AFU detected buffer write tag parity error\n");
        if (wed0->error & 0x0100ull)
            sprintf (msg, "AFU detected response tag parity error\n");
        if (wed0->error & 0x0080ull)
            sprintf (msg, "AFU received AERROR response\n");
        if (wed0->error & 0x0040ull)
            sprintf (msg, "AFU received DERROR response\n");
        if (wed0->error & 0x0020ull)
            sprintf (msg, "AFU received NLOCK response\n");
        if (wed0->error & 0x0010ull)
            sprintf (msg, "AFU received NRES response\n");
        if (wed0->error & 0x0008ull)
            sprintf (msg, "AFU received FAULT response\n");
        if (wed0->error & 0x0004ull)
            sprintf (msg, "AFU received FAILED response\n");
        if (wed0->error & 0x0002ull)
            sprintf (msg, "AFU received CONTEXT response\n");
        if (wed0->error & 0x0001ull)
            sprintf (msg, "AFU detected unsupported job code\n");

		hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    }
}

void dump_trace (struct htx_data * htx_d, struct cxl_afu_h *afu_h,int command_lines, int response_lines, int control_lines)
{
    uint64_t trace_id, trace_time;
    uint64_t tdata0, tdata1, tdata2;
    int rc;
    int command_lines_outstanding = command_lines;
    int response_lines_outstanding = response_lines;
    int control_lines_outstanding = control_lines;
    char msg[4096];

    //trace_id = 0x8000C00000000000ull;
    trace_id = 0x8000C00000000000;//rblack changing this line

    sprintf (msg, "Command events:\n");
    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

    cxl_mmio_write64 (afu_h, MMIO_TRACE_ADDR, trace_id);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata0);
    if ( rc != 0 ) {
        sprintf (msg, "mmio error: %d \n", rc );
	    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
    }
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata1);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata2);
    command_lines_outstanding = command_lines_outstanding - 1;

    sprintf (msg,"0x%016llx:0x%016llx:0x%016llx\n", (long long) tdata0, (long long) tdata1, (long long) tdata2);
    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

    trace_time = (tdata0 >> 24) & 0xffffffffffull;

    sprintf (msg,"0x%010llx:", (long long) trace_time);
    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

    while (command_lines_outstanding != 0) {
        if (tdata0 & 0x0000000000800000ull) {
            sprintf (msg," Tag:0x%02x,%d Command:0x%04x,%d Addr:0x%016llx,%d abt:%d cch:0x%x size:%d\n",
              (unsigned) ((tdata0 >> 15) & 0xffull),
              (int) ((tdata0 >> 14) & 0x1ull),
              (unsigned) ((tdata0 >> 1) & 0x1fffull),
              (int) (tdata0 & 0x1ull),
              (long long) tdata1,
              (int) ((tdata2 >> 63) & 0x1ull),
              (int) ((tdata2 >> 60) & 0x7ull),
              (unsigned) ((tdata2 >> 44) & 0xffffull),
              (int) ((tdata2 >> 32) & 0xfffull)
			);
			hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
		}
	    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata0);
    	rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata1);
	    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata2);
    	command_lines_outstanding = command_lines_outstanding - 1;
    	trace_time = (tdata0 >> 24) & 0xffffffffffull;

	    sprintf (msg,"0x%010llx:", (long long) trace_time);
    	hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
    }
    printf ("\n");

    ++trace_id;

    sprintf (msg,"Response events:\n");
	hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

	cxl_mmio_write64 (afu_h, MMIO_TRACE_ADDR, trace_id);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata0);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata1);
    response_lines_outstanding = response_lines_outstanding - 1;
    trace_time = (tdata0 >> 24) & 0xffffffffffull;

    sprintf (msg,"0x%010llx:", (long long) trace_time);
    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

    while (response_lines_outstanding != 0) {
    	if (tdata0 & 0x0000000000800000ull) {
            sprintf (msg," Tag:0x%02x,%d Code:0x%02x credits:%d\n",
              (unsigned) ((tdata0 >> 15) & 0xffull),
              (int) ((tdata0 >> 14) & 0x1ull),
              (unsigned) ((tdata0 >> 6) & 0xffull),
              (unsigned) (((tdata0 & 0x3full) < 3) | ((tdata1 >> 61) & 0x7ull))
            );
			hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
    	}

	    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata0);
    	rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata1);
	    response_lines_outstanding = response_lines_outstanding - 1;
		trace_time = (tdata0 >> 24) & 0xffffffffffull;

    	sprintf (msg,"0x%010llx:", (long long) trace_time);
		hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
    }

    printf ("\n");

    ++trace_id;

    sprintf (msg,"Control events:\n");
    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

    cxl_mmio_write64 (afu_h, MMIO_TRACE_ADDR, trace_id);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata0);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata1);
    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata2);
    control_lines_outstanding = control_lines_outstanding - 1;
    trace_time = (tdata0 >> 24) & 0xffffffffffull;

    sprintf (msg,"0x%010llx:", (long long) trace_time);
    hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);

    while (control_lines_outstanding != 0) {
  		if (tdata0 & 0x0000000000800000ull) {
        	sprintf (msg," Command:0x%02x,%d Addr:0x%016llx,%d\n",
              (unsigned) ((tdata0 >> 15) & 0xffull),
              (int) ((tdata0 >> 14) & 0x1ull),
              (long long) ((tdata0 << 50) | (tdata1 >> 14)),
              (int) ((tdata1 >> 13) & 0x1ull)
      	    );
			hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
	    }
    	if (tdata1 & 0x0000000000000800ull) {
		    sprintf (msg,"0x%010llx:", (long long) trace_time);
	    	hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
        	sprintf (msg," Done, Error:0x%016llx\n",
              (long long) ((tdata1 << 53) | (tdata2 >> 11))
	        );
	  		hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
    	}
	    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata0);
    	rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata1);
	    rc = cxl_mmio_read64 (afu_h, MMIO_TRACE_ADDR, &tdata2);
    	control_lines_outstanding = control_lines_outstanding - 1;
    	trace_time = (tdata0 >> 24) & 0xffffffffffull;
	    sprintf (msg,"0x%010llx:", (long long) trace_time);
		hxfmsg(htx_d, -1, HTX_HE_SOFT_ERROR, msg);
    }
  	printf ("\n");

}

int main (int argc, char *argv[])
{
	char rule_filepath[PATH_SIZE];

	int ret;

	int32_t rc = 0;
	int32_t oper = 0;

	uint64_t stat_ctl_reg_wrdata = 0x0000000000000000;
    uint64_t stat_ctl_reg_rddata;
    int com_trace_reads = 3;
    int resp_trace_reads = 3;
    int ctl_trace_reads = 2;

	uint32_t stanza = 0, num_stanzas = 0;
	uint32_t miscompare_count = 0;
 
	/*
	 * HTX Data Structure and rules file variable
	 */
	struct htx_data htx_d;


    /*
     * Copy Command line argument passed in out internal data structure
     */
	memset(&htx_d, 0x00, sizeof(struct htx_data));
	if(argc < 4) {
		printf("Usage <hxecapi_binary_name> <dev_name> <REG/OTH> <rules_file> \n");
		return(0);
	}

	if(argv[0]) strcpy(htx_d.HE_name, argv[0]);
	if(argv[1]) strcpy(htx_d.sdev_id, argv[1]);
	if(argv[2]) strcpy(htx_d.run_type,argv[2]);
	if(argv[3]) strcpy(rule_filepath, argv[3]);

	hxfupdate(START, &htx_d);

	strcpy(afu_device,basename(htx_d.sdev_id));
	sprintf(device, "/dev/cxl/%s",afu_device);
	sprintf(msg,"argc %d argv[0] %s argv[1] %s argv[2] %s argv[3] %s \n", argc, argv[0], argv[1], argv[2], argv[3]);
	hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);

	rc = get_rule_capi(&htx_d, rule_filepath, &num_stanzas);
	if(rc == -1) {
		sprintf(msg, "Rule file parsing failed. rc = %#x\n", rc);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
		return(rc);
	}
	if(DEBUG) print_rule_file_data(num_stanzas);

	do {
		stanza = 0;

		for(stanza = 0; stanza < num_stanzas; stanza ++) {

			miscompare_count = 0;

			/*
			 * Update Stanza count for SUP
			 */
			htx_d.test_id = stanza + 1;
			hxfupdate(UPDATE, &htx_d);

			if(DEBUG) printf("stanza=%d \n", stanza);

			buffer_cl	= rule_data[stanza].buffer_cl;
			timeout 	= rule_data[stanza].timeout;
			aligned 	= rule_data[stanza].aligned;
			compare 	= rule_data[stanza].compare;
			num_oper    = rule_data[stanza].num_oper;


			for(oper = 0; oper < num_oper; oper++) {

				// Map AFU
				struct cxl_afu_h *afu_h;
				afu_h = cxl_afu_open_dev (device);
				if (!afu_h) {
					sprintf(msg, "cxl_afu_open_dev for device failed errno = %d %s\n",errno,strerror(errno));
					hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
					return -1;
				}

				// Prepare WED
				struct wed *wed0 = NULL;
				ret = posix_memalign ((void **) &(wed0), CACHELINE_BYTES,
									  sizeof (struct wed));
				if(alloc_test(&htx_d, "WED", (__u64) wed0, ret)) {
					sprintf(msg, "Memory allocation failed\n");
					hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
					return -1;
				}
				if(DEBUG)
					printf("Allocated WED memory @ 0x%016llx\n", (long long) wed0);

				wed0->endian = 1;
				wed0->status = 0;
				wed0->major = 0xFFFF;
				wed0->minor = 0xFFFF;
				wed0->size = buffer_cl*CACHELINE_BYTES;

				__u8 *from = NULL;
				__u8 *to = NULL;

				/*
				Set number of entries to read from trace array.Trace array only 256 entries deep,
				so never need to read more than that or will produce redundant data.Don't read
				trace array lines that won't be set. Harmless on hardware as invalid lines will
				read as F's.Reading unitialized RAM entries may cause parity errors on mmio bus
				in simulation.Anticapte number of entires to read. Formula is base of 3 for
				command and response trace arrays for wed reads/writes.Add 2*buffer_cl, because
				there will be 2 commands/responses for each cacheline: cmd/rsp for read and
				cmd/rsp for write.
				*/

				if(buffer_cl >= 127) {
					com_trace_reads = 256;
					resp_trace_reads = 256;
				} else {
				    com_trace_reads = com_trace_reads + 2*buffer_cl;
				    resp_trace_reads = resp_trace_reads + 2*buffer_cl;
				}

				/* Claim from memory buffer. Force alingment of address on cacheline boundary. */
				if (aligned) {
					ret = posix_memalign ((void **) &(from), CACHELINE_BYTES, wed0->size);
					if(alloc_test(&htx_d, "From", (__u64) wed0, ret)) {
						sprintf(msg, "Memory allocation failed\n");
						hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
					}
				}
				else {
					from = (__u8 *) malloc (2*wed0->size);
					if (!from) {
						sprintf(msg, "malloc:From::Memory allocation failed\n");
                        hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
					}
				}

				if(DEBUG)
					printf("Allocated From memory @ 0x%016llx\n", (long long) from);

				/* Claim to memory buffer. Force alingment of address on cacheline boundary. */
				if (aligned) {
					ret = posix_memalign ((void **) &(to), CACHELINE_BYTES, wed0->size);
					if(alloc_test(&htx_d, "To", (__u64) wed0, ret)) {
						sprintf(msg, "Memory allocation failed\n");
                        hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
					}
				}
				else {
					to = (__u8 *) malloc (2*wed0->size);
					if (!to) {
						sprintf(msg, "malloc:To::Memory allocation failed\n");
                        hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
					}
				}

				if(DEBUG)
					printf("Allocated To memory @ 0x%016llx\n", (long long) to);

				/* Polute from buffer with random data*/
				__u64 seed = time (NULL);

				/*
				  Regression tests
				  seed = 1409339338;
				  seed = 1404967577;
				  seed = 1404849600;
				  seed = 1404443769;
				  seed = 1404836371;
				  seed = 1404966788;
				  seed = 1404959792;  wed0->size = 48*CACHELINE_BYTES;
				  seed = 1404855547;  wed0->size = 2*CACHELINE_BYTES;
				  seed = 1404850291;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1404850494;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1404852769;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1404856112;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1404857589;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1404916270;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1404958891;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1414612324;  wed0->size = 1*CACHELINE_BYTES;
				  seed = 1414746286;  wed0->size = 1*CACHELINE_BYTES;
				*/

				printf ("Using seed %lld\n", (long long) seed);
				srand (seed);

				if (aligned) {
					wed0->from = from;
					wed0->to = to;
				}
				else {
					wed0->from = &(from[rand() % (wed0->size)]);
					wed0->to = &(to[rand() % (wed0->size)]);
					wed0->size -= (rand() % CACHELINE_BYTES);
					if (DEBUG) {
						printf ("Unaligned mode offsets:\n");
						printf ("\tFrom: 0x%016llx\n", (long long) wed0->from);
						printf ("\tTo:   0x%016llx\n", (long long) wed0->to);
						printf ("\tSize: 0x%016llx\n", (long long) wed0->size);
					}
				}
				int i;

				for(i=0;i<(wed0->size);i++)
					wed0->from[i]=(oper+10);

				printf ("Starting copy of %lld bytes from 0x%016llx to 0x%016llx\n",
				   (long long) wed0->size, (long long) wed0->from, (long long)
				   wed0->to);

				/* Send start to AFU */
				ret = cxl_afu_attach (afu_h, (__u64) wed0);

				if(ret < 0 ) {
					sprintf(msg, "cxl_afu_attach for device failed errno = %d %s. \n",errno,strerror(errno));
                    hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
                    return -1;
                }

				/* Map AFU MMIO registers */
				if (DEBUG)
					printf ("Mapping AFU registers...\n");

                ret = cxl_mmio_map (afu_h, CXL_MMIO_BIG_ENDIAN);

                if(ret < 0 ) {
                	sprintf(msg, "cxl_mmio_map for device failed errno = %d %s. \n",errno,strerror(errno));
                    hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
                    return -1;
                }

				/* Pre mmio section. Do mmio reads and writes. Set bit 0 to 1 when writing trace options
				register to kick off copy routine*/
				stat_ctl_reg_wrdata = 0x8000000000000000;
				ret = cxl_mmio_read64 (afu_h, MMIO_STAT_CTL_REG_ADDR, &stat_ctl_reg_rddata);
				if(ret < 0 ) {
                    sprintf(msg, "cxl_mmio_read64 for device failed errno = %d %s. \n",errno,strerror(errno));
                    hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
                }

				if(DEBUG) {
					printf("Trace Options register is %016llx after attach. Waiting for bit 32 to be a 1 (not including this read).\n", (long long) stat_ctl_reg_rddata);
				}

				for(i=0;i<20;i++) {
					ret = cxl_mmio_read64 (afu_h, MMIO_STAT_CTL_REG_ADDR, &stat_ctl_reg_rddata);
					if(ret < 0 ) {
						sprintf(msg, "cxl_mmio_read64 for device failed errno = %d %s. \n",errno,strerror(errno));
						hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
    				}
				    stat_ctl_reg_rddata = stat_ctl_reg_rddata >> 31;
				    stat_ctl_reg_rddata = stat_ctl_reg_rddata % 2;
				    if(DEBUG) {
					    printf("Pre mmio state bit is %08lx\n", stat_ctl_reg_rddata);
				    }
				    if(stat_ctl_reg_rddata == 1)
				    	break;
				    if(i==19){
					    sprintf(msg, "ERROR: Never hit pre mmio state.\n");
						hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
				    }
				}

				/*Extra mmios here before starting copy function*/

				if(DEBUG) {
				    printf("Writing bit 0 of trace options register to 1 to end pre mmio stage\n");
				}

/*Leave pre-mmio state*/
				ret = cxl_mmio_write64 (afu_h, MMIO_STAT_CTL_REG_ADDR, stat_ctl_reg_wrdata);
				if(ret < 0 ) {
			        sprintf(msg, "cxl_mmio_write64 for device failed errno = %d %s. \n",errno,strerror(errno));
			        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
			    }

				/* Wait for AFU to start or timeout*/
				struct timespec start, now;
				double time_passed;
				if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
					sprintf(msg, "clock_gettime failed errno = %d %s\n",errno,strerror(errno));
					hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
					return -1;
				}

				now = start;

				while (wed0->major==0xFFFF) {
					struct timespec ts;
					ts.tv_sec = 0;
					ts.tv_nsec = 100;
					nanosleep(&ts, &ts);
					if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
						sprintf(msg, "clock_gettime failed errno = %d %s\n",errno,strerror(errno));
						hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
					}
					time_passed = (now.tv_sec - start.tv_sec) +
						   (double)(now.tv_nsec - start.tv_nsec) /
						   (double)(1000000000L);
					if (((int) time_passed) > timeout)
						break;
				}
				//wed0 major field untouched after timeout time? Error!
				if (wed0->major==0xFFFF) {
					sprintf(msg, "Timeout after %d seconds waiting for AFU to start\n",timeout);
					check_errors (&htx_d, wed0);
					dump_trace (&htx_d, afu_h, com_trace_reads, resp_trace_reads, ctl_trace_reads);
					hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
					return -1;
				}

				printf ("AFU has started, waiting for AFU to finish...\n");
				fflush (stdout);

				ret = cxl_mmio_read64 (afu_h, MMIO_STAT_CTL_REG_ADDR, &stat_ctl_reg_rddata);
				if(ret < 0 ) {
			        sprintf(msg, "cxl_mmio_read64 for device failed errno = %d %s. \n",errno,strerror(errno));
			        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
			    }

			    if(DEBUG) {
				    printf("Trace Options register is %016llx shortly after start\n", (long long) stat_ctl_reg_rddata);
			    }

				/* Wait for AFU to signal job complete or timeout*/
				if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
					sprintf(msg, "clock_gettime failed errno = %d %s\n",errno,strerror(errno));
					hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
					return -1;
				}
				now = start;
				while (!wed0->status) {
					struct timespec ts;
					ts.tv_sec = 0;
					ts.tv_nsec = 100;
					nanosleep(&ts, &ts);
					if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
						sprintf(msg, "clock_gettime failed errno = %d %s\n",errno,strerror(errno));
						hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
						return -1;
					}
					time_passed = (now.tv_sec - start.tv_sec) +
						   (double)(now.tv_nsec - start.tv_nsec) /
						   (double)(1000000000L);
					if (((int) time_passed) > timeout)
						break;
				}

				ret = cxl_mmio_read64 (afu_h, MMIO_STAT_CTL_REG_ADDR, &stat_ctl_reg_rddata);
				if(ret < 0 ) {
			        sprintf(msg, "cxl_mmio_read64 for device failed errno = %d %s. \n",errno,strerror(errno));
			        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
			    }

			    stat_ctl_reg_rddata = stat_ctl_reg_rddata >> 30;
				stat_ctl_reg_rddata = stat_ctl_reg_rddata % 2;

				if(DEBUG) {
					printf("Post mmio state bit is %08lx\n", stat_ctl_reg_rddata);
				}

				while (stat_ctl_reg_rddata != 1) {
				    struct timespec ts;
				    ts.tv_sec = 0;
				    ts.tv_nsec = 100;
				    nanosleep(&ts, &ts);
				    if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
  				        perror("clock_gettime");
				        return -1;
				    }
				    time_passed = (now.tv_sec - start.tv_sec) +
                   (double)(now.tv_nsec - start.tv_nsec) /
                   (double)(1000000000L);
				    if (((int) time_passed) > timeout) {
				        printf("ERROR! Trace options post mmio start bit still isn't set!\n");
					    return -1;
				    }
					ret = cxl_mmio_read64 (afu_h, MMIO_STAT_CTL_REG_ADDR, &stat_ctl_reg_rddata);
				    if(ret < 0 ) {
				        sprintf(msg, "cxl_mmio_read64 for device failed errno = %d %s. \n",errno,strerror(errno));
				        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
				    }
				    stat_ctl_reg_rddata = stat_ctl_reg_rddata >> 30;
				    stat_ctl_reg_rddata = stat_ctl_reg_rddata % 2;
				} // end while

				ret = cxl_mmio_read64 (afu_h, MMIO_STAT_CTL_REG_ADDR, &stat_ctl_reg_rddata);
				if(ret < 0 ) {
			        sprintf(msg, "cxl_mmio_read64 for device failed errno = %d %s. \n",errno,strerror(errno));
				    hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
				}
				if(DEBUG) {
					printf("Trace Options register is %016llx indicating post mmio stage. Dumping Trace Arrays....\n", (long long) stat_ctl_reg_rddata);
				}

			    /*Unconditional dump of trace arrays.
				  dump_trace(&htx_d, afu_h,com_trace_reads,resp_trace_reads,ctl_trace_reads);
				  if(DEBUG) {
					  printf("Finished Dumping Trace arrays. Writing bit of Trace options register to send memcpy back to idle\n");
				  }
				  Other mmios could be added here other than dumping the trace arrays.*/

				stat_ctl_reg_wrdata = 0x4000000000000000;
				ret = cxl_mmio_write64 (afu_h, MMIO_STAT_CTL_REG_ADDR, stat_ctl_reg_wrdata);
				if(ret < 0 ) {
					sprintf(msg, "cxl_mmio_write64 for device failed errno = %d %s. \n",errno,strerror(errno));
					hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
				}
				if(compare) {
					rc = hxfcbuf_capi(&htx_d, &miscompare_count, (char *)wed0->from, (char *)wed0->to, wed0->size);
					if(rc) {
						htx_d.bad_writes += 1;
					} else {
						printf("**************************: oper = %d, r_buf=%#llx, w_buf=%#llx, bufsize=%#lx \n",  oper, (unsigned long long)wed0->from,
                                                       (unsigned long long)wed0->to, wed0->size);

	                    htx_d.good_writes += 1;
    	                htx_d.bytes_writ += wed0->size;
					}
        	        hxfupdate(UPDATE, &htx_d);
				}

				if (DEBUG) {
					check_errors (&htx_d, wed0);
				}
				/* Unmap AFU*/
				ret = cxl_mmio_unmap (afu_h);

				if(ret < 0 ) {
                	sprintf(msg, "cxl_mmio_unmap for device failed errno = %d %s. \n",errno,strerror(errno));
                    hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
                    return -1;
				}

				cxl_afu_free (afu_h);

				if((__u8 *)from) {
					free((__u8 *)from);
					from = NULL;
				}
				if((__u8 *)to) {
					free((__u8 *)to);
					to = NULL;
				}

			}/*End of for loop*/


		}
		/* Off num_stanza loop */

		/*
		 * Update cycle count for SUP
		 */
		hxfupdate(FINISH, &htx_d);
		if(exit_flag)
			break;

    } while((rc = strcmp(htx_d.run_type, "REG") == 0) || (rc = strcmp(htx_d.run_type, "EMC") == 0));

	return 0;
}


int hxfcbuf_capi(struct htx_data * htx_d, uint32_t * cnt, char * wbuf, char * rbuf, __u64 len) {
	char path[MAX_STRING];
	char s[3];
	char work_str[MAX_STRING], misc_data[MAX_STRING];
	char * msg_ptr = NULL;
	uint32_t i = 0, j = 0, mis_flag = FALSE;
	int32_t rc = 0;

	if(memcmp(wbuf, rbuf, len)) {
		while ((mis_flag == FALSE) && (i < len)) {
			if (wbuf[i] != rbuf[i]) {
				mis_flag = TRUE;
			} else {
				i++;
			}
		}
	}

	if (mis_flag == TRUE) {         /* problem with the compare?                    */
		rc = -1;
		msg_ptr = misc_data;        /* show bad compare                         */
		sprintf(msg_ptr, "Miscompare at displacement = %d(%#x). Transfer Size=%lu(%#lx) \n",i, i, len, len);
		strcat(msg_ptr, "Expected Data = ");

		memset(work_str, '\0', MAX_STRING);
		for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++) {
		#ifdef  __HTX_LINUX__
			sprintf(s, "%x", wbuf[j]);
		#else
			sprintf(s, "%0.2x", wbuf[j]);
		#endif
			strcat(work_str, s);
		}
		sprintf(msg_ptr + strlen(msg_ptr), "%s\n", work_str);
		strcat(msg_ptr, "Actual Data   = ");

		memset(work_str, '\0', MAX_STRING);
		for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++) {
		#ifdef  __HTX_LINUX__
		sprintf(s, "%x", rbuf[j]);
		#else
			sprintf(s, "%0.2x", rbuf[j]);
		#endif
			 strcat(work_str, s);
		}
		sprintf(msg_ptr + strlen(msg_ptr), "%s\n", work_str);

        if (* cnt < MAX_MISCOMPARES) {
		  /*
		   * Copy write and read buffers to dump file.
		   */

		   (*cnt)++;

		   /*
		   * Expected buffer path
		   */
			strcpy(path, htx_d->htx_exer_log_dir);
			strcat(path, "htx_dev_cxl_");
			strcat(path, afu_device);
			strcat(path, ".expected");
			sprintf(work_str, ".%d", *cnt);
			strcat(path, work_str);
			hxfsbuf(wbuf, len, path, htx_d);
			sprintf(msg_ptr + strlen(msg_ptr), "The miscompare buffer dump files are %s,  ", path);
			/*
			 * Actual buffer path
			 */

			strcpy(path, htx_d->htx_exer_log_dir);
			strcat(path, "htx_dev_cxl_");
			strcat(path, afu_device);
			strcat(path, ".actual");
			sprintf(work_str, ".%d", *cnt);
			strcat(path, work_str);
			hxfsbuf(rbuf, len, path, htx_d);
            sprintf(msg_ptr + strlen(msg_ptr), " and %s\n", path);
        } else {
			sprintf(work_str, "The maximum number of saved miscompare \
									buffers (%d) have already\nbeen saved.  The read and write buffers for this \
									miscompare will\nnot be saved to disk.\n", MAX_MISCOMPARES);
			strcat(msg_ptr, work_str);
		}
		hxfmsg(htx_d, i, HTX_HE_MISCOMPARE, msg_ptr);
	} /* endif */
	return(rc);

} /* hxfcbuf() */

