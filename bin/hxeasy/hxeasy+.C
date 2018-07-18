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
//   FUNCTIONS: CkPath
//		GetPath
//		PrtSyntax
//		SIGTERM_hdl
//		get_rule
//		main
//		pat_to_buf
//		sync_ports

#ifdef __HTX_LINUX__
#include <headers.h>
#else
#include"headers.h"     // This file has all the header files that hxeasy uses.
#endif


// the main functions expects to be passed three parameters:
//      1. exerciser name  hxeasy.
//      2. dev_id   ex. /dev/com1 or /dev/tty1
//      3. Run type...   REG, OTH or EMC.
//      4. rule file name.  


/*****************************************************************************/
/*  GLOBAL VARIABLES                                                         */
/*****************************************************************************/

ARGS_T *ArgDataPtr; 	// needs to be global so sig handler can use it
/*
 * Local static functions
 */
static int CkPath(const char path[], char * err_msg_text);
static int GetPath(char path[], const char pathin[], int len,
                    const char envirVar[], const char defaultPath[],
                    ARGS_T *args);

int main(int argc, char *argv[])
{

		// declare the args structrue. This has info that is needed by
		// the programs functions and will be passed to all functions that need
		// it.  I put this struct in a object so I could get it off
		// the stack. The class CArgData will malloc space for the
		// struct.
	CArgData global_data_object;

		// set value of ArgDataPtr
	ArgDataPtr = global_data_object.GetArgDataPtr();
	
		// zero the struct.....
	memset(ArgDataPtr,0x00,sizeof(ARGS_T));
	ArgDataPtr->global_data=(CArgData *)&global_data_object;


	// declare an object for use in communication with htx msg files.
	CmsgBase BaseMsgOjbect;
		// Get a stats pointer for initialization
	struct htx_data *stats=BaseMsgOjbect.GetStatsPtr();
		// Get a pointer to the Base Msg object. This will be passed in the 
		// ArgData structure so that all functions can use it to set up
		// a msg buffer for writting to htx message files.
	ArgDataPtr->BaseMsgObjectPtr=(CmsgBase *)&BaseMsgOjbect;

	FILE            *rfd;          // rule file descriptor
	char            error;         // rule stanza error flag 
	int             line_number;   // cur line # in Rules file
	int             rc;        
	int             loop_flag=1;   // set this = 1, so I will keep running
	char			writego[16];

	// used only in aix. 
	char			cmdbuf[120];
	int				semval;

	// check out the inputs. Inputs fixed to require four
	if(argc - 1 != 3) {
#ifdef __HTX_LINUX__
		/*
		 * clean up Semphores
		 */ 
		if(argc == 2 && (strcmp(argv[1],"-c") == 0) )	{
		//	system("ipcs | grep 0xeea | awk '{print $2}'| xargs ipcrm sem 2>/dev/null");
			system(". /tmp/hxeasy.ipcrm");
			exit(0);
		}
#endif
	    PrtSyntax(argv[0]);
		RETURN(BADBOY);
	} else {
		// copy in the args.
	    strncpy(stats->HE_name, argv[0], 32);
	    stats->HE_name[31] = '\0';
	    strncpy(stats->sdev_id, argv[1], 32);
	    stats->sdev_id[31] = '\0';
	    strncpy(stats->run_type, argv[2], 4);
	    stats->run_type[3] = '\0';
	    int i = 0;
	    while(*(stats->run_type+i) != '\0') {
		*(stats->run_type+i) = toupper(*(stats->run_type+i));
		i++;
	    }
	}
/********************************************************************/
/* Check for valid device type                                      */
/********************************************************************/
	if(strlen(argv[1])<=8) {
	    PrtSyntax(argv[1]);
		RETURN(BADBOY);
	}

/********************************************************************/
/* Check for valid run_type argument.                               */
/********************************************************************/
    if( strcmp(stats->run_type, "OTH") && 
	strcmp(stats->run_type, "REG") &&
	strcmp(stats->run_type, "EMC") ) {
	PrtSyntax(argv[0]);
		RETURN(BADBOY);
    }   


/********************************************************************/
/* Call hxfupdate() to initialize htx_data structure and to         */
/* initialize the hxfupdate procedure.                              */
/********************************************************************/
    stats->msg_text[0] = '\0';
    hxfupdate(START, stats);


    // NOTE!!!  you must call hxfupdate before the first Cmsg class declaration
	// write msg to htxmsg log that says we've started.
	// Since the call to hxfupdate(START  sets up the stats pointer data
	// that is needed by all the Cmsg objects...

	// declare a temp Cmsg object to use in the main section of code
	// for htx communication. Makes a copy of orig htx stats struct.
	Cmsg ToHtx(ArgDataPtr);
	// Now save the ToHtx class pointer for use by other subroutines. 
	ArgDataPtr->MainPtr=(Cmsg *)&ToHtx;


	// set up signal handler to handle control-C to stop the run.  


	signal(SIGHUP,sig_handler);
	signal(SIGINT,sig_handler);
	signal(SIGTERM,sig_handler);
	signal(SIGQUIT,sig_handler);
	signal(SIGABRT,sig_handler);
#ifndef __HTX_LINUX__
	/*
	 * There is no SIGDANGER in Linux
	 */ 
	signal(SIGDANGER,sig_handler);
#endif

//#ifdef ALARM_S
	signal(SIGALRM,sig_handler);
//#endif

 /********************************************************************/
/* Check Rules path for validity.                                   */
/********************************************************************/
   if(GetPath(  RULESPATH, 
		argv[argc -1], 
		FNAME_MAX, 
		"HTXRULES",                                  
		RULE_PATH,
		ArgDataPtr) == 0) 
	{
	    RETURN(BADBOY);
	}

	// this is first message to message log.... THE START MESSAGE>
    sprintf(ToHtx.GetMsgAddr(), "%s %s %s", 
		stats->sdev_id,
		stats->run_type,
		RULESPATH);
    ToHtx.SendMsg(0,HTX_HE_INFO);


/********************************************************************/
/* Open ArgDataPtr->Rules file for reading.                                     */
/********************************************************************/
   if ((rfd = fopen(RULESPATH, "r")) == (FILE *) NULL) {        /* Problem? */
      sprintf(ToHtx.GetMsgAddr(), "Error opening ArgDataPtr->Rules file -  \n" );
      ToHtx.SendMsg(1, HTX_HE_HARD_ERROR);                      
      RETURN(BADBOY);
   }

/********************************************************************/
/* Check each rule stanza for validity. *****************************/
/********************************************************************/
/* This instantiation of get_rule doesn't just check the ArgDataPtr->Rules file for
 * correctness. Some ArgDataPtr->Rules such as pattern name
 * are set by this instance. Other ArgDataPtr->Rules are left in their default
 * state after this loop exits because get_rule is actually called
 * twice, first to read the ArgDataPtr->Rules file and second to return the EOF. 
 * This is probably bad because the (foolish) user may assume (I did) that
 * the ArgDataPtr->Rules are in force immediately after this call.
 */
   error = 'n';
   line_number = 0;
   while ((rc = get_rule(rfd, &line_number, &ArgDataPtr->Rules,ArgDataPtr)) != EOF) {
      
      if (rc == 1 || GetPath(   PATTERNPATH, 
				ArgDataPtr->Rules.pattern_id, 
				FNAME_MAX, 
				"HTXPATTERNS", 
				PATTERN_PATH,
				ArgDataPtr) == 0)
	 error = 'y';
   }                            /* endwhile */
   if (error == 'y') {
      RETURN(BADBOY);
   }

// We are ready to start talking to the
// device,  tty for AIX. POSIX compilant for AIX..
// there will be hardware configuration
// phase in AIX.  

	// Init Ccom Port class used to manipulate the Ports.
	// This class is not closed until main exits...
	
       /*
	* Port declaration is changed 'us signal handler is using delete to free this object
	* and Linux SCRICTLY PROHIBITS this sort of things
	* All such declaration should be cleaned up sometime...right now
	* not a high priority item
	*/

	ArgDataPtr->Port= (void *)new Ccom(stats->sdev_id,ArgDataPtr);
	Ccom & Port = (Ccom&) *((Ccom*)ArgDataPtr->Port);

	// make the initial setups and connection. .
	if(Port.ConnectPorts(ArgDataPtr)) { 
		sprintf(ToHtx.GetMsgAddr(), "Connect ports failed. \n" );
		ToHtx.SendMsg(1, HTX_HE_HARD_ERROR);                      
		RETURN(0); 
	}

	// Note for Aix the port is left open after the connection is made
	// and it will stay open for the reset of the test.......

/********************************************************************/
/* Start the test phase of the code......                           */
/********************************************************************/

	// Allocate the buffers needed.  Use size required.     
	// Allocating to a max limit uses too much memory       
	// for a large number of ttys. I use max numchars found 
	// in rules file
	CGetSpace      space(ArgDataPtr->Rules.num_chars+1);
	ArgDataPtr->Space_Class=(CGetSpace *)&space;


    // Create the events that all the threads need for sychronization..
	// and set pointers to these event objects in the argdata struct. 
	// All events are created in the not signaled state. the sem is 1

	// These first two events are used in the main thread to sync the start
	// and stop of the test passes in port to port operation.
	// I would like to create them only if wrap_type is not ePlug but
	// I can't use an if statement to encase the creation of objects
	// becuase the destructor will run when it leaves the if braces
	// and that removes the handle to the event. So I will have the
	// event constructor decide if I need the events or not. 

	CcreateEvent    OpenEventS(FALSE,FALSE,ArgDataPtr->OpenS,ArgDataPtr);  
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	ArgDataPtr->OpenEventS=(CcreateEvent *)&OpenEventS;
	CcreateEvent    OpenEventC(FALSE,FALSE,ArgDataPtr->OpenC,ArgDataPtr);  
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	ArgDataPtr->OpenEventC=(CcreateEvent *)&OpenEventC;

	// The Wstart events are used by the Reader and Writer. They must be 
	// set up based on which one is client and server. 
	
	// Set up the readers write start event.
    if(ArgDataPtr->Flags&CLIENT && ArgDataPtr->TestParms.wrap_type!=ePlug) {
        strcpy(writego,ArgDataPtr->WstartC);
    } else {
        strcpy(writego,ArgDataPtr->WstartS);
    }
    CcreateEvent    WstartReader(FALSE,FALSE,writego,ArgDataPtr);
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	ArgDataPtr->WstartReader=(CcreateEvent *)&WstartReader;

	// Set up the writers write start event.
    if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
        if(ArgDataPtr->Flags&CLIENT) {
			strcpy(writego,ArgDataPtr->WstartS);
        } else {
			strcpy(writego,ArgDataPtr->WstartC);
        }
    } else {
		// In the case of wrap plug only one process exists and the
		// same event will do for the synchronization so don't 
		// create a new event just make the WstartWriter event = to 
		// the WstartReader event. 
		ArgDataPtr->WstartWriter=(CcreateEvent *)&WstartReader;
		strcpy(writego,"INVALID_NAME");
    }

	CcreateEvent    WstartWriter(FALSE,FALSE,writego,ArgDataPtr);
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	if(strcmp(writego,"INVALID_NAME")!=0) {
		ArgDataPtr->WstartWriter=(CcreateEvent *)&WstartWriter;
	}

	// The Wstop events are used by the Reader and Writer. They must be 
	// set up based on which one is client and server. 
	// The readers use this event to tell the write to stop writting and
	// go back in the loop waiting for next test to start. 
	
	// Set up the readers write stop event.
    if(ArgDataPtr->Flags&CLIENT && ArgDataPtr->TestParms.wrap_type!=ePlug) {
        strcpy(writego,ArgDataPtr->WstopC);
    } else {
        strcpy(writego,ArgDataPtr->WstopS);
    }
    CcreateEvent    WstopReader(FALSE,FALSE,writego,ArgDataPtr);
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	ArgDataPtr->WstopReader=(CcreateEvent *)&WstopReader;

	// Set up the writers write stop event.
    if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
        if(ArgDataPtr->Flags&CLIENT) {
			strcpy(writego,ArgDataPtr->WstopS);
        } else {
			strcpy(writego,ArgDataPtr->WstopC);
        }
    } else {
		// In the case of wrap plug only one process exists and the
		// same event will do for the synchronization so don't 
		// create a new event just make the WstopWriter event = to 
		// the WstopReader event. 
		ArgDataPtr->WstopWriter=(CcreateEvent *)&WstopReader;
		strcpy(writego,"INVALID_NAME");
    }

	CcreateEvent    WstopWriter(FALSE,FALSE,writego,ArgDataPtr);
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	if(strcmp(writego,"INVALID_NAME")!=0) {
		ArgDataPtr->WstopWriter=(CcreateEvent *)&WstopWriter;
	}

	// The Wstoped events are used by the Reader and Writer. They must be 
	// set up based on which one is client and server. 
	// Used by the writers to tell the readers they have stopped. 	

	// Set up the readers write stoped event.
    if(ArgDataPtr->Flags&CLIENT && ArgDataPtr->TestParms.wrap_type!=ePlug) {
        strcpy(writego,ArgDataPtr->WstopedC);
    } else {
        strcpy(writego,ArgDataPtr->WstopedS);
    }
    CcreateEvent    WstopedReader(FALSE,FALSE,writego,ArgDataPtr);
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	ArgDataPtr->WstopedReader=(CcreateEvent *)&WstopedReader;

	// Set up the writers write stoped event.
    if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
        if(ArgDataPtr->Flags&CLIENT) {
			strcpy(writego,ArgDataPtr->WstopedS);
        } else {
			strcpy(writego,ArgDataPtr->WstopedC);
        }
    } else {
		// In the case of wrap plug only one process exists and the
		// same event will do for the synchronization so don't 
		// create a new event just make the WstopedWriter event = to 
		// the WstopedReader event. 
		ArgDataPtr->WstopedWriter=(CcreateEvent *)&WstopedReader;
		strcpy(writego,"INVALID_NAME");
    }

	CcreateEvent    WstopedWriter(FALSE,FALSE,writego,ArgDataPtr);
	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }
	if(strcmp(writego,"INVALID_NAME")!=0) {
		ArgDataPtr->WstopedWriter=(CcreateEvent *)&WstopedWriter;
	}

	// Now init the ArgData structs patern and write data ptrs.
	PATTERNDATA=space.GetPbuf();
	ArgDataPtr->TestParms.WriteDataPtr=space.GetWbuf();
	ArgDataPtr->TestParms.ReadDataPtr=space.GetRbuf();

	if(ArgDataPtr->Flags&ABORT) { RETURN(0); }

	// the first time thru here it is possible (in port to port mode) that
	// one of the processes could get here and on to the first sync_ports
	// before the other process does his event create on the open events.
	// If this happens the syncs will hang. So, I put a sync here to make
	// sure that no matter who gets here first he waits on the other process
	// giving it enough time to create the events. 


    if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
		// This will be created only if were not on wrap plug.
		// and it will stay around until its removed by delete after it is 
		// no longer needed. 
		CcreateEvent    Winit(FALSE,FALSE,ArgDataPtr->InitS,ArgDataPtr);
		if(ArgDataPtr->Flags&ABORT) return(BADBOY);

		int sem_gone=0;

        if(ArgDataPtr->Flags&CLIENT) {
            Winit.SignalEvent();    // set event to 0
        } else {
            Winit.SetEvent();    // set event to 1
        }
        // now wait for other process.
        while(1) {
			usleep(200000);
            if((semval=Winit.GetEvent()) < 0) {
                // the sem is gone, this means the other side has
                // already exited this section of code so I can
                // leave too.
                sem_gone=1;
                break;
            } else {
                if( (ArgDataPtr->Flags&CLIENT) && (semval==1) ) break;
                if( (ArgDataPtr->Flags&SERVER) && (semval==0) ) break;
            }
        }
		if(!sem_gone) {
			// now set the value back so the other process can leave if its
			// waiting.
			if(ArgDataPtr->Flags&CLIENT) Winit.SignalEvent();
			else Winit.SetEvent();
		} 
    }



