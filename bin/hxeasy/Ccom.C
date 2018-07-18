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

//   FUNCTIONS: ClearComm
//		ClosePort
//		ConnectPorts
//		GetDcbPtr
//		GetProp
//		GetState
//      Quick_Sync
//		Ltcsetattr
//		OpenPort
//		SetHW
//		SetPortDefaults
//		SetTimeOuts
//		atoi
//		for
//		if
//		odm_get_obj
//		ready_ports
//		set_vmin


/*****************************************************************************/
/*                                                                           */
/* This file has class routines for the Ccom class. It handles all access to */
/* the com ports or tty ports for nt and aix.                                */
/*                                                                           */
/*****************************************************************************/
#ifndef __HTX_LINUX__
#include <stream.h>
#include <fstream.h>
#include <iostream.h>
#include "headers.h"
#else
#include <headers.h>
#endif

#define HIBAUD_SET 1
#define HIBAUD_RESET 0

//***********************************************************************
//		CONSTRUCTOR for AIX
//***********************************************************************
Ccom::Ccom(char *device,ARGS_T *ArgDataPtr)
{
	// this name in aix is /dev/ttyxx
	strcpy(PortNamePtr,device);
	ArgPtr=ArgDataPtr;
}

//***********************************************************************
//		DESTRUCTOR for AIX 
//***********************************************************************
Ccom::~Ccom()
{
	ClosePort(ArgPtr);
}

int Ccom::Fcntl(int command)
{
         return(fcntl(async_des, command, 0));
}

enum atype Ccom::GetDevType()	
{
/*
 * Return default device type for Linux
 */ 	
#ifndef __HTX_LINUX__
   struct CuDv CuDev, CuParent;
   char criteria[25];
   int rc, mode;             // mode 0 is rs232 mode 1 is rs422
   CLASS_SYMBOL cusdev;

   rc = odm_initialize();
   
   cusdev = odm_open_class(CuDv_CLASS);
   sprintf(criteria, "name='%s'", &PortNamePtr[5]);

   rc = (int)odm_get_obj(cusdev, criteria, (char *)&CuDev, ODM_FIRST);

   // This is broken into "Device Class / Device Subclass / Device Type" 
   // PdCn class contains subclasses that can be connected to a device    
   // in the connkey field.  
   //  See page B-9 of "Writing a Device Driver for AIX Version 3.2"    

   /* Device type for parent of tty devices is as follows:
      cxma   -  16c232        old 128port rans.. frio(isa) and mockernut(mca).
	  cxpa0  -  16e422        enh 128port 422 rans  trinity (pci)
	  cxpa0  -  16e232        enh 128port 232 rans  trinity (pci)
      64p    -  64p232
      16p    -  16p232
      16p    -  16p422
       8p    -  8p232
       8p    -  8p422
       8p    -  8p188
      native -  s1a / s2a                                   
     8p isa  -  pcxr            creek
	 8p isa 232/422-pc8s        san gabriel
	 8p pci 232/422 -  pci8r    medina  */

	if(strstr(CuDev.PdDvLn_Lvalue, "rs422") != (char *)NULL)
		mode=1;
	else mode = 0;
/* odmget -q"name=ttyX" CuDv */
/* get parent and do */
/* odmget -q"name=parent" CuDv */
/* type is in PdDvLn_Lvalue of parent*/

   sprintf(criteria, "name='%s'", CuDev.parent);
   rc = (int)odm_get_obj(cusdev, criteria, (char *)&CuParent, ODM_FIRST);
   odm_close_class(cusdev);
   if(rc== -1) {
      return eAtUnknown;
   }
   if(strstr(CuParent.PdDvLn_Lvalue, "16c232") != (char *)NULL)
      return e128p;			// mockernut, frio and trinity old rans. 232
   if(strstr(CuParent.PdDvLn_Lvalue, "16e232") != (char *)NULL)
      return e128_232;			// trinity new rans. 232 higher speed
   if(strstr(CuParent.PdDvLn_Lvalue, "16e422") != (char *)NULL)
      return e128_422;			// trinity new rans. 422 higher speed
   else if(strstr(CuParent.PdDvLn_Lvalue, "64p232") != (char *)NULL)
      return e64p;
   else if(strstr(CuParent.PdDvLn_Lvalue, "64p422") != (char *)NULL)
      return e64p;
   else if(strstr(CuParent.PdDvLn_Lvalue, "16p232") != (char *)NULL)
      return e16p;
   else if(strstr(CuParent.PdDvLn_Lvalue, "16p422") != (char *)NULL)
      return e16p422;
   else if(strstr(CuParent.PdDvLn_Lvalue, "8p232") != (char *)NULL)
      return e8p;
   else if(strstr(CuParent.PdDvLn_Lvalue, "8p188") != (char *)NULL)
      return e8p;
   else if(strstr(CuParent.PdDvLn_Lvalue, "8p422") != (char *)NULL)
      return e8p422;
   else if(strstr(CuParent.PdDvLn_Lvalue, "s1a") != (char *)NULL)
      return eNative;
   else if(strstr(CuParent.PdDvLn_Lvalue, "s2a") != (char *)NULL)
      return eNative;
   else if(strstr(CuParent.PdDvLn_Lvalue, "pnp") != (char *)NULL)
      return eNative;
   else if(strstr(CuParent.PdDvLn_Lvalue, "s3a_3") != (char *)NULL)
      return eNative;
   else if(strstr(CuParent.PdDvLn_Lvalue, "ibm731") != (char *)NULL)
      return e731;
   else if(strstr(CuParent.PdDvLn_Lvalue, "pcxr") != (char *)NULL)
      return e8isa;
   else if(strstr(CuParent.PdDvLn_Lvalue, "4f111100") != (char *)NULL)
		if(mode) return e8med422;    // for 422 mode medina 
		else return e8med;           // for 232 mode medina
   else if(strstr(CuParent.PdDvLn_Lvalue, "4f11c800") != (char *)NULL)
		if(mode) return ejas422;    // for 422 mode jasmine 
		else return ejas;           // for 232 mode jasmine
   else if(strstr(CuParent.PdDvLn_Lvalue, "cts3_16") != (char *)NULL)
		if(mode) return ects3_16422;    // no  422 mode network ran 
		else return ects3_16;           // for 232 mode network ran 16 port
   else if(strstr(CuParent.PdDvLn_Lvalue, "pc8s") != (char *)NULL)
		if(mode) return e8422;    // different from e8p422
		else return e8isa;
   else
      return eAtUnknown;
#else
    // TWM change for hibicus
   	// deal with linux types..
	// so far  jasmine ejas_linux
	// and     hibiscus portserver   ects3_16_linux
	// ****** I create all the hibiscus ttys with id of "rX" where X=0-7
	// so /dev/ttyr000-ttyr015 is for portserver 0
	// and /dev/ttyr100-ttyr115 is for portserver 1  etc..
	// /proc/dgrp/ports/rX  is a file with a list of ports 
	// for now I will just assume ttyr[0-9][0-9][0-9]  is hibiscus.
	if(strstr(PortNamePtr, "ttyr") != (char *)NULL)
		return ects3_16_linux;         // for 232 mode network ran 16 port
	else if(strstr(PortNamePtr, "ttyn") != (char *)NULL)
		return ejas_linux;         // for jasmine
	else
      return eAtUnknown;
#endif
}

int Ccom::SetHW(ARGS_T *ArgDataPtr,char flag)	
{
	int rc=0;
	switch (ArgDataPtr->device_type) {
	  case e128p:
	  case e8isa:
	  case e8422:
	  case e128_232:
	  case e128_422:
	  case e8med:
	  case e8med422:
	  case ects3_16:
	  case ects3_16422:
		 rc=Set128p(ArgDataPtr,flag); 
		 break;
	  case ejas:
	  case ects3_16_linux:
	  case ejas_linux:
	  case ejas422:
	  case e64p:
	  case e16p:
	  case e16p422:
	  case  e8p:
	  case  e8p422:
	  case eNative:
	  case eAtUnknown:
		 break;
	  case e731:
		 rc=Set731p(ArgDataPtr);   
		 break;
	}

   return(rc);
}

void Ccom::set_vmin(int chars_to_read,ARGS_T *ArgDataPtr)
{
   if(terms.c_cc[VMIN] != chars_to_read) {
      terms.c_cc[VMIN] = chars_to_read;
      (void) Ltcsetattr(TCSANOW, TC_VMIN,ArgDataPtr);
   }
}

int Ccom::SetPortDefaults( ARGS_T *ArgDataPtr, char clocal_flag)
{
    int rc;
	int retry=0;

	while(1) {
	   rc = tcgetattr(async_des, &terms);
	   if (rc == -1) {
			if(errno==EAGAIN) {
				if(++retry < 20) {
					sleep(1);
					continue;
				}
			}
			Cmsg ToHtx(ArgDataPtr); 
		  (void) sprintf(ToHtx.GetMsgAddr(),
				 "tcgetattr error - %s\n", strerror(errno));
		  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
		  return(BADBOY);
	   } else break; 
	}
	retry=0;
/********************************************************************/
/* terms.c_cc[4] and terms.c_cc[5] are the MIN characters and Time  */
/* out values for a read request.  These values must be set to      */
/* prevent a hang on a read operation.  See discussion of ICANON    */
/* in the c_lflag under termios special file in AIX Technical       */
/* Reference Manual                                                 */
/********************************************************************/
	terms.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | IEXTEN | NOFLSH | TOSTOP | ISIG | XCASE | FLUSHO | PENDIN | ECHOPRT | ECHOKE | ECHOCTL);

	terms.c_cc[VMIN] = 1;
	terms.c_cc[VTIME] = 0;
	//terms.c_cc[VLNEXT] = 0x00;	// disables VNEXT PROCESSING

	// NOTE: I am not setting VSTART and VSTOP
  	//  terms.c_cc[VSTART] = -1;
  	//  terms.c_cc[VSTOP] = -1;

