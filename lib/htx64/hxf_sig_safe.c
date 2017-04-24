/********************************************************************************
 * 	HTX Signal Safe Subroutines. 
 * 	Motive - All libc subroutines are not signal safe, this could lead to 
 *			 potentioal deadlocks, intention of these functions is to make 
 *			 libc calls as signal safe. 
 *			 Libc Call which are not signal safe : 
 *				1. malloc. 
 *				2. free
 * Descriptions : All the functionw would be a wrapper around the actuall 
 *			 libc calls, htx_* wrapper function would mask all signals, 
 *			 make the libc call and restore signal mask after call return 
 *			 finishes. 
 *******************************************************************************/

/*******************************************************************************
 * Overshadowing of actual libc call would have been done in 
 * standard HTX header file  
 ******************************************************************************/
#include <signal.h>
#include <errno.h> 
#include <stdlib.h>
#include <time.h>


/******************************************************************************
 * 	Signal safe malloc subroutine . 
 *  Syntax : 
 * 		void * htx_malloc(size_t size); 
 *****************************************************************************/

void * 
htx_malloc(size_t size) { 

	sigset_t newMask, oldMask; 
	void * ptr; 
	int ret = 0, skip_reset = 0, old_errno; 

	/**************************************************************************
     * Intialize the oldmask to exclude all of the defined signals. 
	 *************************************************************************/
	sigemptyset(&oldMask); 

	/*************************************************************************
	 * Intialize the newmask to include all of the defined signals. 
	 *************************************************************************/
	sigfillset(&newMask); 

	/*************************************************************************
	 * Block All signals. Save the thread's old signal mask.  
	 ************************************************************************/
	old_errno = errno; 

	ret = pthread_sigmask(SIG_BLOCK, &newMask, &oldMask); 
	if(ret == -1) { 
		errno = old_errno; 
		skip_reset = 1;  
	} 

	/*************************************************************************
	 * Issue the call now ...
	 ************************************************************************/
	ptr = (void *)malloc(size); 
	
	/************************************************************************
     * Restore the old mask for calling thread.If there are any pending 
	 * unblocked signals after the call to pthread_sigmask(), at least one 
	 * of those signals shall be delivered before the call to pthread_sigmask() 
	 * returns.        
	 ***********************************************************************/ 

	if(skip_reset == 0) { 
		/*******************************************************************
		 * If SIG_BLOCK would have failed then signal mask would remain 
		 * unchange, so donot reset here with old mask. 
		 ******************************************************************/
		old_errno = errno;
   		ret = pthread_sigmask(SIG_SETMASK, &oldMask, NULL);

		if(ret == -1) {
			errno = old_errno;
		} 
	} 

	return(ptr); 

} 

/******************************************************************************
 *  Signal safe free subroutine .
 *  Syntax :
 *      void htx_free(void * ptr);
 *****************************************************************************/

void 
htx_free(void * ptr) {

    sigset_t newMask, oldMask;
    int ret = 0, skip_reset = 0, old_errno;

    /**************************************************************************
     * Intialize the oldmask to exclude all of the defined signals.
     *************************************************************************/
    sigemptyset(&oldMask);

    /*************************************************************************
     * Intialize the newmask to include all of the defined signals.
     *************************************************************************/
    sigfillset(&newMask);

    /*************************************************************************
     * Block All signals. Save the thread's old signal mask.
     ************************************************************************/
    old_errno = errno;

    ret = pthread_sigmask(SIG_BLOCK, &newMask, &oldMask);
    if(ret == -1) {
        errno = old_errno;
        skip_reset = 1;
    }

    /*************************************************************************
     * Issue the call now ...
     ************************************************************************/
	free(ptr); 

    /************************************************************************
     * Restore the old mask for calling thread.If there are any pending
     * unblocked signals after the call to pthread_sigmask(), at least one
     * of those signals shall be delivered before the call to pthread_sigmask()
     * returns.
     ***********************************************************************/

    if(skip_reset == 0) {
        /*******************************************************************
         * If SIG_BLOCK would have failed then signal mask would remain
         * unchange, so donot reset here with old mask.
         ******************************************************************/
        old_errno = errno;
        ret = pthread_sigmask(SIG_SETMASK, &oldMask, NULL);

        if(ret == -1) {
            errno = old_errno;
        }
    }

}





/******************************************************************************
 * 	Signal safe localtime subroutine . 
 *  Syntax : 
 * 		struct tm * htx_localtime(const time_t *); 
 *****************************************************************************/