/*****************************************************************************/
/* SPAWN THE WRITE THREAD>>>>>>>>                                            */
/*****************************************************************************/
    // create write thread class.
     CThread Write( Writer );

	 ArgDataPtr->Write_Class=(CThread *)&Write;

    // Now start the thread...........
    if(Write.Start( (LPDWORD) ArgDataPtr)) {
		sprintf(ToHtx.GetMsgAddr(),"Write.Start failed\n");
		ToHtx.SendMsg(0,HTX_HE_INFO);
		goto abortexit;
    }

	//******************************************************************
	//      Begin the test loops here......
	//******************************************************************
	

	do {

      // if abort occurs now after threads are started I have to 
      // get them to abort too.
      if(ArgDataPtr->Flags&ABORT) goto abortexit;

/********************************************************************/
/* Process each rule stanza.                                        */
/********************************************************************/
      (void) rewind(rfd);
      line_number = 0;

		// This reads one rule stanza each pass thru the while loop.
		// When the all the stanzas are read and tests run, we go to
		// the do loop and start all over
      while ((get_rule(rfd, &line_number, &ArgDataPtr->Rules,ArgDataPtr)) != EOF) {
		// Check the vality of the pattern file read from the current stanza.

		GetPath(PATTERNPATH, 
			ArgDataPtr->Rules.pattern_id, 
			FNAME_MAX, 
			"HTXPATTERNS", 
			PATTERN_PATH,
			ArgDataPtr);

		// I want to run the entire test from the main code. So the
		// runtest function will be place here within the main routine.


		// First check to see if theres a new pattern to read.
		if(strcmp(ArgDataPtr->Rules.pattern_id,ArgDataPtr->lastpattern_id)) {
			if((rc=ReadPattern(ArgDataPtr))) {
				sprintf(ToHtx.GetMsgAddr(),"ReadPattern failed\n");
				ToHtx.SendMsg(0,HTX_HE_INFO);
				goto abortexit;
			}
			strcpy(ArgDataPtr->lastpattern_id,ArgDataPtr->Rules.pattern_id);
		} 


		if(sync_ports(ArgDataPtr))  {
			sprintf(ToHtx.GetMsgAddr(),"sync_ports 1. failed\n");
			ToHtx.SendMsg(0,HTX_HE_INFO);
			goto abortexit;
		}

		// At this point I can set the test variables that are constant
		// for this run thru the rules file. Things like, flow control
		// (linux and aix), CLOCAL BRKINT or HUPCL(for aix).

		if(Port.ready_port(ArgDataPtr)) {
			sprintf(ToHtx.GetMsgAddr(),"Port.ready_port failed\n");
			ToHtx.SendMsg(0,HTX_HE_INFO);
			goto abortexit; 
		}

		if(sync_ports(ArgDataPtr))  {
			sprintf(ToHtx.GetMsgAddr(),"sync_ports 2. failed\n");
			ToHtx.SendMsg(0,HTX_HE_INFO);
			goto abortexit;
		}

		/**********************************************************************/
		/* Following is a series of 4 embedded FOR loops.  These loops allow  */
		/* multiple specifications for baud rate, character size, number of   */
		/* stop bits, and parity type respectively.  The NumOperLoop() allows */
		/* multiple repetitions of a stanza.   By using the NUM_OPER          */
		/* parameter, a much higher stress level can be acheived.             */
		/* NUM_CHARS = m*BUFSIZE = n*255 and NUM_OPER > 1 will yield the      */
		/* highest stress the the test unit.  In general NUM_OPER should be   */
		/* as large as possible but small enough to allow adequate cycles.    */
		/**********************************************************************/

		for (int ii = 0; ii < 16; ii++) {
			if (ArgDataPtr->Rules.cbaud[ii] == -1)  break; 

			// limit baud rate for specific devices....in AIX
			// 8port pci and new rans and wrap only to 230000
			// 8port isa to 115200
			// all others to 38400
					// if baud is negative and doing wrap test
			if( (	ArgDataPtr->Rules.cbaud[ii] < 0 && 
					ArgDataPtr->TestParms.wrap_type == ePlug) || 
					// or baud is bigger than 230400 on 8port pci or new rans
					(ArgDataPtr->Rules.cbaud[ii] > 230400 &&
					(ArgDataPtr->device_type == e8med ||
					ArgDataPtr->device_type == ejas ||
					ArgDataPtr->device_type == ejas422 ||
					ArgDataPtr->device_type == e128_232 ||
					ArgDataPtr->device_type == e128_422 ||
					ArgDataPtr->device_type == e8med422))  ||
					// or baud is bigger than 115200 on 8port isa
					(ArgDataPtr->Rules.cbaud[ii] > 115200 &&
					(ArgDataPtr->device_type == e8isa ||
					ArgDataPtr->device_type == e8422))  ||
                  // or baud is bigger than 115200 on med or 128 in port to port
                    ((ArgDataPtr->Rules.cbaud[ii] > 115200 &&
                        ArgDataPtr->TestParms.wrap_type != ePlug) &&
                    (ArgDataPtr->device_type == e8med ||
                    ArgDataPtr->device_type == e128_232 ||
                    ArgDataPtr->device_type == e128_422 ||
                    ArgDataPtr->device_type == e8med422))  ||
					// of device not 128 or 8port isa or jasmine and baud > 38400
					(   ArgDataPtr->device_type != e8isa &&
						ArgDataPtr->device_type != e8422 &&
						ArgDataPtr->device_type != ects3_16 &&
						ArgDataPtr->device_type != ects3_16_linux &&
						ArgDataPtr->device_type != ects3_16422 &&
						ArgDataPtr->device_type != ejas422 && 
						ArgDataPtr->device_type != ejas && 
						ArgDataPtr->device_type != e8med &&
						ArgDataPtr->device_type != e8med422 &&
			//			ArgDataPtr->device_type != e128p &&
						ArgDataPtr->device_type != e128_422 &&
						ArgDataPtr->device_type != e128_232 &&
						ArgDataPtr->Rules.cbaud[ii] > 38400) ) {
				   /*******************************************************/
				   /* It is not possible to do half_duplex on a wrap plug */
				   /*******************************************************/
/*			sprintf(ToHtx.GetMsgAddr(),"continue: type %d, baud %d\n",ArgDataPtr->device_type,ArgDataPtr->Rules.cbaud[ii]);
			ToHtx.SendMsg(0,HTX_HE_INFO);*/
				   continue; 
			} else {
				   /*******************************************************/
				   /* If at least one valid BAUD rate, set flag so don't  */
				   /* exit when ArgDataPtr->Rules file EOF is read.       */ 
				   /*******************************************************/
				ArgDataPtr->BaudValid =1;
			}
			// Set the baud rate in the ArgData structure. If a - 
			// rate is specifed to indicate half duplex, convert it 
			// to + and set the Half Duplex Flag.
			if(ArgDataPtr->Rules.cbaud[ii] > 0) { 
				ArgDataPtr->Flags&=~HALF_DUPLEX;
			} else {
				ArgDataPtr->Flags|=HALF_DUPLEX;
			}
			// Set up the baud_rate in terms struct or DCB.
			// NOTE: this also sets hibaud on device if it supports it
			// and the rate is higher than 38400...If rate is less than
			// 38400 hibaud is removed from device.  
			// This sends positive baud rate to baud_rate.....
			Port.baud_rate(ArgDataPtr,((ArgDataPtr->Rules.cbaud[ii] > 0) ? ArgDataPtr->Rules.cbaud[ii] : (- ArgDataPtr->Rules.cbaud[ii])));

			// Pick the next char size for the next test. 
			for (int jj = 0; jj < 4; jj++) {
			   if (ArgDataPtr->Rules.chsize[jj] == -1)  break;

				// Set up the char_size in terms struct or DCB.
			   Port.char_size(ArgDataPtr,ArgDataPtr->Rules.chsize[jj]);

			   // Now adjust the data for the char size specified.
			   pat_to_buf(      PATTERNDATA, 
						ArgDataPtr->TestParms.WriteDataPtr, 
						ArgDataPtr->Rules.num_chars, 
						ArgDataPtr->Rules.chsize[jj],
						ArgDataPtr);

				// Pick the next Stop bit setting for this test.
			   for (int kk = 0; kk < 2; kk++) {
					if (ArgDataPtr->Rules.cstopb[kk] == -1) break;

					// Set up the stopbits in terms struct or DCB.
					Port.stop_bits(ArgDataPtr,ArgDataPtr->Rules.cstopb[kk]);

					for (int ll = 0; ll < 3; ll++) {
					if (ArgDataPtr->Rules.parodd[ll] == '\0') break;

						// Set up the parity in terms struct or DCB.
						Port.set_parity(ArgDataPtr,ArgDataPtr->Rules.parodd[ll]);

						//****************************************************
						// Using NUM_OPER in the ArgDataPtr->Rules file will 
						// allow maximum IO at each individual permutation of 
						// the ArgDataPtr->Rules without adding 
						// waits for setting up.  All the overhead in setting
						// up,in flushing unread data, and in syncronizing 
						// two processes is very expensive.  Using NUM_OPER 
						// allows the test to avoid these cost until an error 
						// occurs or it is time to change parameters.
						//****************************************************

	// NOW DO THE TEST.......

#ifdef BUGIT
						sprintf(ToHtx.GetMsgAddr(),"baud %d,Stopbits %d,"
						       " ChSize %d, parity....%c, num_oper=%d\n",
								ArgDataPtr->TestParms.BaudRate, 
								ArgDataPtr->TestParms.CStopB,
								ArgDataPtr->TestParms.ChSize,
								ArgDataPtr->TestParms.Parity,
								ArgDataPtr->Rules.num_oper);
						ToHtx.SendMsg(0,HTX_HE_INFO);                    
#endif

						// do sync before setting attributes of ports. 
						if(sync_ports(ArgDataPtr)) {
							sprintf(ToHtx.GetMsgAddr(),"sync_ports 3. failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}

					//	if(Port.Ltcsetattr(TCSAFLUSH, TC_TCSET1,ArgDataPtr)<0) goto abortexit;
						
	//change to TSCANOW to fix timeout of setattr on nighthawk native port
						if(Port.Ltcsetattr(TCSANOW, TC_TCSET1,ArgDataPtr)<0) {
							sprintf(ToHtx.GetMsgAddr(),"Port.Ltcsetattr failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}
						if(sync_ports(ArgDataPtr)) {
							sprintf(ToHtx.GetMsgAddr(),"sync_ports 4. failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}

                        // flush read write buffers.  Use member function.
                        // note: if the writer is still active when this
                        // flush occurs there may still be data in the buffers.
						// NOTE: do this before setting attributes to make sure
						// no data is left to be transmitted. 

                        if(Port.FlushBuffers())  {
							sprintf(ToHtx.GetMsgAddr(),"Port.FlushBuffers failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}

						if(sync_ports(ArgDataPtr)) {
							sprintf(ToHtx.GetMsgAddr(),"sync_ports 5. failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}

						// at this point I run the 
						//reader function....that does the test.

						if(Reader(ArgDataPtr)==BADBOY) {
							sprintf(ToHtx.GetMsgAddr(),"Reader failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}

						// now clean an left over read data. 
						Port.Clean_Read_Q(ArgDataPtr);

						// resync ports.. 
						// needed to make sure both
						// write threads are done.
						// before I close this port.
						if(sync_ports(ArgDataPtr)) {
							sprintf(ToHtx.GetMsgAddr(),"sync_ports 6. failed\n");
							ToHtx.SendMsg(0,HTX_HE_INFO);
							goto abortexit;
						}

						 ++ArgDataPtr->pass_count;   // increment pass count

					}               /* endfor [for (ll = 0; ll < 3; ll++)]    */
				}               /* endfor [for (kk = 0; kk < 2; kk++)]    */
			}                   /* endfor [for (jj = 0; jj < 5; jj++)]    */
		}                       /* endfor [for (ii = 0; ii < 13; ii++)]   */

#ifdef BUGIT
		sprintf(ToHtx.GetMsgAddr(),"Run Complete rc=0\n");
		ToHtx.SendMsg(0,HTX_HE_INFO);
#endif
      }         // End of while loop thru rule stanzas.           
   
      if(ArgDataPtr->BaudValid == 0) {
		(void) strcpy(ToHtx.GetMsgAddr(), "Can't do half-duplex on a wrap-plug."
		"There must be at least\n"
		"1 full-duplex baud specified on wrap plugs.\n"
		"Or you have speeds that are two fast for the device being tested\n");
		ToHtx.SendMsg(1, HTX_HE_HARD_ERROR);
		goto abortexit;
      }

   	  hxfupdate(FINISH, stats);

   } while (strcmp(stats->run_type, "REG") == 0 ||
	    strcmp(stats->run_type, "EMC") == 0 || loop_flag);

								
   RETURN(0);

abortexit:


	// note this will not be printed by SendMsg if ABORT flag is set 
	sprintf(ToHtx.GetMsgAddr(),"abortext entered\n");
	ToHtx.SendMsg(0,HTX_HE_INFO);

	if(!(ArgDataPtr->Flags&ABORT)) {
		sprintf(ToHtx.GetMsgAddr(),"Set Abort in hxeasy main.\n");
		ToHtx.SendMsg(0,HTX_HE_INFO);
		// set abort flag.
		ArgDataPtr->Flags|=ABORT;
	}

	// the problem here is port to port..... 
    // this main is talking to another processes writer......
	// the writer will stop but I will not return so as to
	// run dx's.... so this next code should fix that. 
	if(ArgDataPtr->TestParms.wrap_type != ePlug) {
		if(ArgDataPtr->sig_cnt==0) {	// no signals recieved  means the error
										// is within this process.
			// signal the other process..
			kill(atoi(ArgDataPtr->HisInfo.Pid),SIGTERM);
		}
	}
	

	// Tell write to stop.....
	WstopReader.SignalEvent();
	// signal write so it can see stop in case its waiting.
	WstartReader.SignalEvent();

	/* drain read q.. */
	Port.Clean_Read_Q(ArgDataPtr);

	// wait for write thread.. 
	if(ArgDataPtr->TestParms.wrap_type != ePlug) {
		// do this for port to port connection. 
		while(1) {
			// check the stop signal. The 0 time out says, check it
			// and retrun immediately.
			if((rc=WstopedReader.WaitEvent(0))==WAIT_OBJECT_0) {
				// if event is signaled go on and exit
				break;
			} else {
				if(rc != WAIT_TIMEOUT) break;	// error go on out
				else {
					// event is not signaled so check other process. 
					sprintf(cmdbuf,"IT=`ps -ef | grep \"%s\" | sed '/grep/d'`\n"
									"if [ \"$IT\" = \"\" ]\n"
									"then\n"
									"exit 0\n"
									"else\n"
									"exit 1\n"
									"fi", ArgDataPtr->HisInfo.Pid);

					// If you don't see the process the go on out
					// otherwise stay in this loop. 
					if(system(cmdbuf)==0) break;
				}
			}
			usleep(100000);   // wait a 100 ms before next try. 
		}
	} else {
		// do this for wrap plug. 
		if(WstopedReader.WaitEvent(COMM_TIMEOUT)==WAIT_TIMEOUT) {
			sprintf(ToHtx.GetMsgAddr(),"MAIN: Time out write not stopped\n"); 
			ToHtx.SendMsg(0,HTX_HE_SOFT_ERROR);
		}
	}

	RETURN(0);
}                       //      end of hxeasy MAIN.............

/**************************************************************/
//  Subroutines.......begin............Main ends..........
/**************************************************************/


void PrtSyntax(char * ProgramName)
{
   fprintf(stderr, "usage: %s /dev/XXXXX [REG EMC] rule_path\n", ProgramName);
   fprintf(stderr, "   or: %s /dev/XXXXX OTH rule_path\n", ProgramName);
}
	 
static int GetPath(     char path[], 
			const char pathin[], 
			int len, 
			const char envirVar[], 
			const char defaultPath[],
			ARGS_T *ArgDataPtr)
{  
   int    envirLen;
   int    inLen;
   int    defLen;
   char * Eptr;
   Cmsg *ToHtx=(Cmsg *)ArgDataPtr->MainPtr;

   if((inLen = (int)strlen(pathin)) > len) {
	strcpy(ToHtx->GetMsgAddr(), "GetPath: Path parameter is too long.\n");
	ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
	path[0] = '0';
	return 0; 
   }
   strcpy(path, pathin);
   if(CkPath(path, ToHtx->GetMsgAddr())) {
      return 1;
   }
   
   /* If pathin begins with '.' or '/', CkPath failed and we are through.     */
   if(pathin[0] == '.' || pathin[0] == SLASH_CHR) {
      // print error message formatted by CkPath funcion.
      ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
      path[0] = '0';
      return 0;
   }

   if((defLen = (int)strlen(defaultPath)) != 0) {
      if(defLen + inLen +2 > len) {
      sprintf(ToHtx->GetMsgAddr(), "GetPath: Total default path + name is longer than %d characters\n.", len);

      ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
       path[0] = '0';
       return 0;
    }
      strcpy(path, defaultPath);
      strcat(path, SLASH_TXT);
      strcat(path, pathin);
      if(CkPath(path, ToHtx->GetMsgAddr() + strlen(ToHtx->GetMsgAddr())))
	 return 1;
   }

   /* If environmental variable doesn't exist, Ckpath has failed.             */
   if((Eptr = getenv(envirVar)) == NULL) {
      // Append to message formatted by CkPath function.         
      sprintf(ToHtx->GetMsgAddr() + strlen(ToHtx->GetMsgAddr()), "Environmental variable %s doesn't exist or is not exported.\n", envirVar);
      ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
      path[0] = '0';
      return 0;
   }
   envirLen = (int)strlen(Eptr);
   if(envirLen + inLen +2 > len) {
      sprintf(ToHtx->GetMsgAddr(), "GetPath: Total environ path + name is longer than %d characters\n.", len);
      ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
      path[0] = '0';
      return 0;
   }
   strcpy(path, getenv(envirVar));
   strcat(path, SLASH_TXT);
   strcat(path, pathin);
   if(CkPath(path, ToHtx->GetMsgAddr() + strlen(ToHtx->GetMsgAddr())) == 0) {
      /* Print error message formatted by CkPath function.                    */
      ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
      path[0] = '0';
      return 0;
   }
   return 1;
}
static int CkPath(const char path[], char * err_msg_text)
{
   int des;

   if((des = open(path, O_RDONLY, 0)) == BADBOY) {
      /* Since this is a check, we won't print error message -- just make it  */
      /* available.                                                           */
      sprintf(err_msg_text, "CkPath: open error: \"%s%s\" - \n", 
	     (path[0] == '.' || path[0] == SLASH_CHR) ? "" : SLASH_TXT, path);
      return 0;
   } else {
      close(des);
      return 1;
   }
}


/*****  p a t _ t o _ b u f ( )  ********************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     pat_to_buf()                                         */
/*                                                                          */
/* DESCRIPTIVE NAME =  Copy pattern to buffer.                              */
/*                                                                          */
/* FUNCTION =          This function copies the specified pattern to the    */
/*                     specified buffer while masking the hi-order bits     */
/*                     according to the current character size.             */
/*                                                                          */
/* INPUT =             pattern - array which contains the desired pattern.  */
/*                     wbuf - array which is the buffer.                    */
/*                     num_chars - the number of characters to copy.        */
/*                     chsize - the current character size.                 */
/*                                                                          */
/* OUTPUT =            The buffer.                                          */
/*                                                                          */
/* NORMAL RETURN =     None - void function.                                */
/*                                                                          */
/* ERROR RETURNS =     None - void function.                                */
/*                                                                          */
/* EXTERNAL REFERENCES = None.                                              */
/*                                                                          */
/****************************************************************************/

void pat_to_buf(char *pattern, char *wbuf, int num_chars, int chsize,ARGS_T *ArgDataPtr)
{
   int                  cq=0;
   unsigned char                 mask;

   switch (chsize) {
   case 7:
      mask = 0x7f;
      break;
   case 6:
      mask = 0x3f;
      break;
   case 5:
      mask = 0x1f;
      break;
   default:
      mask = (unsigned char) 0xff;
      break;
   }                            /* endswitch */

    // check to see if I need to replace the ctrl s or q chars in the
    // pattern file. digi fixed driver take out -- the block of VNEXT char 0x16.
    if(ArgDataPtr->Rules.ixon=='Y' || ArgDataPtr->Rules.ixoff=='Y' ||
       ArgDataPtr->Rules.ixon=='y' || ArgDataPtr->Rules.ixoff=='y' ||
		ArgDataPtr->device_type == evcon) {

            // replace chars in pattern 0x11 and 0x13  and 0x16 with 0x17 
           for (cq = 0; cq < num_chars; cq++) {
              wbuf[cq] = (pattern[cq] & (char) mask);
              if( (wbuf[cq]==0x11) || (wbuf[cq]==0x13))
                wbuf[cq]=(0x00 & (char) mask);
           }

	} else {
		// Don't worry about replacing the start and stop chars in pattern
	   for (cq = 0; cq < num_chars; cq++) {
		  wbuf[cq] = pattern[cq] & (char) mask;
		}
	}

   return;
}                               /* pat_to_buf() */

void sig_handler(int sig)
{
	/*The void classes must be type cast first. alone with ToHtx   */

	Cmsg *ToHtx=(Cmsg *)ArgDataPtr->MainPtr;

//#ifdef ALARM_S
	if(sig==SIGALRM) {
		signal(sig,sig_handler);
		return;
	}
//#endif
	if(sig==SIGHUP) {
		sprintf(ToHtx->GetMsgAddr(),"SIGHUP received, send ABORT\n");
		ToHtx->SendMsg(0,HTX_HE_INFO);
		ArgDataPtr->Flags |= ABORT;
		return;
	}
#ifndef __HTX_LINUX__
	if(sig==SIGDANGER) {
		sprintf(ToHtx->GetMsgAddr(),"SIGDANGER received, increase paging space.\n");
		ToHtx->SendMsg(0,HTX_HE_HARD_ERROR);
		return;
	}
#endif

	++ArgDataPtr->sig_cnt;


	sprintf(ToHtx->GetMsgAddr(),"%d signal received. Going down\n",sig);
	ToHtx->SendMsg(0,HTX_HE_INFO);

	// this do for both aix and nt. 

	// get pointers to all the objects I need to destruct.
	// heres the list:
	Ccom *Port=(Ccom *)ArgDataPtr->Port;
	CcreateEvent *WstartReader=(CcreateEvent *)ArgDataPtr->WstartReader;
	CcreateEvent *WstopReader=(CcreateEvent *)ArgDataPtr->WstopReader;
	/*
	CcreateEvent *WstartWriter=(CcreateEvent *)ArgDataPtr->WstartWriter;
	CcreateEvent *OpenEventS=(CcreateEvent *)ArgDataPtr->OpenEventS;
	CcreateEvent *OpenEventC=(CcreateEvent *)ArgDataPtr->OpenEventC;
	CcreateEvent *WstopWriter=(CcreateEvent *)ArgDataPtr->WstopWriter;
	CcreateEvent *WstopedReader=(CcreateEvent *)ArgDataPtr->WstopedReader;
	CcreateEvent *WstopedWriter=(CcreateEvent *)ArgDataPtr->WstopedWriter;
	CArgData *global_data=(CArgData *)ArgDataPtr->global_data;
	CGetSpace *Space_Class=(CGetSpace *)ArgDataPtr->Space_Class;
	*/
	// setting the abort will take the write thread down on wrap test
	// and exit will kill thread otherwise so don't delete thread class.
	// it causes error in nt when tread goes before cmsg dx runs in the
	// thread. The sleep after abort is required to give write thread
	// time to die.
	ArgDataPtr->Flags|=ABORT;

	// Don't try to use these if they have not been set up
	if(WstopReader != NULL) {
			// tell the writer to stop
		WstopReader->SignalEvent();	
			// tap write so it can see stop in case its waiting.
		WstartReader->SignalEvent();
			// clean read pipe...
		// Port->Clean_Read_Q(ArgDataPtr);
			// make sure writer has stopped.
		// WstopedReader->WaitEvent(COMM_TIMEOUT);
	}

/* Take out this closing stuff it seems to be hanging up sometimes.
	// Now call destructors for class objects in reverse order they wer
	// declared. NOTE: if they have 0 pointer(they don't exits) delete
	// handles that and just returns. 
	delete WstopedWriter;	// take down event
	delete WstopedReader;	// take down event
	delete WstopWriter;	// take down event
	delete WstopReader;	// take down event
	delete WstartWriter;	// take down event
	delete WstartReader;	// take down event
	delete OpenEventC;	// take down event
	delete OpenEventS;	// take down event
	sleep(4);
	delete Space_Class;	// take down test data storage
	delete Port;	    // take down the port class.  
	delete ArgDataPtr->MainPtr;	// take down htx com storage  
	delete ArgDataPtr->BaseMsgObjectPtr;	// take down htx com storage base 
	delete global_data;	// take down ArgData storage  
*/
	delete Port;	    // take down the port class.  

	// now you can just exit. 
	exit(0);

}
int sync_ports(ARGS_T *ArgDataPtr) 
{
	if(ArgDataPtr->Flags&ABORT) {
		return(BADBOY);
	}

	if(ArgDataPtr->TestParms.wrap_type != ePlug     ) {
		Cmsg *ToHtx=(Cmsg *)ArgDataPtr->MainPtr;
		CcreateEvent *OpenEventC=(CcreateEvent *)ArgDataPtr->OpenEventC;
		CcreateEvent *OpenEventS=(CcreateEvent *)ArgDataPtr->OpenEventS;

		if(ArgDataPtr->Flags&SERVER) {

			// server taps client. 
			OpenEventC->SignalEvent();

			// now server waits for client ..and resets event 
			if(OpenEventS->WaitEvent(COMM_TIMEOUT)==
				WAIT_TIMEOUT) {
				sprintf(ToHtx->GetMsgAddr(),"OPEN: Time out"
					" waiting for %s to open \n",
					ArgDataPtr->HisInfo.DeviceName);
				ToHtx->SendMsg(0,HTX_HE_INFO);
				return(BADBOY);
			}

		} else {

			// client waits for server ..and resets event 
			if(OpenEventC->WaitEvent(COMM_TIMEOUT)==
				WAIT_TIMEOUT) {
				sprintf(ToHtx->GetMsgAddr(),"OPEN: Time out"
					" waiting for %s to open \n",
					ArgDataPtr->HisInfo.DeviceName);
				ToHtx->SendMsg(0,HTX_HE_INFO);
				return(BADBOY);
			}

			// now client taps the server event.
			OpenEventS->SignalEvent();

		}
	} // NOTE: for wrap plug I don't sync the open, no need.

	return(0);
}