/*
 * Set input/output processing flags
 */

	// use software flow on connection, and hardware control. 
	// NOTE: you can use software flow control here only if you make
	//       sure you send no 0x11 or 0x13 chars in the connect data. 
	//       So if you have to send numbers for tty number or pid, 
	//       convert them to char then send them.
	terms.c_iflag |= (INPCK | IGNBRK | PARMRK | IXOFF | IXON);

// note: I don't use tcsendbreak to sync opens so I don't need IGNBRK off
// and BRKINT on. 

	terms.c_iflag &= ~(ICRNL | BRKINT | IGNCR | IGNPAR | INLCR | ISTRIP | IUCLC | IXANY | IMAXBEL);

	// ignore all output options..... 
	terms.c_oflag &= ~(OPOST);   // must be off or tabs (0x08) are replaced
								// with blanks and miscompare time.


	terms.c_cflag |=  (HUPCL | CREAD |(clocal_flag == 'Y' ? CLOCAL : 0) );
	terms.c_cflag &= ~((clocal_flag == 'N' ? CLOCAL : 0) | PAREXT);



	if(baud_rate(ArgDataPtr,9600)) return(BADBOY);
	char_size(ArgDataPtr,8);
	stop_bits(ArgDataPtr,1);
	set_parity(ArgDataPtr,'N');

	// need rts flow control set 
	set_flow(ArgDataPtr);

//	if(Ltcsetattr(TCSAFLUSH, TC_DEFAULTS,ArgDataPtr)) return(BADBOY);
// change to next line 8/3/00 TWM. So that tcsetattr would not hang
// if wrap plug was missing or port bad..
	if(Ltcsetattr(TCSANOW, TC_DEFAULTS,ArgDataPtr)) return(BADBOY);

	return(0);
}


int Ccom::Ltcsetattr(int method, int id, ARGS_T *ArgDataPtr)
{
	int             rc;
	int             rc2;
	struct termios  act_term;
	int retry=0;
 
	errno = 0;			/* clear errno                               */

	while(1) {
	    rc = tcsetattr(async_des, method, &terms);
		if(rc== -1 && errno==EAGAIN) {
			if(++retry < 20) {
				sleep(1);
				continue;
			}
		}
		else break; 
	}
	retry=0;

	while(1) {
	    rc2 = tcgetattr(async_des, &act_term);
		if(rc2== -1 && errno==EAGAIN) {
			if(++retry < 20) {
				sleep(1);
				continue;
			}
		}
		else break; 
	}
	retry=0;

#ifndef __HTX_LINUX__
	// Do this to make up for change to aix in 9616, They changed some
	// compliance stuff and now you won't get the upper bits of c_cflag
	// back any more.
	terms.c_cflag&=0x00FFF;
#endif


   if (rc == -1  || rc2 == -1  ||
	act_term.c_iflag != terms.c_iflag  ||
	act_term.c_oflag != terms.c_oflag  ||
/* 	act_term.c_cflag != terms.c_cflag  ||  */
	act_term.c_lflag != terms.c_lflag  ) {
/********************************************/
/* Format error message and call hxfupdat  */
/* to process error.  Then, return a bad    */
/* exit code.                               */
/********************************************/
	  Cmsg ToHtx(ArgDataPtr);
      (void) sprintf(ToHtx.GetMsgAddr(), "tcsetattr %d error - %s\n",
      					id, strerror(errno));
      if(rc == -1 || rc2 == -1)
        (void) sprintf(ToHtx.GetMsgAddr() + strlen(ToHtx.GetMsgAddr()),
                "tcsetattr rc=%d, tcgetattr rc=%d\n", rc, rc2);

      (void) sprintf(ToHtx.GetMsgAddr() + strlen(ToHtx.GetMsgAddr()),
          "Set attributes c_iflag=%X, c_oflag=%X,  c_cflag=%X, c_lflag=%X\n",
          terms.c_iflag, terms.c_oflag, terms.c_cflag, terms.c_lflag); 

      (void) sprintf(ToHtx.GetMsgAddr() + strlen(ToHtx.GetMsgAddr()),
          "Act attributes c_iflag=%X, c_oflag=%X, c_cflag=%X, c_lflag=%X\n",
          act_term.c_iflag, act_term.c_oflag, act_term.c_cflag, act_term.c_lflag);
	  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
      return(BADBOY);
   }

	return(0);
}

int Ccom::sethibaud(int rate,ARGS_T *ArgDataPtr)
{

	/* the rate will be - if half duplex is requested, just make it */
	/* positive and then set hibaud accordingly */
	/*
	 * NO NEED TO SET HIBAUD IN LINUX JUST USEING CFSETOSPEED
	 */

#ifndef __HTX_LINUX__
	rate=abs(rate);
	if(ArgDataPtr->device_type==e731) {
		int status;
		int action_attempted;
		struct cs_termiox termiox;
		struct cs_termiox vtermiox;

		if(rate>38400 ) {
				/* Set hibaud */
				termiox.x_sflag = CS_HIBAUD;
				if ((status = dev_ioctl(CS_TCBISX, (void *)&termiox,
						  sizeof(termiox),"CS_TCBISX = Setting hibaud",ArgDataPtr)) < 0) {
					Cmsg ToHtx(ArgDataPtr);
					(void) sprintf(ToHtx.GetMsgAddr(), "Could not set hibaud; rc=%d, errno=%s\n", status,strerror(errno));
					ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
					return(BADBOY);
				}
				action_attempted=HIBAUD_SET;
		}
		else {
				/* Turn off hibaud */
				termiox.x_sflag = CS_HIBAUD;
				if ((status = dev_ioctl(CS_TCBICX, (void *)&termiox, 
					sizeof(termiox),"CS_TCBICX - Clearing hibaud",ArgDataPtr)) < 0) {
					Cmsg ToHtx(ArgDataPtr);
					  (void) sprintf(ToHtx.GetMsgAddr(), "Could not clear hibaud; rc=%d, 					errno=%s\n",status, strerror(errno)); 
					  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
					  return(BADBOY);
				}
				action_attempted=HIBAUD_RESET;
		}


		/* now show the hibaud setting to verify its right */
			if ((status = dev_ioctl(CS_TCGETX, (void *)&vtermiox, sizeof(vtermiox), 
			"CS_TCGETX - Verify: getting current hibaud setting",ArgDataPtr)) < 0) {
					Cmsg ToHtx(ArgDataPtr);
				  (void) sprintf(ToHtx.GetMsgAddr(), "Verify: Could not get hibaud; rc=%d, errno = %s\n", status, strerror(errno)); 
				  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
				  return(BADBOY);
			}

		switch(action_attempted) {
			case HIBAUD_RESET:
				/* we were turning off hibaud; hibaud should not be set */
				if(vtermiox.x_sflag & CS_HIBAUD) {
					Cmsg ToHtx(ArgDataPtr);
				  (void) sprintf(ToHtx.GetMsgAddr(), "Verify: hibaud not turned off\n"); 
				  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
				  return(BADBOY);
				}
			break;
			case HIBAUD_SET:
				/* we were turning hibaud on; hibaud should be set */
				if(!(vtermiox.x_sflag & CS_HIBAUD)) {
					Cmsg ToHtx(ArgDataPtr);
				  (void) sprintf(ToHtx.GetMsgAddr(), "Verify: hibaud not turned on\n"); 
				  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
				  return(BADBOY);
				}
			break;
			default:
					Cmsg ToHtx(ArgDataPtr);
				  (void) sprintf(ToHtx.GetMsgAddr(), "bad action_attempted %d, in sethibaud()\n",action_attempted); 
				  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
				  return(BADBOY);
		}
	} else if (ArgDataPtr->device_type==e128_232 ||
                ArgDataPtr->device_type==e128_422 ||
                ArgDataPtr->device_type==e128p ||
                ArgDataPtr->device_type==e8med ||
                ArgDataPtr->device_type==e8med422 ||
                ArgDataPtr->device_type==e8422 ||
                ArgDataPtr->device_type==ects3_16 ||
                ArgDataPtr->device_type==ects3_16422 ||
                ArgDataPtr->device_type==e8isa) {

		struct cxma_struct cxma;

		// first get the current state of the cxma structure.....
		if(ioctl(async_des,CXMA_GETA, &cxma)<0) {
			Cmsg ToHtx(ArgDataPtr);
			(void) strcpy(ToHtx.GetMsgAddr(), "sethibaud: CMXA_GETA failed 8port isa.\n");
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		}

		if(rate>38400) cxma.cxma_flags |= CXMA_FAST;		// turn on hibaud
		else cxma.cxma_flags &= ~CXMA_FAST;				// turn off hibaud

		if(ioctl(async_des,CXMA_SETA, &cxma)<0) {
			Cmsg ToHtx(ArgDataPtr);
			(void) strcpy(ToHtx.GetMsgAddr(), "sethibaud: CMXA_SETA failed 8port isa.\n");
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		}

	} else if (ArgDataPtr->device_type==ejas ||
                ArgDataPtr->device_type==ejas422) {
		if(rate>38400)  {
			/*use TXGBAUD to get current baud rate..*/
			if(ioctl(async_des, TXSBAUD, &rate) < 0) {
				Cmsg ToHtx(ArgDataPtr);
				(void) sprintf(ToHtx.GetMsgAddr(), "sethibaud: TXSBAUD ioctl failed to set rate %d.\n", rate); 
				ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			} 
		}
	} 
#endif

	return(0);
}  


int Ccom::dev_ioctl( int cmd,
					void *ptr,
					unsigned size,
					char *msg,
					ARGS_T *ArgDataPtr)
{
        int rc = 0;
	#ifndef __HTX_LINUX__
        struct strioctl strioctl;

        strioctl.ic_cmd = cmd;
        strioctl.ic_timout = 0;
        strioctl.ic_dp = (char *)ptr;
        strioctl.ic_len = size;
        rc = ioctl(async_des, I_STR, &strioctl);
        if (rc == -1) {
			Cmsg ToHtx(ArgDataPtr);
			  (void) sprintf(ToHtx.GetMsgAddr(),"Ioctl failed : %s\n",msg); 
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
        }
	#endif
        return(rc);
}
 
