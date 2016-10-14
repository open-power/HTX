
.machine "any"
.file  	 "sync_func.s"		
.globl  htx_sync
.align 2        
htx_sync:
.set SYNC_WORD,		3
.set CPU_ID_MASK,	4
again:								# Stream synchronization code
	lwarx   0,0,SYNC_WORD	   		#   Turn off my id bit in sync word
        andc    0,0,CPU_ID_MASK   	#     in an atomic manner
        stwcx.  0,0,SYNC_WORD	   	#
        bne     again				#
        sync                    	#	Make sure all is well
        isync
        mfcr 0
syncLoop:							#	Loop while others are syncing
	lwz     5,0(SYNC_WORD)			#	Get sync word
	cmpli   0,0,5,0					#	If zero, syncing is not needed
	beq     inSync					#	Cleanup and get out
	and.    6,5,CPU_ID_MASK			#	If my id bit is on
	beq     syncLoop				#	All is well, loop again
	.long 0x1						#	It should not come here	
inSync: blr							# return back

#====================================================================================

# First get the sync lock
# r5 - sync lock addr, r7 - scratch pad
#
.globl  htx_sync_new
.set GLOBAL_LOCK,	5
htx_sync_new:
syncme_inc:
	lwarx 7, 0, GLOBAL_LOCK
	cmpwi 7,0
	bne syncme_inc
	addi 7,7,0x1
	stwcx. 7, 0, GLOBAL_LOCK
	bne syncme_inc
	sync
##sync we have taken the lock, lets do the job
sync_loop:
	lwarx 7, 0, SYNC_WORD
	andc    7,7,CPU_ID_MASK
	stwcx. 7, 0, SYNC_WORD
	bne sync_loop
	sync
syncunlockit:
	lwarx 7, 0, GLOBAL_LOCK
	cmpwi 7,0x1
	bne sync_hang
	addi 7,7,-0x1
	stwcx. 7, 0, GLOBAL_LOCK
	bne syncunlockit
	isync
wait_loop:
	lwz 6, 0(SYNC_WORD)
	cmpwi 6, 0x0
	bne wait_loop
	blr
sync_hang:
	b $
