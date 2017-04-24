
/* @(#)77	1.3.4.3  src/htx/usr/lpp/htx/lib/htx64/hxfmsg.c, htx_libhtx, htxfedora 1/12/16 05:16:25 */

#include <htx_local.h>
#include <hxihtx64.h>
#include <stdlib.h>


/*
 * NAME: hxfmsg()
 *
 * FUNCTION: Sends a message to the HTX system via the hxfupdate() ERROR call.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This function is called by any Hardware Exerciser (HE) program and
 *      is included as part of the libhtx.a library.
 *
 * NOTES:
 *
 *      operation:
 *      ---------
 *
 *      set htx_data error number and severity fields
 *
 *      copy message string to htx_data text field
 *
 *      call hxfupdate() ERROR to send message to HTX system
 *
 *      return(hxfupdate return code)
 *
 *
 * RETURNS:
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *                   0 -- Normal exit; buffers compare OK.
 *       exit_code > 0 -- Problem in hxfupdate() ERROR call.
 *
 *
 */

int hxfmsg(struct htx_data *p, int err, enum sev_code  sev, char *text)
     /*
      * p -- pointer to the Hardware Exerciser's htx_data data structure
      * err -- error code
      * sev -- severity code
      * text -- pointer to message text
      */
{
  /*
   ***  Beginning of Executable Code  *****************************************
   */
	if(p == NULL){ /* to handle cases when the htx_data pointer is not updated in hxfupdate*/
		printf("%s",text);
		return 0;
	}
	else{
		p->error_code = err;
		p->severity_code = sev;
		(void) strncpy(p->msg_text, text, MAX_TEXT_MSG);
		if (p->msg_text[MAX_TEXT_MSG - 1] != '\0')
		p->msg_text[MAX_TEXT_MSG -1] = '\0';  /* string MUST end with null char  */

		if((p->severity_code) >= HTX_SYS_INFO){
			return(hxfupdate(MESSAGE, p));
		}
		else if( (p->severity_code) < HTX_SYS_INFO) {
			return(hxfupdate(ERROR, p));
		}
	}
	return 0;
} /* hxfmsg() */