int Ccom::Set731p(ARGS_T *ArgDataPtr)
{

	#ifndef __HTX_LINUX__
        struct cs_termiox termiox;
        struct cs_termiox vtermiox;
        int status;
		struct rule_format *r = &ArgDataPtr->Rules;


        /* Get current termiox settings */
        if ((status = dev_ioctl(CS_TCGETX, (void *)&termiox,
          sizeof(termiox), (char *) "CS_TCGETX - Getting current slew rate",ArgDataPtr)) < 0) {
			Cmsg ToHtx(ArgDataPtr);
			  (void) sprintf(ToHtx.GetMsgAddr(), "Could not get slew rate; rc=%d, errno=%s\n",status, strerror(errno)); 
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
              return(BADBOY);
        }

        /* Clear existing slew settings */
        termiox.x_sflag &= ~CS_SLEW;
        /* Set new slew rate */
        if(strcmp(r->slewrate, "DEFAULT") == 0)
                termiox.x_sflag |= CS_SLEW_MEDIUM;
        else if(strcmp(r->slewrate, "SLOW") == 0)
                termiox.x_sflag |= CS_SLEW_SLOW;
        else if(strcmp(r->slewrate, "MEDIUM") == 0)
                termiox.x_sflag |= CS_SLEW_MEDIUM;
        else if(strcmp(r->slewrate, "FAST") == 0)
                termiox.x_sflag |= CS_SLEW_FAST;
        else if(strcmp(r->slewrate, "SUPER") == 0)
                termiox.x_sflag |= CS_SLEW_SUPER;
        else
		{
			Cmsg ToHtx(ArgDataPtr);
			  (void) sprintf(ToHtx.GetMsgAddr(), "Invalid slew rate %s\n",
				r->slewrate); 
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			  return(-1);
		}
        if ((status = dev_ioctl(CS_TCSETXW,(void *)&termiox,
          sizeof(termiox),(char *) "CS_TCSETXW - Setting slew rate",ArgDataPtr)) < 0) {
			Cmsg ToHtx(ArgDataPtr);
			  (void) sprintf(ToHtx.GetMsgAddr(),"Could not set slew rate %s; rc=%d, errno=%s\n",r->slewrate,status,strerror(errno)); 
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			  return(BADBOY);
        }

        /* Verify the slew rate got set properly */
        if ((status = dev_ioctl(CS_TCGETX, (void *)&vtermiox,
          sizeof(vtermiox), (char *) "CS_TCGETX - Verify: getting slew rate",ArgDataPtr)) < 0) {
			Cmsg ToHtx(ArgDataPtr);
			  (void) sprintf(ToHtx.GetMsgAddr(), "Verify: Could not get slew rate; rc=%d, errno=%s\n",status,strerror(errno)); 
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			  return(BADBOY);
        }

        if((termiox.x_sflag & CS_SLEW) != (vtermiox.x_sflag & CS_SLEW)) {
			Cmsg ToHtx(ArgDataPtr);
			  (void) sprintf(ToHtx.GetMsgAddr(), "Verify: Slew Rate not set properly; wanted %x, got %x\n", termiox.x_sflag,vtermiox.x_sflag); 
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			  return(BADBOY);
        }
	#endif
	return(0);
}


int Ccom::Set128p(ARGS_T *ArgDataPtr,char flag)
{
	//This is not needed for linux..
#ifndef __HTX_LINUX__
	struct rule_format *r = &ArgDataPtr->Rules;
	int retry=0;
   	struct cxma_struct cxma;
   	struct cxma_struct act_cxma;
	

	// first get the current state of the cxma structure.....
	while(1) {
		if(ioctl(async_des,CXMA_GETA, &cxma)<0) {
			if(errno==EAGAIN) {
				if(retry < 20) {
					sleep(1);
					continue;
				}
			}
			Cmsg ToHtx(ArgDataPtr);
			(void) sprintf(ToHtx.GetMsgAddr(), "1: CMXA_GETA failed. errno=%d\n",errno); 
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		} else break; 
	}
	retry=0;

	// now set up the flags based on the rules file. 
	// I want fastcook off during connect so the write data won't
	// bypass the ioctls and cause problems. 

	if(r->fastcook && !(flag&SET_FOR_CONNECT) ) {
	    cxma.cxma_flags |= (CXMA_COOK);
	} else {
		cxma.cxma_flags &= ~(CXMA_COOK);
	}


	if(r->altpin) {
	  cxma.cxma_flags |= (CXMA_ALTPIN);
	} else {
	  cxma.cxma_flags &= ~(CXMA_ALTPIN);
	}


	// make sure were not forcing carrier detect. if in 232 mode.
	// if in 422 mode force carrier detect.
	
	if(ArgDataPtr->device_type == e8422 ||
		ArgDataPtr->device_type == ects3_16 ||
		ArgDataPtr->device_type == ects3_16422 ||
		ArgDataPtr->device_type == e8med422 ||
		ArgDataPtr->device_type == e128_422 ) {
		// force carrier detect in 422 mode. 
		// note if you use hardware switch I won't know what mode is. 
		cxma.cxma_flags |= (CXMA_FORCEDCD);
	}
	else cxma.cxma_flags &= ~(CXMA_FORCEDCD); 

	// make sure fast baud setting is off. 
	cxma.cxma_flags &= ~(CXMA_FAST);

/**************************************************/
/* We will turn these off and set on the stack.   */
/* Exception is dtrpace, it is needed to ck DTR   */
/* since DTR line disp only uses dcdpace          */
/**************************************************/
	cxma.cxma_flags &= ~RTSPACE;
	cxma.cxma_flags &= ~CTSPACE;
	cxma.cxma_flags &= ~DCDPACE;
   	if(r->dtrpace)
      	cxma.cxma_flags |= (DTRPACE);
   	else
      	cxma.cxma_flags &= ~DTRPACE;

	while(1) {
		if(ioctl(async_des,CXMA_SETA, &cxma)<0) {
            if(errno==EAGAIN) {
                if(retry < 20) {
                    sleep(1);
                    continue;
                }
            }
			Cmsg ToHtx(ArgDataPtr);
			(void) strcpy(ToHtx.GetMsgAddr(), "CMXA_SETA failed.\n");
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		} else break;
	}
	retry=0;
  
	// Now get the act values to make sure their set. 
	while(1) {
		if(ioctl(async_des,CXMA_GETA, &act_cxma)<0) {
			if(errno==EAGAIN) {
				if(retry < 20) {
					sleep(1);
					continue;
				}
			}
			Cmsg ToHtx(ArgDataPtr);
			(void) strcpy(ToHtx.GetMsgAddr(), "2: CMXA_GETA failed.\n");
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		} else break;
	}


	// compare the two structs: 
	if(memcmp(&act_cxma,&cxma,sizeof(struct cxma_struct)) != 0) {
		Cmsg ToHtx(ArgDataPtr);
		(void) strcpy(ToHtx.GetMsgAddr(), "cxma struct values not set!\n");
		ToHtx.SendMsg(1,HTX_HE_INFO);
	}
#endif
	return(0);
} 

//**********************************************************************
// Class routines 
//**********************************************************************

void Ccom::char_size(ARGS_T *ArgDataPtr,int i)
{
   	ArgDataPtr->TestParms.ChSize=i;
   terms.c_cflag &= ~CSIZE;
   switch (i) {
   case -1:
      break;
   case 5:
      terms.c_cflag |= CS5;
      break;
   case 6:
      terms.c_cflag |= CS6;
      break;
   case 7:
      terms.c_cflag |= CS7;
      break;
   case 8:
      terms.c_cflag |= CS8;
      break;
   default:
      break;
   }				/* endswitch */
   return;
}				/* char_size() */


