/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

static char sccsid[] = "@(#)84	1.1  src/htx/usr/lpp/htx/bin/hxesamp/hxesamp.c, exer_samp, htxubuntu 11/2/09 03:42:44";

#include "hxihtx.h"
#include <signal.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct htx_data htx;              /* HTX Hardware Exerciser data         */
int fdes;                         /* file descriptor                     */

/*
 * NAME: main()
 *
 * This is a sample HTX hardware exerciser program to show how to interface
 * with HTX.
 *
 * This is the minimum required to be able to run under the HTX Supervisor.
 *
 * This exerciser can be invoked by either the HTX supervisor or directly from
 * the command line to bypass communicating with the HTX Supervisor (e.g.
 * run in "stand-alone" mode).
 *
 * To run in "stand-alone" mode simply enter the following command:
 * hxesamp /dev/samp OTH
 *
 * Since argv[2] is OTH, the hxfupdate() functions calls will not try to
 * communicate with the HTX supervisor using IPC shared memory functions, and
 * any messages put out using hxfmsg() will be written to either "stdout" or
 * "stderr" rather than being sent to HTX using IPC msg send/msg receive.
 *
 * If the message severity code is <=6, the message will got to stderr, if >=7
 * stdout.
 *
 * This allows the exerciser to be tested without interfacing with the
 * HTX supervisor.
 *
 * When the HTX Supervisor invokes the exerciser, it supplies arg[1] /dev/samp,
 * argv[2] run_type (REG or EMC), and optionally argv[3] a file path name
 * containing input parameters.
 *
 * argv[1], argv[2], and argv[3] come from the htx master device table (MDT).
 *
 * The run_type is determined from HTX main menu option 3.
 *
 * Normally the device to be exercised is passed to the exerciser in argv[1]
 * thus allowing the same exerciser to run multiple devices, however any
 * unique string can be used. This sample exerciser uses "samp".
 *
 */

main (argc, argv)
int argc;
char *argv[];
{
	int rc;                         /* generic return code                     */
	int error_code;                 /* hxfmsg error code                       */
	int severity_code;              /* hxfmsg msg severity code                */
	struct sigaction sigvector;     /* structure for signals                   */

	/* SIGTERM signal handler function         */
	void SIGTERM_handler(int, int, struct sigcontext *);

	/*
	 * Set up HTX HE data structure...
	 */
	(void) strcpy(htx.HE_name, argv[0]);
	(void) strcpy(htx.sdev_id, argv[1]);
	(void) strcpy(htx.run_type, argv[2]);

	/*
	 * Make initial call to hxfupdate()...
	 */

	hxfupdate(START, &htx);

	error_code=0;
	severity_code=7;
	hxfmsg(&htx, error_code, severity_code, "exerciser started");

	/*
	 * Open device to be exercised here, if an open is required.
	 * exit with non-zero return code if not successful.
	 */

	/*
	 * Set SIGTERM_handler() to catch SIGTERM signal...
	 * Since all HTX exercisers are written to loop forever when running
	 * under the HTX Supervisor this is the normal way to terminate the
	 * exerciser.
	 */

	sigemptyset(&(sigvector.sa_mask));  /* do not block signals            */
	sigvector.sa_flags = 0;             /* do not restart sys calls        */

	sigvector.sa_handler = (void (*)(int)) SIGTERM_handler;
	(void) sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);


	/*
	 * Main loop to exercise device *****************************************
	 */
	do {
		/*
		 * Assume 512 bytes written successfully to device.
		 * Since this is for illustration purposes only, we will not
		 * actually perform the I/O, and will force a good return code.
		 */
		rc = 0;        /* force good return code */
		if (rc == 0) {
			htx.bytes_writ     = 512;
			htx.good_writes    = 1;
		}
		else {
			htx.bad_writes     = 1;
			error_code=10;
			severity_code=2;
			(void) hxfmsg(&htx, error_code, severity_code, "write error");
		} /* endif */

		(void) hxfupdate(UPDATE, &htx);

		/*
		 * Assume an error occurred reading from the device.
		 * Since this is for illustration purposes only, we will not
		 * actually perform the I/O, and will force a bad return code.
		 */
		rc = 1;
		if (rc == 0) {
			htx.bytes_read     = 512;
			htx.good_reads     = 1;
		}
		else {
			htx.bad_reads      = 1;
			error_code=15;
			severity_code=2;
			(void) hxfmsg(&htx, error_code, severity_code, "read error");
		} /* endif */

		/*
		 * The following hxfupdate() will let the htx supervisor know
		 * the exerciser is still running, update the htx status (htx main
		 * menu option 5), and the htx statistics (htx main menu option 6).
		 */
		(void) hxfupdate(UPDATE, &htx);

		/*
		 * Remainder of device exercise goes here
		 */
		(void) hxfupdate(FINISH, &htx); /* update HTX cycle counter        */

	}while (strcmp(htx.run_type, "REG") == 0 || strcmp(htx.run_type ,"EMC") == 0);

	/*
	 * End of main loop to exercise device.
	 * Come here only when running in stand "stand-alone" mode.
	 * close device and cleanup as necessary.
	 */
	error_code=0;
	severity_code=7;
	hxfmsg(&htx, error_code, severity_code, "exerciser finished");
	exit(0);

} /* main() */

/*
 * NAME: SIGTERM_handler()
 *
 * FUNCTION: Sample HTX HE program SIGTERM handler function.
 *
 * This code shows the structure of a typical HE SIGTERM signal
 * handler function.
 *
 */

void SIGTERM_handler(int sig, int code, struct sigcontext *scp)
{
	/*
	 * Come here as a result of the htx supervisor sending "sigterm" on htx
	 * shutdown.
	 *
	 * Close device, close any open files, general cleanup here
	 */
	(void) hxfmsg(&htx, 0, 7, "SIGTERM received, terminating");
	exit(0);

} /* SIGTERM_handler() */