struct tm * 
htx_localtime(const time_t *p_time) { 

	sigset_t newMask, oldMask; 
	struct tm * ptr; 
	int ret = 0, skip_reset = 0, old_errno; 

	/**************************************************************************
     * Intialize the oldmask to exclude all of the defined signals. 
	 *************************************************************************/
	sigemptyset(&oldMask); 

	/*************************************************************************
	 * Intialize the newmask to include all of the defined signals. 
	 *************************************************************************/
	sigfillset(&newMask); 

	/*************************************************************************
	 * Block All signals. Save the thread's old signal mask.  
	 ************************************************************************/
	old_errno = errno; 

	ret = pthread_sigmask(SIG_BLOCK, &newMask, &oldMask); 
	if(ret == -1) { 
		errno = old_errno; 
		skip_reset = 1;  
	} 

	/*************************************************************************
	 * Issue the call now ...
	 ************************************************************************/
	ptr = localtime(p_time); 
	
	/************************************************************************
     * Restore the old mask for calling thread.If there are any pending 
	 * unblocked signals after the call to pthread_sigmask(), at least one 
	 * of those signals shall be delivered before the call to pthread_sigmask() 
	 * returns.        
	 ***********************************************************************/ 

	if(skip_reset == 0) { 
		/*******************************************************************
		 * If SIG_BLOCK would have failed then signal mask would remain 
		 * unchange, so donot reset here with old mask. 
		 ******************************************************************/
		old_errno = errno;
   		ret = pthread_sigmask(SIG_SETMASK, &oldMask, NULL);

		if(ret == -1) {
			errno = old_errno;
		} 
	} 

	return(ptr); 

} 


/******************************************************************************
 * 	Signal safe htx_ctime subroutine . 
 *  Syntax : 
 * 		char * htx_ctime(const time_t *); 
 *****************************************************************************/

char * 
htx_ctime(const time_t *p_time) { 

	sigset_t newMask, oldMask; 
	char * ptr; 
	int ret = 0, skip_reset = 0, old_errno; 

	/**************************************************************************
     * Intialize the oldmask to exclude all of the defined signals. 
	 *************************************************************************/
	sigemptyset(&oldMask); 

	/*************************************************************************
	 * Intialize the newmask to include all of the defined signals. 
	 *************************************************************************/
	sigfillset(&newMask); 

	/*************************************************************************
	 * Block All signals. Save the thread's old signal mask.  
	 ************************************************************************/
	old_errno = errno; 

	ret = pthread_sigmask(SIG_BLOCK, &newMask, &oldMask); 
	if(ret == -1) { 
		errno = old_errno; 
		skip_reset = 1;  
	} 

	/*************************************************************************
	 * Issue the call now ...
	 ************************************************************************/
	ptr = ctime(p_time); 
	
	/************************************************************************
     * Restore the old mask for calling thread.If there are any pending 
	 * unblocked signals after the call to pthread_sigmask(), at least one 
	 * of those signals shall be delivered before the call to pthread_sigmask() 
	 * returns.        
	 ***********************************************************************/ 

	if(skip_reset == 0) { 
		/*******************************************************************
		 * If SIG_BLOCK would have failed then signal mask would remain 
		 * unchange, so donot reset here with old mask. 
		 ******************************************************************/
		old_errno = errno;
   		ret = pthread_sigmask(SIG_SETMASK, &oldMask, NULL);

		if(ret == -1) {
			errno = old_errno;
		} 
	} 

	return(ptr); 

}


/******************************************************************************
 *  Signal safe localtime subroutine .
 *  Syntax :
 *      struct tm * htx_localtime_r(const time_t *);
 *****************************************************************************/

struct tm *
htx_localtime_r (const time_t *timep, struct tm *result) {

    sigset_t newMask, oldMask;
    struct tm * ptr;
    int ret = 0, skip_reset = 0, old_errno;

    /**************************************************************************
     * Intialize the oldmask to exclude all of the defined signals.
     *************************************************************************/
    sigemptyset(&oldMask);

    /*************************************************************************
     * Intialize the newmask to include all of the defined signals.
     *************************************************************************/
    sigfillset(&newMask);

    /*************************************************************************
     * Block All signals. Save the thread's old signal mask.
     ************************************************************************/
    old_errno = errno;

    ret = pthread_sigmask(SIG_BLOCK, &newMask, &oldMask);
    if(ret == -1) {
        errno = old_errno;
        skip_reset = 1;
    }

    /*************************************************************************
     * Issue the call now ...
     ************************************************************************/
    ptr = localtime_r(timep,result);

    /************************************************************************
     * Restore the old mask for calling thread.If there are any pending
     * unblocked signals after the call to pthread_sigmask(), at least one
     * of those signals shall be delivered before the call to pthread_sigmask()
     * returns.
     ***********************************************************************/

    if(skip_reset == 0) {
        /*******************************************************************
         * If SIG_BLOCK would have failed then signal mask would remain
         * unchange, so donot reset here with old mask.
         ******************************************************************/
        old_errno = errno;
        ret = pthread_sigmask(SIG_SETMASK, &oldMask, NULL);

        if(ret == -1) {
            errno = old_errno;
        }
    }

    return(ptr);

}

 