int  Ccom::baud_rate(ARGS_T *ArgDataPtr,int i)
{
	  
	ArgDataPtr->TestParms.BaudRate=i;

	if(ArgDataPtr->TestParms.wrap_type==ePlug &&
		(ArgDataPtr->device_type==e8p ||
		ArgDataPtr->device_type==e64p ||
		ArgDataPtr->device_type==e8p422 ||
		ArgDataPtr->device_type==e16p422 ||
		ArgDataPtr->device_type==e16p ||
		ArgDataPtr->device_type==eAtUnknown ||
		ArgDataPtr->device_type==eNative ) ) {
		// limit the bufsize. to prevent overflows on wrap plugs
		if(ArgDataPtr->Rules.bufsize>16) {
			ArgDataPtr->Rules.bufsize=16;
		}
	}
	// in linux these high speeds can be set.
//B57600
//B115200
//B230400
//B460800

//twm
//	Cmsg ToHtx(ArgDataPtr); 
   switch (i) {
   case -1:
      break;
   case 0:
      (void) cfsetispeed(&terms, B0);
      (void) cfsetospeed(&terms, B0);
      break;
   case 50:
      (void) cfsetispeed(&terms, B50);
      (void) cfsetospeed(&terms, B50);
      break;
   case 75:
      (void) cfsetispeed(&terms, B75);
      (void) cfsetospeed(&terms, B75);
      break;
   case 110:
      (void) cfsetispeed(&terms, B110);
      (void) cfsetospeed(&terms, B110);
      break;
   case 134:
      (void) cfsetispeed(&terms, B134);
      (void) cfsetospeed(&terms, B134);
      break;
   case 150:
      (void) cfsetispeed(&terms, B150);
      (void) cfsetospeed(&terms, B150);
      break;
   case 200:
      (void) cfsetispeed(&terms, B200);
      (void) cfsetospeed(&terms, B200);
      break;
   case 300:
      (void) cfsetispeed(&terms, B300);
      (void) cfsetospeed(&terms, B300);
      break;
   case 600:
      (void) cfsetispeed(&terms, B600);
      (void) cfsetospeed(&terms, B600);
      break;
   case 1200:
      (void) cfsetispeed(&terms, B1200);
      (void) cfsetospeed(&terms, B1200);
      break;
   case 1800:
      (void) cfsetispeed(&terms, B1800);
      (void) cfsetospeed(&terms, B1800);
      break;
   case 2400:
      (void) cfsetispeed(&terms, B2400);
      (void) cfsetospeed(&terms, B2400);
      break;
   case 4800:
      (void) cfsetispeed(&terms, B4800);
      (void) cfsetospeed(&terms, B4800);
      break;
   case 9600:
      (void) cfsetispeed(&terms, B9600);
      (void) cfsetospeed(&terms, B9600);
//twm
//		(void) sprintf(ToHtx.GetMsgAddr(),"9600 set."); 
//		ToHtx.SendMsg(1,HTX_HE_INFO);
      break;
   case 19200:
      (void) cfsetispeed(&terms, B19200);
      (void) cfsetospeed(&terms, B19200);
      break;
   case 38400:
      (void) cfsetispeed(&terms, B38400);
      (void) cfsetospeed(&terms, B38400);
//twm
//		(void) sprintf(ToHtx.GetMsgAddr(),"38400 set."); 
//		ToHtx.SendMsg(1,HTX_HE_INFO);
      break;
   case 57600:
#ifdef __HTX_LINUX__ 
		(void) cfsetispeed(&terms, B57600);
		(void) cfsetospeed(&terms, B57600);
#else
	  	if(ArgDataPtr->device_type==e731) {		
			(void) cfsetispeed(&terms, B2400);
			(void) cfsetospeed(&terms, B2400);
		} else if (ArgDataPtr->device_type==e128_232 || 
				ArgDataPtr->device_type==e128_422 ||
				ArgDataPtr->device_type==e128p ||
				ArgDataPtr->device_type==e8med ||
				ArgDataPtr->device_type==e8med422 ||
				ArgDataPtr->device_type==e8422 ||
				ArgDataPtr->device_type==ects3_16 ||
				ArgDataPtr->device_type==ects3_16422 ||
				ArgDataPtr->device_type==e8isa) {
			(void) cfsetispeed(&terms, B50);
			(void) cfsetospeed(&terms, B50);
		} else if (ArgDataPtr->device_type==ejas || 
				ArgDataPtr->device_type==ejas422) {
		} else {
			Cmsg ToHtx(ArgDataPtr); 
			(void) sprintf(ToHtx.GetMsgAddr(),"Invalid device_type %d for use with"
				" higher baud rates..57600",ArgDataPtr->device_type); 
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		}
#endif
      break;
   case 115200:
#ifdef __HTX_LINUX__ 
		(void) cfsetispeed(&terms, B115200);
		(void) cfsetospeed(&terms, B115200);
#else
	  	if(ArgDataPtr->device_type==e731) {		
			(void) cfsetispeed(&terms, B19200);
			(void) cfsetospeed(&terms, B19200);
		} 
        else if (ArgDataPtr->device_type==e128_232 ||
                ArgDataPtr->device_type==e128_422 ||
				ArgDataPtr->device_type==e128p ||
                ArgDataPtr->device_type==e8med ||
                ArgDataPtr->device_type==e8med422 ||
                ArgDataPtr->device_type==e8422 ||
                ArgDataPtr->device_type==ects3_16 ||
                ArgDataPtr->device_type==ects3_16422 ||
                ArgDataPtr->device_type==e8isa) {

			(void) cfsetispeed(&terms, B110);
			(void) cfsetospeed(&terms, B110);
		} else if (ArgDataPtr->device_type==ejas || 
				ArgDataPtr->device_type==ejas422) {
		} else {
			Cmsg ToHtx(ArgDataPtr); 
			(void) sprintf(ToHtx.GetMsgAddr(),"Invalid device_type %d for use with"
				" higher baud rates..115200",ArgDataPtr->device_type); 
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		}
#endif
      break;
   case 230000:
   case 230400:
#ifdef __HTX_LINUX__ 
		(void) cfsetispeed(&terms, B230400);
		(void) cfsetospeed(&terms, B230400);
//twm
//		(void) sprintf(ToHtx.GetMsgAddr(),"230400 set."); 
//		ToHtx.SendMsg(1,HTX_HE_INFO);
#else
        if (ArgDataPtr->device_type==e128_232 ||
                ArgDataPtr->device_type==e128_422 ||
				ArgDataPtr->device_type==e128p ||
                ArgDataPtr->device_type==e8med ||
                ArgDataPtr->device_type==ects3_16 ||
                ArgDataPtr->device_type==ects3_16422 ||
                ArgDataPtr->device_type==e8med422) {

			(void) cfsetispeed(&terms, B200);
			(void) cfsetospeed(&terms, B200);
		} else if (ArgDataPtr->device_type==ejas || 
				ArgDataPtr->device_type==ejas422) {
		} else {
			Cmsg ToHtx(ArgDataPtr); 
			(void) sprintf(ToHtx.GetMsgAddr(),"Invalid device_type %d for use with"
				" higher baud rates..230000",ArgDataPtr->device_type); 
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		}
#endif
      break;
   case 460800:
#ifdef __HTX_LINUX__ 
		(void) cfsetispeed(&terms, B460800);
		(void) cfsetospeed(&terms, B460800);
#else
		if (ArgDataPtr->device_type==ejas || 
				ArgDataPtr->device_type==ejas422) {
		} else if (ArgDataPtr->device_type==ects3_16 ||
				ArgDataPtr->device_type==ects3_16422) {

			(void) cfsetispeed(&terms, B300);
			(void) cfsetospeed(&terms, B300);
		} else {
			Cmsg ToHtx(ArgDataPtr); 
			(void) sprintf(ToHtx.GetMsgAddr(),"Invalid device_type %d for use with"
				" higher baud rates..460800",ArgDataPtr->device_type); 
			ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			return(BADBOY);
		}
#endif
      break;
   default:
      break;
   }				/* endswitch */

#ifndef __HTX_LINUX__ 
	/* hibuad must be turned on if i>38400 and off is its less. So it must be
	 * called for every baud rate ICALL.
	 * NOT needed for linux.
	*/
	if(sethibaud(i,ArgDataPtr)) return(BADBOY);
#endif

   return(0);
}				/* baud_rate() */


void Ccom::stop_bits(ARGS_T *ArgDataPtr,int i)
{
	// Set up this test parm in ArgData struct.
	ArgDataPtr->TestParms.CStopB=i;

	terms.c_cflag &= ~CSTOPB;
	switch (i) {
		case 2:
		  terms.c_cflag |= CSTOPB;
		  break;
		default:
		  break;
	}				/* endswitch */
	return;
}				/* stop_bits() */



void  Ccom::set_parity(ARGS_T *ArgDataPtr,char i)
{

	ArgDataPtr->TestParms.Parity=i;       

	terms.c_cflag &= ~(PARENB | PARODD);
	terms.c_iflag &= ~INPCK;
	switch (i) {
		case 'O':
		  terms.c_cflag |= (PARENB | PARODD);
		  terms.c_iflag |= INPCK;
		  break;
		case 'E':
		  terms.c_cflag |= PARENB;
		  terms.c_iflag |= INPCK;
		  break;
		default:
		  break;
	}				/* endswitch */
	return;
}				/* set_parity() */

int Ccom::set_flow(ARGS_T *ArgDataPtr)
{
    /* this routine sets up flow control */
/*
 * struct termiox is still not available in Linux, so use termios CRTSCTS it...
 * Once it becomes available(probably in 2.6 kernel), hash-if-defs can be taken out
 * For now: if device is hibiscus I will use the ditty to set flow  but its done * in set hibaud.
 */
#ifdef __HTX_LINUX__
	//In linux set hardware flow..  only rts, not dtr in linux.
    if(ArgDataPtr->Rules.rts &&
		!(ArgDataPtr->Flags&WRITE_ONLY))  {
		terms.c_cflag |= CRTSCTS;
    }

    if(ArgDataPtr->Rules.dtr &&
		!(ArgDataPtr->Flags&WRITE_ONLY)) {
		terms.c_cflag |= CRTSCTS;
    }
    /* now check the case where neither is desired...... */
    if((!ArgDataPtr->Rules.dtr && !ArgDataPtr->Rules.rts) ||
		ArgDataPtr->Flags&WRITE_ONLY ) {
        /* no hardware flow control at all     */
		terms.c_cflag&=~CRTSCTS;
    }
	//NOTE: Ltcsetattr is always done after set_flow runs so terms will get set. 
#else

    struct termiox flow;

	int retry=0;
	int a_retry=0;

	/* first get current settings */
	while(1) {
		if(ioctl(async_des,TCGETX,&flow) == -1) {
			if(errno==EAGAIN) {
				if(++a_retry<10) {
					sleep(1);
					continue;
				} else {
					Cmsg ToHtx(ArgDataPtr);
					(void) sprintf(ToHtx.GetMsgAddr(), 
					"EAGAIN getting termiox data retry 10- \n");
					ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
					return(BADBOY);
				}
			} else {
				if(++retry<5) {
					sleep(1);
					continue;
				} else {
					Cmsg ToHtx(ArgDataPtr);
					(void) sprintf(ToHtx.GetMsgAddr(), 
					"Error getting termiox data - %s.\n",strerror(errno));
					ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
					return(BADBOY);
				}
			}
		} else break;
	}

   /*******************************************************************/
   /* Put rts or dtr on  NOTE xon not handled here..      */
   /* That is handled by termios c_iflag XON values......      */
	/* You should be able to use soft and hard flow control at the same time */
   /*******************************************************************/

	/* 422 mode does not have hardware flow control , use software control */
	if( ArgDataPtr->device_type==e8422 || 
		ArgDataPtr->device_type==e16p422 || 
		ArgDataPtr->device_type==e8med422 || 
		ArgDataPtr->device_type==ejas422 || 
		ArgDataPtr->device_type==e128_422 || 
		ArgDataPtr->device_type==e8p422) {
		// turn off hardware flow control.
  		ArgDataPtr->Rules.rts=0;
  		ArgDataPtr->Rules.dtr=0;
	}

    if(ArgDataPtr->Rules.rts &&
		!(ArgDataPtr->Flags&WRITE_ONLY))  {
        flow.x_hflag&=~DTRXOFF;    /* make sure DTR is off */
        flow.x_hflag|=RTSXOFF;     /* turn RTS on........  */
    }

    if(ArgDataPtr->Rules.dtr &&
		!(ArgDataPtr->Flags&WRITE_ONLY)) {
        flow.x_hflag&=~RTSXOFF;    /* make sure RTS is off */
        flow.x_hflag|=DTRXOFF;     /* turn DTR on........  */
    }
    /* now check the case where neither is desired...... */
    if((!ArgDataPtr->Rules.dtr && !ArgDataPtr->Rules.rts) ||
		ArgDataPtr->Flags&WRITE_ONLY ) {
        /* no hardware flow control at all     */
        flow.x_hflag&=~RTSXOFF;    /* make sure RTS is off */
        flow.x_hflag&=~DTRXOFF;    /* make sure DTR is off */
        flow.x_hflag&=~CTSXON;     /* make sure CTS is off */
    }
	else {
    	// I want flow control to be bi directional CTSXON will always be set 
	    flow.x_hflag|=CTSXON;
	}

    if(ioctl(async_des,TCSETX,&flow)<0) {
		 Cmsg ToHtx(ArgDataPtr);
      (void) sprintf(ToHtx.GetMsgAddr(), "Error setting termiox data - %s.\n", 
		strerror(errno));
	  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
      return(BADBOY);
    }


   if(ArgDataPtr->Rules.ihog != 0 && 
		ioctl(async_des, TXSETIHOG, &ArgDataPtr->Rules.ihog) == -1) {
		Cmsg ToHtx(ArgDataPtr);
        (void) sprintf(ToHtx.GetMsgAddr(), 
		"Error setting ihog limit to %d - %s.\n", ArgDataPtr->Rules.ihog, 
		strerror(errno));
	    ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
        return(BADBOY);
   }
#endif
	return(0);

}

int Ccom::ready_port(ARGS_T *ArgDataPtr)
{
	// for 422 mode adapters force software flow control 
    if( ArgDataPtr->device_type==e8422 ||
        ArgDataPtr->device_type==e16p422 ||
        ArgDataPtr->device_type==e8med422 ||
        ArgDataPtr->device_type==ejas422 ||
        ArgDataPtr->device_type==e128_422 ||
        ArgDataPtr->device_type==e8p422) {

		ArgDataPtr->Rules.ixon = 'Y';
		ArgDataPtr->Rules.ixoff = 'Y';
	}

	// NOTE: in aix we don't have to open the ports, it stays open. 
	if (ArgDataPtr->Rules.ign_brk == 'Y') {
		terms.c_iflag |= IGNBRK;
	} else {
		terms.c_iflag &= ~IGNBRK;
	}          /* endif */
	if (ArgDataPtr->Rules.hupcl != 'N') {
		terms.c_cflag |= HUPCL;
	} else {
		terms.c_cflag &= ~HUPCL;
	}          /* endif */

	if(ArgDataPtr->Flags&WRITE_ONLY) {
		terms.c_cflag |= CLOCAL;
	} else {
		if (ArgDataPtr->Rules.clocal == 'Y') {
			terms.c_cflag |= CLOCAL;
		} else {
			terms.c_cflag &= ~CLOCAL;
		}         
	}

	if (ArgDataPtr->Rules.brk_int == 'Y') {
		terms.c_iflag |= BRKINT;
	} else {
		terms.c_iflag &= ~BRKINT;
	}          /* endif */
	if (ArgDataPtr->Rules.ign_par == 'Y') {
		terms.c_iflag |= IGNPAR;
	} else {
		terms.c_iflag &= ~IGNPAR;
	}          /* endif */
	if (ArgDataPtr->Rules.par_mark == 'Y') {
		terms.c_iflag |= PARMRK;
	} else {
		terms.c_iflag &= ~PARMRK;
	}          /* endif */
	/* set software control on 422 devices......... */
	if (ArgDataPtr->Rules.ixon == 'Y' &&
		!(ArgDataPtr->Flags&WRITE_ONLY)) {
		terms.c_iflag |= IXON;
	} else {
		terms.c_iflag &= ~IXON;
	}          /* endif */
	if (ArgDataPtr->Rules.ixoff == 'Y'&&
		!(ArgDataPtr->Flags&WRITE_ONLY)) {
		terms.c_iflag |= IXOFF;
	} else {
		terms.c_iflag &= ~IXOFF;
	}          /* endif */
/****************************************************/
/* turn on the CREAD flag, but leave everything     */
/* else the same.                                   */
/****************************************************/
    terms.c_cflag |= CREAD;

	// set flow control for this pass. 
	if(set_flow(ArgDataPtr)) return(BADBOY);

// now set the terms values in tty subsystem........
	// if(Ltcsetattr(TCSAFLUSH, TC_TCSET1,ArgDataPtr)) return(BADBOY);
	if(Ltcsetattr(TCSANOW, TC_TCSET1,ArgDataPtr)) return(BADBOY);

    if(SetHW(ArgDataPtr,SET_FOR_TEST)) {
		return(BADBOY);
	}

	return(0);
}             // end ready_ports().........



int Ccom::ConnectPorts(ARGS_T *ArgDataPtr)
{
	int BytesWritten=0;
	int BytesRead=0;
	Cmsg *ToHtx=(Cmsg *)ArgDataPtr->MainPtr;
	int BytesToWrite= (int) sizeof(INFO_T);
	int TotalRead=0;
	int q;      // used as for loop counter.
	char * WriteFrom=(char *)&ArgDataPtr->MyInfo;
	char ReadTo[(4*sizeof(INFO_T))];
	char *rdptr=ReadTo;
	int index=0;
	int trynext=0;
	struct htx_data *stats=ToHtx->GetTempStatsPtr();

	int his_tty;
	int rc;
	int semval;
	int sem_gone;
	
    // Now I need to put the info packet together;
    // first 0 the packet....
    memset(&ArgDataPtr->MyInfo,0x00,sizeof(INFO_T));

	strcpy(ArgDataPtr->MyInfo.DeviceName,PortNamePtr);
    sprintf(ArgDataPtr->MyInfo.Pid,"%d",getpid());
	DWORD len = HXEASY_HOSTNAMELEN;
    gethostname(ArgDataPtr->MyInfo.HostName,len);

	// set startmark character in info packet.
	strcpy(ArgDataPtr->MyInfo.startmark,"+++++");

   hxfupdate(UPDATE,stats);

   ArgDataPtr->device_type = GetDevType();

#ifdef DEBUG_LINUX
      (void) sprintf(ToHtx->GetMsgAddr(), " TCIOFLUSH |%x|, devtype %d....name %s\n",TCIOFLUSH,ArgDataPtr->device_type,ArgDataPtr->MyInfo.DeviceName);
      ToHtx->SendMsg(1,HTX_HE_INFO);
#endif

   if(ArgDataPtr->device_type == e731)
                rc=OpenPort(O_NONBLOCK,ArgDataPtr);
   else {
				// The first open must be non blocking.. for some
				// reason 128ports won't raise CD on the first open. 
	            rc=OpenPort( O_NONBLOCK | O_NOCTTY,ArgDataPtr);
	}

   if(rc == -1) {
      (void) sprintf(ToHtx->GetMsgAddr(), "Unable to open port. HW config phase.\n");
      ToHtx->SendMsg(1,HTX_HE_HARD_ERROR);
      return(BADBOY);
  
   }
   if(ArgDataPtr->device_type == e731) {
		/* this next code is needed to get the cns tty in the right state */
		/* to avoid data loss.       */
		/* turn off O_NONBLOCK */
		if(Fcntl(F_SETFL)) {
			  (void) sprintf(ToHtx->GetMsgAddr(),"Unable to turn off O_NONBLOCK\n");
			  ToHtx->SendMsg(1,HTX_HE_HARD_ERROR);
			  return(BADBOY);
		}
   }

	// I need to get clocal set here......
	if(SetPortDefaults(ArgDataPtr, 'Y')) return(BADBOY);
	
	// use SET_CONNECT flag to tell code not to set fastcook on. 
   if(SetHW(ArgDataPtr, SET_FOR_CONNECT)) {   
		(void) sprintf(ToHtx->GetMsgAddr(),"SetHW failed.....\n");
		ToHtx->SendMsg(1,HTX_HE_HARD_ERROR);
		return(BADBOY);
	}

	// I need to sync the port to port processes here. I use quick_sync
	// to find out if other side is set up and to find out his tty number.
	// Then I create an event to use for syncing between opens and closes. 

	if(!(ArgDataPtr->Flags & WRITE_ONLY)) {
		his_tty=Quick_Sync(ArgDataPtr);
		if(his_tty == BADBOY) {
			return(BADBOY);
		}
	} else {
		ArgDataPtr->TestParms.wrap_type=ePlug;
		ArgDataPtr->Flags |= SERVER;
	}

	// to sync with only one event I find out if I am client or server
	// as defined in quick_sync. Then:
	//     if client set event to 1 and wait for it to be 0
	//     if server set event to 0 and wait for it to be 1
	// Using this I can guarentee that the quick sync connects on both 
	// sides are done before I go on to close the port.    

	if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
		// This will be created only if were not on wrap plug. 
		// and it will be removed when I leave this current scope. 

		CcreateEvent    Winit(FALSE,FALSE,ArgDataPtr->InitS,ArgDataPtr);
		if(ArgDataPtr->Flags&ABORT) return(BADBOY); 
		char wc[16];

		sem_gone=0;
		memset(wc,0x00,16);
		strcpy((char *)wc,"T");
		strcat((char *)wc,ArgDataPtr->MyInfo.DeviceName+DEVNAME_START);

		if(ArgDataPtr->Flags&CLIENT) {
			Winit.SignalEvent();    // set event to 0
		} else {
			Winit.SetEvent();    // set event to 1
		}
		// now wait for other process.	
		while(1) {
			if(ArgDataPtr->Flags&ABORT) return(BADBOY);
			Clean_Read_Q(ArgDataPtr);
			usleep(300000);
			WriteComm(16,(char *)&wc[0], ArgDataPtr);
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
	} else {
		// clean quick_sync data for wrap or port to port. 
		// to insure that the port will not hang on close...
		if(!(ArgDataPtr->Flags & WRITE_ONLY))
			Clean_Read_Q(ArgDataPtr);     
	}

	// at this point I know both processes are here...ready to close. 
/********************************************************************/
/* At least in the case of the 128 port devices.  A combination of  */
/* 8 pin cables and starting up in 10 pin configuration will hang   */
/* one of the ports after changing to altpin(8 pin).  Closing the   */
/* device fixes this.                                               */
/********************************************************************/

   if(ArgDataPtr->device_type!=e731 ) {
         // We don't close for the e731 because there is NO synchronization
         // of the two ports at this point.  If port A is ahead of port B
         // and has already done the second open, it will be vulnerable
         // to the SIGHUP generated when port B does the following close.
         // To eliminate the HANGUP problem, we avoid the close and reopen
         // code.  Other device types should probably do the same, however,
         // it appears that the 128 port product risks hung ports unless
         // the devices are closed.

    	ClosePort(ArgDataPtr);

		// I need a sync here between the close and open. use the
		// Init event. if this is not wrap plug. 
		if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
			// This will be created only if were not on wrap plug. 
			// and it will be removed when I leave this current scope. 
			CcreateEvent    Winit(FALSE,FALSE,ArgDataPtr->InitS,ArgDataPtr);
			if(ArgDataPtr->Flags&ABORT) return(BADBOY); 

			sem_gone=0;

			if(ArgDataPtr->Flags&CLIENT) {
				Winit.SignalEvent();    // set event to 0
			} else {
				Winit.SetEvent();    // set event to 1
			}
			// now wait for other process.	
			while(1) {
				usleep(300000);
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

		// for a reason I don't understand if you take out this 1 sec
		// sec sleep, wrap connections will detect drop in CD and you get
		// SIGHUP signal that will kill you. 
		sleep(1);

		if(ArgDataPtr->Flags&WRITE_ONLY) {
			if(OpenPort( O_NONBLOCK ,ArgDataPtr)) return(BADBOY);
		} else {
			if(OpenPort( 0 ,ArgDataPtr)) return(BADBOY);
		}

		// This must be done after each open..... for now this is the last
		// open of the test so the tty's are set up for the duration..
		// set CLOCAL for this first close so terminal servers will run
		// port to port
		if(SetPortDefaults(ArgDataPtr, 'Y')) return(BADBOY);
    }

	// this sync is needed to give both sides time to setup defaults so
	// that chars written during connect are interpreted as data and 
	// not as control chars. 

	// use the Init Event.... again. 
	if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
		// This will be created only if were not on wrap plug. 
		// and it will be removed when I leave this current scope. 
		CcreateEvent    Winit(FALSE,FALSE,ArgDataPtr->InitS,ArgDataPtr);
		if(ArgDataPtr->Flags&ABORT) return(BADBOY); 

		sem_gone=0;

		if(ArgDataPtr->Flags&CLIENT) {
			Winit.SignalEvent();    // set event to 0
		} else {
			Winit.SetEvent();    // set event to 1
		}
		// now wait for other process.	
		while(1) {
			usleep(300000);
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

	// now do the formal connection and get data needed for the 
	// other events to be created later in hxeasy+.C
	// NOTE: This is the only connect needed in NT, since I only have
	//       to do one open in NT there is no sync problem with port to port. 

	// Put in update call here to make sure hxt does not think
	// I'm hung..
	hxfupdate(UPDATE,stats);

// set up pointers for connect write and read. 
	// set up to write packet again.
	WriteFrom=(char *)&ArgDataPtr->MyInfo;
	BytesToWrite=sizeof(INFO_T);
	rdptr=ReadTo;
	TotalRead=0;
	index=0;

	if(!(ArgDataPtr->Flags & WRITE_ONLY)) {

		// 0 the read memory..To make sure its clean. 
		memset(rdptr,0x00,(3*sizeof(INFO_T)));

		// make sure pipe is clean before I start connect stuff here. 
		FlushBuffers();

		while( TotalRead < (int)(3*sizeof(INFO_T)) ) {

			if(ArgDataPtr->Flags&ABORT) return(BADBOY);

			// Now write the info packet.
			if((BytesWritten=WriteComm(     BytesToWrite,
					WriteFrom,
					ArgDataPtr)) < 0) return(BADBOY);
			WriteFrom+=BytesWritten;
			BytesToWrite-=BytesWritten;
			if(BytesToWrite<=0) {
				// set up to write packet again.
				WriteFrom=(char *)&ArgDataPtr->MyInfo;
				BytesToWrite=sizeof(INFO_T);
			}

			// these sleeps are used to keep data amounts down if one side
			// happens to be behind or gets lost in the process table for
			// a while on very busy systems. 
			usleep(300000);

			// Now read his info packet wait till you get it no overlap.
			do {
				if(ArgDataPtr->Flags&ABORT) return(BADBOY);

				if((BytesRead=ReadComm(sizeof(INFO_T),
					rdptr, 
					FLUSH_TIMEOUT,         // aix and linux wait for one sec.
					ArgDataPtr)) == BADBOY) 
				return(BADBOY);

				rdptr+=BytesRead;
				TotalRead+=BytesRead;

			} while(BytesRead && (TotalRead < (int)(3*sizeof(INFO_T))) );
		}

		// set up to write packet again.
		WriteFrom=(char *)&ArgDataPtr->MyInfo;
		BytesToWrite=sizeof(INFO_T);

		// I can use the Init event to sync the completion of connect. 

		if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
			// This will be created only if were not on wrap plug. 
			// and it will be removed when I leave this current scope. 
			CcreateEvent    Winit(FALSE,FALSE,ArgDataPtr->InitS,ArgDataPtr);
			if(ArgDataPtr->Flags&ABORT) return(BADBOY); 

			sem_gone=0;

			if(ArgDataPtr->Flags&CLIENT) {
				Winit.SignalEvent();    // set event to 0
			} else {
				Winit.SetEvent();    // set event to 1
			}
			// now wait for other process.	
			while(1) {
				if(ArgDataPtr->Flags&ABORT) return(BADBOY);
				usleep(500000);
				if((BytesWritten=WriteComm(     BytesToWrite,
						WriteFrom,
						ArgDataPtr)) < 0) return(BADBOY);

				WriteFrom+=BytesWritten;
				BytesToWrite-=BytesWritten;
				if(BytesToWrite<=0) {
					// set up to write packet again.
					WriteFrom=(char *)&ArgDataPtr->MyInfo;
					BytesToWrite=sizeof(INFO_T);
				}
				Clean_Read_Q(ArgDataPtr);
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
		// note If I am on wrap plug I don't need to send more data I already
		// have the connect data from myself. 

		// Now find the start of the message and pull it out.
		// now put the info data into the His Info struct.
		// scan thru the ReadTo buffer find "+++++" for start of message.

		// find the start of the message packet 4 plus signs.
		while(1) {
			while(ReadTo[index++]!='+') {	 // find first +
				if(index > (int)(4*sizeof(INFO_T))) {
					sprintf(ToHtx->GetMsgAddr(),"Could not find valid packet\n");
					ToHtx->SendMsg(0,HTX_HE_INFO);
					for( q=0; q<(int)(3*sizeof(INFO_T)); q++) {
						// change nulls to @ signs. So they will print. 
						if(ReadTo[q]==0x00) ReadTo[q]=0x40;  
					}
					ReadTo[q+1]=0x00;
					sprintf(ToHtx->GetMsgAddr(),"%s",ReadTo);
					ToHtx->SendMsg(0,HTX_HE_SOFT_ERROR);
					return(BADBOY);
				}
			}
			// look for 4 more plus signs
			for (q=index; q<(index+4); q++) {
				if(ReadTo[q]!='+') {
					trynext=1;
					break;
				}
			}
			if(trynext) {
				trynext=0;
				index=index+6;  // move on to next copy of the message	
				continue;
			}
			index=index-1;
			memcpy( (char *)&ArgDataPtr->HisInfo,
					(char *)&ReadTo[index],sizeof(INFO_T));
			break;
		}
	} // end if its not WRITE_ONLY run..... 
	else {		// if it is WRITE_ONLY
		memcpy((char *)&ArgDataPtr->HisInfo,(char *)&ArgDataPtr->MyInfo,
				sizeof(INFO_T));
		/* turn off O_NONBLOCK , so the writes will block */
		if(Fcntl(F_SETFL)) {
		  (void) sprintf(ToHtx->GetMsgAddr(),"Unable to turn off O_NONBLOCK\n");
		  ToHtx->SendMsg(1,HTX_HE_HARD_ERROR);
		  return(BADBOY);
		}
	}

    // This section puts together the connection info needed by the rest
	// of the test code.

	// pull off mine and his port numbers
	char Mynum[6];
	char Hisnum[6];
	int	start;
	start=DEVNAME_START;
	strcpy(Mynum,ArgDataPtr->MyInfo.DeviceName + start);
	strcpy(Hisnum,ArgDataPtr->HisInfo.DeviceName + start);
	ArgDataPtr->my_ttynum = atoi(Mynum);
	ArgDataPtr->his_ttynum = atoi(Hisnum);
	char serverport[6];
	char clientport[6];

	// Now that I have both com Port numbers I can build the Unique     
	// names to be used for the events that synchronize the threads.  
	// I use 6 events for this synchronization. Thier names are saved
	// in the ArgData structure so they will be available to all the
	// threads for event creation.
	// 1) OpenS.... These event is used in port to port mode to synchronize
	// 2) OpenC     the opens of the two ports. Each time thru a test the
	//              ports are opened at the start of the rule and closed
	//              at the end and each time the opens must be synchronized.
	//              One is used by the server (OpenS) the other by the client.
	//              Their names will be.....
	//              EOpenS_serverport#
	//              EOpenC_serverport#
	//              
    // 3) WstartS.. These events are used By the read and write threads to
    // 4) WstartC   control the writes activities. In wrap mode only the
    //              WstartS event is used....Since the write threads now
    //              runs all the time I need some way to tell the write to
    //              begin the next test and start writting.
    //
    // 5) WstopS..  These events are used By the read and write threads to
    // 6) WstopC    control the writes activities. In wrap mode only the
    //              WstartS event is used....Since the write threads now
    //              runs all the time, the read needs a way to tell the       
    //              write to stop. 
    //
    // 7) WstopedS. These events are used By the read and write threads to
    // 8) WstopedC  control the writes activities. In wrap mode only the
    //              WstopedS event is used....Since the write threads now
    //              runs all the time I need some way to tell the read that
    //              the write is done. 
    //

	// There are two cases.. 
	// 1) test all on one host.
	//    In this case the server is determined by the larger of the
	//    2 port numbers.
	//
	// 2) The ports are on different hosts....
	//    In this case the server is determined by doing a strcmp of the
	//    hostnames and the larger os the two is the server.

	// Put the base names of the events in the ArgData string variables.
	// this first set cris cross from port to port.

	// NOTE: THESE NAMES MUST BE 6 CHARS LONG.....EXACTLY....!!!!!!!!!!
	//       then create event code depends on these being 6 chars. 
	strcpy(ArgDataPtr->OpenS,"EOpenS");
	strcpy(ArgDataPtr->OpenC,"EOpenC");
	strcpy(ArgDataPtr->WstartS,"EstrtS");
	strcpy(ArgDataPtr->WstartC,"EstrtC");
	strcpy(ArgDataPtr->WstopS,"EstopS");
	strcpy(ArgDataPtr->WstopC,"EstopC");
	strcpy(ArgDataPtr->WstopedS,"EstpdS");
	strcpy(ArgDataPtr->WstopedC,"EstpdC");

	// I assume I have a good connect packet when I get to here. 
    if(!strcmp((char *)&ArgDataPtr->MyInfo.HostName,
		(char *)&ArgDataPtr->HisInfo.HostName))  {

		if(ArgDataPtr->my_ttynum>ArgDataPtr->his_ttynum) {
			strcpy(serverport,Mynum);
			strcpy(clientport,Hisnum);
		}
		else {
			strcpy(clientport,Mynum);
			strcpy(serverport,Hisnum);
		}

		// hostnames equal , other Port on same host or wrapped one port.
#ifdef _DEBUG_LINUX_
		printf("my_ttynum:%d and his_ttynum:%d\n",ArgDataPtr->my_ttynum, ArgDataPtr->his_ttynum);
#endif
		if(ArgDataPtr->my_ttynum==ArgDataPtr->his_ttynum) {
			// wrap plug on this Port.
			ArgDataPtr->TestParms.wrap_type = ePlug;
			// I don't have to establish a pipe to another machine for
			// synchronization. And I will have only 1 read thread and
			// one write thread.. and of course the compare thread when
			// test starts. So set up the Event names for the sync events.

			// set myself up as server since I am the only process.
			ArgDataPtr->Flags |= SERVER;

			// I only need  WstartS
			// and DataReady events for this test. The serverport number
			// will be my port.
			strcat(ArgDataPtr->WstartS,serverport);
			strcat(ArgDataPtr->WstopS,serverport);
			strcat(ArgDataPtr->WstopedS,serverport);
		} else {   // the ports differ there are two Ports on the
			   // same machine. I have to decide who's the
			   // client and who's the server and set the 
			   // event names.
            if(ArgDataPtr->my_ttynum > ArgDataPtr->his_ttynum) {
                ArgDataPtr->Flags |= SERVER;
            }
            else {
                ArgDataPtr->Flags |= CLIENT;
            }

			// these event names are same for client or server.
			strcat(ArgDataPtr->WstartS,serverport);
			strcat(ArgDataPtr->WstartC,serverport);
			strcat(ArgDataPtr->WstopS,serverport);
			strcat(ArgDataPtr->WstopC,serverport);
			strcat(ArgDataPtr->WstopedS,serverport);
			strcat(ArgDataPtr->WstopedC,serverport);
			strcat(ArgDataPtr->OpenS,serverport);
			strcat(ArgDataPtr->OpenC,serverport);

			// mark this a port to port run.. set eCable.
			ArgDataPtr->TestParms.wrap_type=eCable;
		}

	} else {
		sprintf(ToHtx->GetMsgAddr(), "Bad connect data!\n");
		ToHtx->SendMsg(0, HTX_HE_HARD_ERROR);
/*  don't do this unless for debug  
				for( q=0; q<(3*sizeof(INFO_T)); q++) {
					// change nulls to @ signs. So they will print. 
					if(ReadTo[q]==0x00) ReadTo[q]=0x40;  
				}
				ReadTo[q+1]=0x00;
				sprintf(ToHtx->GetMsgAddr(),"%s\n",ReadTo);
				ToHtx->SendMsg(0,HTX_HE_INFO);

				sprintf(ToHtx->GetMsgAddr(),"myhostname %s, his hostname %s\n",
				(char *)&ArgDataPtr->MyInfo.HostName,
				(char *)&ArgDataPtr->HisInfo.HostName);
				ToHtx->SendMsg(0,HTX_HE_INFO);

				sprintf(ToHtx->GetMsgAddr(),"myttynum %s, %d his tty %s, %d\n",
				(char *)&ArgDataPtr->MyInfo.DeviceName,
				ArgDataPtr->my_ttynum,
				(char *)&ArgDataPtr->HisInfo.DeviceName,
				ArgDataPtr->his_ttynum);

				sprintf(ToHtx->GetMsgAddr(),"my PID %s, %d his PID %s, %d\n",
				(char *)&ArgDataPtr->MyInfo.Pid,
				atoi(ArgDataPtr->MyInfo.Pid),
				(char *)&ArgDataPtr->HisInfo.Pid,
				atoi(ArgDataPtr->HisInfo.Pid));

				ToHtx->SendMsg(0,HTX_HE_INFO);
				*/
		return(BADBOY);
	}
#ifdef _DEBUG_LINUX_
	printf("\e[5m\e[35mGot Connected!!!!\n"); /*print it bold in RED*/
	printf("\e[0m");
#endif
	// this logic is used to determine who is the writer in HALP_DUPLEX mode.
	if(ArgDataPtr->Flags & SERVER) {
			ArgDataPtr->MyInfo.writer=0x01;
			ArgDataPtr->HisInfo.writer=0x00;
	} else {
			ArgDataPtr->MyInfo.writer=0x00;
			ArgDataPtr->HisInfo.writer=0x01;
	}

	// clean out any read data left over..... 
    Clean_Read_Q(ArgDataPtr);

	// Set up the htx menu # 2 Device_desc location for my device.
	char tmp_buf[16];
    memset(tmp_buf,0x00,16);
#if 0
	if( (stats->p_shm_HE != (struct htxshm_HE *)NULL) && 
		(strcmp(stats->run_type,"OTH") != 0) )  { 

		if(ArgDataPtr->TestParms.wrap_type==ePlug) {
			strncpy(tmp_buf,"wrap_plug",9);
			strncpy(stats->p_shm_HE->device_desc, tmp_buf,16);
		} else {        // we are Port to Port.
			sprintf(tmp_buf,"to_%s", ArgDataPtr->HisInfo.DeviceName+5);
			// Modify device description in shared memory table 
			strncpy(stats->p_shm_HE->device_desc, tmp_buf,16);
		}
	} else {
		sprintf(ToHtx->GetMsgAddr(), "stats->p_shm_H was null, or run_type"
		" OTH!\n");
		ToHtx->SendMsg(0, HTX_HE_INFO);
	}
#endif
	return(0);
}           // End Connect.... 
  
int Ccom::OpenPort(DWORD Oflag,ARGS_T *ArgDataPtr)
{          
	int             mode_flg;
	int rtrycnt = 0;
	mode_flg = (S_IWUSR | S_IWGRP | S_IWOTH); 
	while(1==1) {
		async_des = open(PortNamePtr, O_RDWR | (int) Oflag, mode_flg);
		if(async_des == -1) {
			if(++rtrycnt < 4 && errno == ENOMEM) {
				sleep(1);
				continue;
			} else {
				Cmsg ToHtx(ArgDataPtr);
				sprintf(ToHtx.GetMsgAddr(), "Error opening Port %s: %s Retried 4 times\n",PortNamePtr,strerror(errno));
				ToHtx.SendMsg(1, HTX_HE_HARD_ERROR);                      
				return(BADBOY);
			}
		}
		break;
	}
	return(0);
}

int Ccom::ClosePort(ARGS_T *ArgDataPtr)	
{
	Cmsg ToHtx(ArgDataPtr);
	// for digi's sake make sure buffers are
	// clean before close or close will hang.
	FlushBuffers();

   	int rc;
   	rc = close(async_des);
   	if (rc == -1) {
      	(void) sprintf(ToHtx.GetMsgAddr(), 
			"close error  - %s\n",strerror(errno));
	  	ToHtx.SendMsg(0,HTX_HE_SOFT_ERROR);
      	return(BADBOY);
   	}                           
   	async_des = -1;
	return(0);
}

int Ccom::Quick_Sync(ARGS_T *ArgDataPtr)	
{
	char wc[16];
	char val[16];
	int index=0;
	int hisnum=0;
	int mynum;
	int trycnt=0;
	char eventnum[16];
	char flag;
#define START_NUM	0x01
#define GOTIT		0x02


	// since this may occur while other port is set with echo on I need 
	// a way to know when the other side has done setportdefaults. I found
	// out that when echo is set and you send 0x01 the other side will
	// echo ^A so I just send 0x01 until I get 0x01 or 'T'. 
  Cmsg ToHtx(ArgDataPtr);
	flag=0x00;
	wc[0]=0x01;
	while(1) {
		WriteComm(1,(char *)&wc[0], ArgDataPtr);
		sleep(1);
		while(1) {
			if(ReadComm(1,(char *)&val[0],FLUSH_TIMEOUT,ArgDataPtr)>0) {
				if(val[0]=='T' || val[0]==0x01 ) {
					flag|=GOTIT;
					break; 
				}
			} else break;
		}
		if(flag&GOTIT) {
			break;
		}
		if(++trycnt > 30) {  // this is about 60 sec in aix
			  (void) sprintf(ToHtx.GetMsgAddr(),"Connection Failed: Possible bad or missing wrap plug or cable, or bad serial port hardware.\n");
			  ToHtx.SendMsg(1,HTX_HE_HARD_ERROR);
			  return(BADBOY);
		} 
	}


    memset(wc,0x00,16);
	strcpy((char *)wc,"T");
	/*
	 * in AIX its like 
	 * /dev/ttyX i.e tty number starts at 8th character 
	 * while in Linux its
	 * /dev/ttySX i.e. tty number starts at 9th char
	 */
	strcat((char *)wc,ArgDataPtr->MyInfo.DeviceName+DEVNAME_START);
	mynum=atoi((char *)(ArgDataPtr->MyInfo.DeviceName+DEVNAME_START));


	FlushBuffers();  // make sure pipe starts out clean...... 

	flag=0x00;
	while(1) {
		WriteComm(16,(char *)&wc[0], ArgDataPtr);
		sleep(1);
		// now read one byte at a time till you find TXXX
		while(1) {
			if(ReadComm(1,(char *)&val[index],FLUSH_TIMEOUT,ArgDataPtr)>0) {
				if(val[index]=='T' && !(flag&START_NUM) ) {
					flag|=START_NUM;
				} else if(flag&START_NUM) {
					if(isdigit(val[index])) {
					   	++index;
					} else if(val[index]==0x00) {
						hisnum=atoi(&val[0]);
						flag|=GOTIT;
						break; 
					} else break;
				} 
			} else break;
		}
		if(flag&GOTIT) break; 
	}
	if(mynum==hisnum) { 		// your wrapped. 
		ArgDataPtr->TestParms.wrap_type=ePlug;
#ifdef _DEBUG_LINUX_
		printf("\n\nSetting wrap type to :%d\n\n",ArgDataPtr->TestParms.wrap_type);	
#endif
	} else {
		ArgDataPtr->TestParms.wrap_type=eCable;
#ifdef _DEBUG_LINUX_
		printf("\n\nSetting wrap type to :%d\n\n",ArgDataPtr->TestParms.wrap_type);	
#endif
	}
	// now use event to find out when other side gets your number. 
	// but only if your port to port. 
	strcpy(ArgDataPtr->InitS,"EinitS");
	if(ArgDataPtr->TestParms.wrap_type!=ePlug) {
		// use the smaller of the two tty numbers for event name. 
		if(mynum > hisnum) {
			// his num is smaller use it for event. 
			ArgDataPtr->Flags |= SERVER;
			sprintf(eventnum,"%d",hisnum);
			strcat(ArgDataPtr->InitS,eventnum);
		}
		else {
			// my num is smaller use it for event. 
			ArgDataPtr->Flags |= CLIENT;
			sprintf(eventnum,"%d",mynum);
			strcat(ArgDataPtr->InitS,eventnum);
		}
	} else {
			ArgDataPtr->Flags |= SERVER;
			sprintf(eventnum,"%d",mynum);
			strcat(ArgDataPtr->InitS,eventnum);
	}
	return(hisnum);
}

void Ccom::Clean_Read_Q(ARGS_T *ArgDataPtr)	
{
	char tmpbuf[100];
	while(1) {
        if(ReadComm(100,tmpbuf,FLUSH_TIMEOUT,ArgDataPtr)<=0) break;
	}
	return;
}
int Ccom::FlushBuffers()	
{
	int rc;
	/*set up retry on these flushes..*/
	int MAX_RETRY=10;
	int RETRY=0;

	while(true) {
		++RETRY;
		rc = tcflush(async_des, TCIOFLUSH);
		if (rc == -1) {     
			if(RETRY > MAX_RETRY) return(BADBOY);
		} else {
			break;
		}
		usleep(ArgPtr->Rules.ioctl_sleep);
	}
    usleep(ArgPtr->Rules.ioctl_sleep);
	RETRY=0;
	while(true) {
		++RETRY;
		rc = tcflush(async_des, TCIOFLUSH);
		if (rc == -1) {     
			if(RETRY > MAX_RETRY) return(BADBOY);
		} else {
			break;
		}
		usleep(ArgPtr->Rules.ioctl_sleep);
	}
    usleep(ArgPtr->Rules.ioctl_sleep);
	
	return(0);
}

DWORD Ccom::ReadComm(   DWORD BytesToRead,

						char *Buffer,
						int time_out,
						ARGS_T *ArgDataPtr)
{
#ifndef __HTX_LINUX__
	size_t BytesRead=0;
#else
	ssize_t	BytesRead=0;	/*linux expects return from read/write call tobe of type ssize_t*/							  
#endif

#ifndef ALARM_S
	fd_set                  rd, wr, ex;
	struct timeval          timeout;
#endif
	DWORD					rc;
	int						timeout_cnt=0;
	
#ifndef ALARM_S
	while(++timeout_cnt < 4) {
		// Use select since alarm does not work in threads.. 
		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_ZERO(&ex);
		FD_SET(async_des, &rd) ;   // wait to read 
		// I need to set usec times if passed in value >= 1000
		if(time_out < 1000) {
			timeout.tv_sec = time_out;
			timeout.tv_usec = 0;
		} else {
			timeout.tv_sec = 0;
			timeout.tv_usec = time_out;
		}
		rc = select(FD_SETSIZE, &rd, &wr, &ex, &timeout);
		if (rc == -1) {
			if(!(ArgDataPtr->Flags&ABORT)) {
				Cmsg ToHtx(ArgDataPtr);
				sprintf(ToHtx.GetMsgAddr(), 
				"Read select failed: %s\n",strerror(errno));
				ToHtx.SendMsg(0, HTX_HE_INFO);                      
			}
			return(BADBOY);
		}
		// time_out of FLUSH_TIMEOUT is set on connnection and flush. 
		// do loop then. or connect don't work in linux
		if (rc == 0 && time_out == FLUSH_TIMEOUT) return(0);
		if (rc != 0)  break; 
		// So it stays in the loop for 4 select 0's then it goes on to do the
		// read anyway, and the read uses alarm too. See below.
	}
#endif

 	while(1) {

//#ifdef ALARM_S
	if(time_out > 1000) time_out = 1;
	alarm(time_out);
//#endif
		BytesRead=read(async_des,(void *)Buffer,(size_t)BytesToRead);
		
//#ifdef ALARM_S
		alarm(0);  
//#endif

		if(BytesRead==BADBOY) {
//#ifdef ALARM_S
			if(errno==EINTR) return(0);    // alarm popped.......
//#endif
			// I seem to get a lot of resource temp unavailable so lets put
			// in retry forever after sleep. 
			if(errno==EAGAIN) {
				usleep(100000);
				return(0);
			}
			Cmsg ToHtx(ArgDataPtr);
			sprintf(ToHtx.GetMsgAddr(), "Read failed:  %s \n",strerror(errno));
			ToHtx.SendMsg(1, HTX_HE_HARD_ERROR);                      
			return(BADBOY);
		}
		else break;
	}

	// return the bytes read or -1 on error..... 
	return((DWORD)BytesRead);
}

DWORD Ccom::WriteComm(  DWORD BytesToWrite,
						char * Buffer,
						ARGS_T *ArgDataPtr)
{
	int retry=0;
	DWORD BytesWritten;
	
#ifdef TRACE_
	cout <<"Ccom::WriteComm entry"<<endl;
#endif
	while(1) {
		if(ArgDataPtr->Flags&ABORT) return(BADBOY);
		BytesWritten = (DWORD)write(async_des, (void *)Buffer,(size_t)BytesToWrite);
		if(BytesWritten == BADBOY) {
/*
			if(errno==EAGAIN) {
				// NOTE: aix does not work right in non blocking mode.
				// it will return -1 even if it takes part of the data. 
				usleep(500000);
				Cmsg ToHtx(ArgDataPtr);
				sprintf(ToHtx.GetMsgAddr(), "Got EAGAIN %d \n",ERRNO);
				ToHtx.SendMsg(0, HTX_HE_INFO);
				return(0);    // make called retry later. 
			}
*/
			sleep(1);
			if(++retry==10) {
				Cmsg ToHtx(ArgDataPtr);
				sprintf(ToHtx.GetMsgAddr(), "Writes failing:  errno=%d\n",ERRNO);
				ToHtx.SendMsg(1, HTX_HE_HARD_ERROR);
				return(BADBOY);
			}
			sleep(5);
			continue;
		} else break;
	}
#ifdef TRACE_
	printf("WriteComm::exit with:%d\n",BytesWritten);
#endif
	return((DWORD)BytesWritten);
}

//**********************************************************************
//		END Ccom object code
//**********************************************************************
