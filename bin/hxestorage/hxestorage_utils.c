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

/****************************************************************/
/*	Filename: 	hxestorage_utils.c                              */
/*	contains all the helper functions needed in exercising		*/
/*	the disk.													*/
/****************************************************************/
#include "hxestorage_utils.h"


static struct random_seed_t saved_seed, saved_data_seed;
static unsigned long long volatile saved_maxlba, saved_minlba;
unsigned long long saved_data_len, saved_num_oper;
extern pthread_mutex_t fencepost_mutex;

static int volatile seg_table_entries[1024] = {0};
int volatile collisions;

/*****************************************************************/
/****		set first_block number for sequential operations  ****/
/*****************************************************************/
unsigned long long set_first_blk (struct htx_data *htx_ds, struct thread_context *tctx)
{
	unsigned long long first_blk;
	char msg[512];

	if (tctx->starting_block == BOT) {
		first_blk = tctx->min_blkno;
	} else if (tctx->starting_block == MID) {
		first_blk = tctx->min_blkno + (tctx->max_blkno - tctx->min_blkno + 1) / 2;
	} else if (tctx->starting_block == TOP) {
		first_blk = tctx->max_blkno;
	} else {
		first_blk = tctx->starting_block;
		first_blk += tctx->min_blkno;
	}

	/* Adjust first blk if direction is UP and first blk is smaller than min_blkno */
	if (first_blk < tctx->min_blkno) {
		if (tctx->direction == UP) {
			sprintf(msg, "For thread with thread_id: %s, first_blk defined is less than min_blkno."
						"Setting it to min_blkno(0x%llx).\n", tctx->id, tctx->min_blkno);
			user_msg(htx_ds, 0, 0, INFO, msg);
			first_blk = tctx->min_blkno;
		} else {
			sprintf(msg, "For thread with thread_id: %s, first_blk defined is less than min_blkno."
						"Will not run the test.", tctx->id);
			user_msg(htx_ds, -1, 0, HARD, msg);
			return (-1);
		}
	}

	/* Adjust first blk if direction is DOWN  */
	if (tctx->direction == DOWN) {
		if (tctx->starting_block == MID) {
			first_blk = first_blk - tctx->num_blks + 1;
		} else if (first_blk + tctx->num_blks - 1 > tctx->max_blkno) { /* num_blks is greater than max_blkno */
 		    first_blk = first_blk - (first_blk + tctx->num_blks - tctx->max_blkno - 1);
	    }
	}

	if (tctx->direction == OUT && first_blk != (tctx->min_blkno + ((tctx->max_blkno - tctx->min_blkno + 1) / 2))) {
		first_blk = tctx->min_blkno + ((tctx->max_blkno - tctx->min_blkno + 1) / 2);
	}

	 /* If lba_align is defined, adjust first_blk to the alignment defined */
	if(tctx->lba_align) {
		if(first_blk % tctx->lba_align) {
			if (tctx->starting_block == BOT) {
				first_blk = first_blk + (tctx->lba_align - (first_blk % tctx->lba_align));
			} else if (tctx->starting_block == TOP) {
				first_blk = first_blk - (first_blk % tctx->lba_align);
			} else {
				if (tctx->direction == DOWN) {
					first_blk = first_blk - (first_blk % tctx->lba_align);
				} else {
					first_blk = first_blk + (tctx->lba_align - (first_blk % tctx->lba_align));
				}
			}
		}
	}
	/* DPRINT("%s:%d - id: %s, first blk: 0x%llx\n", __FUNCTION__,__LINE__, tctx->id, first_blk); */
	return first_blk;
}

/**************************************************************/
/*****      initialize block number based on direction     ****/
/**************************************************************/
void init_blkno (struct htx_data *htx_ds, struct thread_context *tctx)
{
	unsigned long long blk;

	/* initialize blkno[0/1/2] to -1 */
	tctx->blkno[0] = tctx->blkno[1] = tctx->blkno[2] = -1;

	/* DPRINT("%s:%d - id: %s, tid: 0x%llx, first_blk: 0x%llx, num_blks: 0x%llx, max_blkno: 0x%llx\n", __FUNCTION__, __LINE__,
				tctx->id, (unsigned long long)tctx->tid, tctx->first_blk, tctx->num_blks, tctx->max_blkno); */
	/*If the IO opers have wrapped around and dlen is Random,
     * we need to calculate blk again, otherwise, directly set it
     * to first blk.
     */
     if (tctx->do_partial != 0 && tctx->transfer_sz.increment == -1) {
        blk = set_first_blk(htx_ds, tctx);
    } else {
        blk = tctx->first_blk;
    }

	if (tctx->direction == UP) {
		tctx->blkno[0] = tctx->blkno[1] = blk;
		tctx->blkno[2] = blk + tctx->num_blks;
	} else if (tctx->direction == DOWN) {
		tctx->blkno[0] = tctx->blkno[2] = blk;
		tctx->blkno[1] = blk + tctx->num_blks;
	} else if (tctx->direction == IN) {
		if (blk > (tctx->min_blkno + (tctx->max_blkno - tctx->min_blkno) / 2)) {
			tctx->blkno[0] = tctx->blkno[2] = blk;
			tctx->blkno[1] = tctx->min_blkno - tctx->num_blks - tctx->blk_hop; /* set minimum block number to negative number */
		} else {
			tctx->blkno[0] = tctx->blkno[1] = blk;
			tctx->blkno[2] = tctx->max_blkno; /* set maximum block number to the max_blkno */
		}
	} else {
		tctx->blkno[0] = tctx->blkno[1] = tctx->blkno[2] = blk;
	}
	/* DPRINT("%s:%d - id: %s, blkno: 0x%llx\n", __FUNCTION__, __LINE__, tctx->id, tctx->blkno[0]);	 */
}

/*****************************************/
/****	set next block number		******/
/*****************************************/
void set_blkno (struct htx_data *htx_ds, struct thread_context *tctx)
{
	unsigned long long temp;

	/* DPRINT("%s:%d - id: %s, blkno[0]=0x%llx, blkno[1]=0x%llx, blkno[2]=0x%llx, direction=%d, blk_hop=0x%x, num_blks=0x%llx, blk_align=0x%x \n",
			__FUNCTION__, __LINE__, tctx->id, tctx->blkno[0], tctx->blkno[1], tctx->blkno[2], tctx->direction,
			tctx->blk_hop, tctx->num_blks, tctx->lba_align); */

	/****************************************************/
	/* If do_partial flag is set to 1 means last IO oper*/
	/* was a partial operation i.e. transfer size was 	*/
	/* less than defined in rule. Save back the original*/
	/* length and call init_blkno to set blkno for next	*/
	/* operation. Otherwise, set blkno for next oper	*/
	/* based on direction. 								*/
	/****************************************************/
	if (tctx->do_partial == 1) {
		tctx->dlen = tctx->saved_dlen;
		tctx->num_blks = tctx->dlen / dev_info.blksize;
		init_blkno(htx_ds, tctx);
		tctx->do_partial = 0;
	} else {
		if (tctx->direction == UP) {
			temp = tctx->blkno[2] + tctx->blk_hop;
			if (tctx->lba_align && (temp % tctx->lba_align)) {
				temp = temp + (tctx->lba_align - (temp % tctx->lba_align));
			}
			tctx->blkno[0] = tctx->blkno[1] = temp;
			tctx->blkno[2] = tctx->blkno[0] + tctx->num_blks;
		} else if (tctx->direction == DOWN) {
			temp = tctx->blkno[2] - tctx->num_blks - tctx->blk_hop;
			if (tctx->lba_align && (temp % tctx->lba_align)) {
				temp = temp - (temp % tctx->lba_align);
			}
			tctx->blkno[0] = tctx->blkno[2] = temp;
			tctx->blkno[1] = tctx->blkno[0] + tctx->num_blks;
		} else {
			if (tctx->blkno[0] == tctx->blkno[1]) {
				temp = tctx->blkno[2] - tctx->num_blks - tctx->blk_hop;
				if (tctx->lba_align && (temp % tctx->lba_align)) {
					temp = temp - (temp % tctx->lba_align);
				}
				tctx->blkno[0] = tctx->blkno[2] = temp;
			} else {
				temp = tctx->blkno[1] + tctx->num_blks + tctx->blk_hop;
				if (tctx->lba_align && (temp % tctx->lba_align)) {
					temp = temp + (tctx->lba_align - (temp % tctx->lba_align));
				}
				tctx->blkno[0] = tctx->blkno[1] = temp;
			}
		}
	}
	/* DPRINT("%s:%d - id: %s, blk: 0x%llx\n", __FUNCTION__, __LINE__, tctx->id, tctx->blkno[0]); */
}

/****************************************/
/***    Sets random block number    *****/
/****************************************/
void random_blkno (struct htx_data *htx_ds, struct thread_context *tctx, unsigned long long len)
{
	long tmp1, tmp2;
	unsigned long long rand_no, rand_blkno;
	unsigned int fencepost_index;
	unsigned long long aligned_blkno, num_blks;

	num_blks = len / dev_info.blksize;

	/*************************************************************************************/
	/* If there is no compare operation defined for the thread OR compare is defined but */
	/* no BWRC threads present, generate rand_blkno between min_blkno and max_blkno.	 */
	/* Otherwise, ignore min_blkno and max_blkno range of current thread and generate 	 */
	/* blkno between min_lba and max_lba of any randomly selected BWRC thread.			 */
	/* In case of "restore_seed", we will get min_blkno and max_blkno seved previously	 */
	/* by save_seeds stanza and generate random_blkno between them.						 */
	/*********************************************************************************** */
	if ((strcasecmp(tctx->oper, "wrc") == 0) || (total_BWRC_threads == 0) ||
	    (tctx->rule_option & RESTORE_SEEDS_FLAG) || (tctx->rule_option & SAVE_SEEDS_FLAG)) {  /* See defect SW352431 for detail on various conditions */
		(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &tmp1);
		(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &tmp2);
		rand_no = (unsigned long long)((unsigned long long)tmp1 << 32 | ((unsigned long long)tmp2 & 0xffffffffULL));
		rand_blkno  = tctx->min_blkno + (rand_no % (tctx->max_blkno - tctx->min_blkno - tctx->lba_align - num_blks + 1));
	} else {
		while(1) {
			(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &tmp1);
			(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &tmp2);
			rand_no = (unsigned long long)((unsigned long long)tmp1 << 32 | ((unsigned long long)tmp2 & 0xffffffffULL));
			fencepost_index = (unsigned int)rand_no % total_BWRC_threads;
			if ((lba_fencepost[fencepost_index].min_lba != lba_fencepost[fencepost_index].max_lba) && /* means BWRC thread has started */
				(lba_fencepost[fencepost_index].max_lba - lba_fencepost[fencepost_index].min_lba - tctx->lba_align + 1 > num_blks)) { /* BWRC thread has completed num_blks */
				break;
			}
			if (exit_flag == 1 || int_signal_flag == 'Y') {
                if (lba_fencepost[fencepost_index].max_lba - lba_fencepost[fencepost_index].min_lba - tctx->lba_align + 1  <= num_blks) {
                    num_blks = lba_fencepost[fencepost_index].max_lba - lba_fencepost[fencepost_index].min_lba - tctx->lba_align;
                    tctx->num_blks = num_blks;
                    tctx->dlen = tctx->num_blks * dev_info.blksize;
                }
                break;
		    }
		}
		/* DPRINT("thread_id: %s fencepost_index: %d, min_lba: 0x%llx, max_lba: 0x%llx, len: 0x%x, rand_no: 0x%llx\n", tctx->id,
					fencepost_index, lba_fencepost[fencepost_index].min_lba, lba_fencepost[fencepost_index].max_lba, len, rand_no); */
		rand_blkno = lba_fencepost[fencepost_index].min_lba + (rand_no % (lba_fencepost[fencepost_index].max_lba - lba_fencepost[fencepost_index].min_lba - tctx->lba_align - num_blks + 1));
	}
	aligned_blkno = rand_blkno;
	if (tctx->lba_align) {
		if(rand_blkno % tctx->lba_align) {
			aligned_blkno = rand_blkno + (tctx->lba_align - (rand_blkno % tctx->lba_align));
		}
	}
	/* DPRINT("thread_id: %s, blkno: 0x%llx\n", tctx->id, aligned_blkno); */
	tctx->blkno[0] = aligned_blkno;
}

/***************************************/
/****	Sets random data length		****/
/***************************************/
void random_dlen (struct thread_context *tctx)
{
	unsigned long long rand_len;
	long rand_no;

	/**************************************************************************/
	/* if transfer_sz.increment is -1 means genarate any random length between*/
	/* min_len and max_len. Length should be multiple of blksize.			  */
	/**************************************************************************/
	if (tctx->transfer_sz.increment == -1) { /* means random dlen */
		(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &rand_no);
		rand_len = rand_no % (tctx->transfer_sz.max_len - tctx->transfer_sz.min_len + 1);

		/* DPRINT("rand_len: %lld\n", rand_len); */
		rand_len = (rand_len / dev_info.blksize) * dev_info.blksize; /* making it multiple of blksize */
		tctx->dlen = tctx->transfer_sz.min_len + rand_len;
	} else {
		/* means increment/decrement size by a fixed value as defined
		 *  by transfer_sz.increment parameter. +ve value meand increment
		 * -ve value means decrement.
		 */
		tctx->dlen += tctx->transfer_sz.increment;
		if (tctx->transfer_sz.increment > 0 && tctx->dlen > tctx->transfer_sz.max_len) { /* means incrementing and crosses max_len */
			tctx->dlen = tctx->transfer_sz.min_len;
		} else if (tctx->transfer_sz.increment < 0 && tctx->dlen < tctx->transfer_sz.min_len) { /* means decrementing and crosses min_len */
			tctx->dlen = tctx->transfer_sz.max_len;
		}
	}
	tctx->num_blks = tctx->dlen / dev_info.blksize;
}

/***************************************************************************/
/****	Set initial seed based on user defined seed / restore seed flag	****/
/****	OR generate new seed.											****/
/***************************************************************************/
void set_seed (struct htx_data *htx_ds, struct thread_context *tctx)
{
	int i, fencepost_index;
	long rand_no;

	 if (tctx->rule_option & RESTORE_SEEDS_FLAG) {
		/* In here means Seeds were saved in previous stanza.
		 * need to restore (i.e. use the same seed);
		 */
		tctx->seed = saved_seed;
		tctx->data_seed = saved_data_seed;
		tctx->max_blkno = saved_maxlba;
		tctx->min_blkno = saved_minlba;
		/* Below code is important to get same blk nos. in case of SAVE_SEEDS_FLAG
		 * and RESTORE_SEEDS_FLAG as SAVE_SEEDS_FLAG stanza uses it to get BWRC
		 * zone where it should be doing operations.
		 */
		if (total_BWRC_threads != 0) {
			(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &rand_no);
		}
	} else if (tctx->rule_option & USER_SEEDS_FLAG) {
		/* Here we will only be updating max_blkno as seed and data_seed
		 * has already been updated in populate_thread_context.
		 */
		tctx->max_blkno = tctx->lba_seed;
	} else {
		init_seed(htx_ds, &(tctx->seed));
		init_seed(htx_ds, &(tctx->data_seed));
    }

    /* save the seeds generated as parent seeds. */
    for (i = 0; i < 3; i++) {
		tctx->parent_seed[i] = tctx->seed.xsubi[i];
		tctx->parent_data_seed[i] = tctx->data_seed.xsubi[i];
	}
	tctx->parent_lba_seed = tctx->max_blkno;

	if (tctx->rule_option & SAVE_SEEDS_FLAG) {
		saved_seed = tctx->seed;
		saved_data_seed = tctx->data_seed;
		/* If tot_BWRC_threads is non-zero, update max_blkno and min_blkno
		 * so that it lies in fencepost range of 1 BWRC thread and save it
		 * for use by RESTORE_SEED_FLAG stanza.
		 */
		if (total_BWRC_threads != 0) {
			(void) nrand48_r(tctx->seed.xsubi, &(tctx->seed.drand_struct), &rand_no);
			fencepost_index = (unsigned long long)rand_no % total_BWRC_threads;
			tctx->max_blkno = lba_fencepost[fencepost_index].max_lba;
			tctx->min_blkno = lba_fencepost[fencepost_index].min_lba;
		}
		saved_maxlba = tctx->max_blkno;
		saved_minlba = tctx->min_blkno;
		if (tctx->transfer_sz.increment == 0) { /* means fixed length */
			saved_data_len = tctx->dlen;
		} else {
			saved_data_len = -1;
		}
	}
}

/***************************************************************************/
/****	Initialize seed for random number generator (nrand48). This		****/
/****	routine initializes the random number seed from a  combination  ****/
/****   of process id, a count of invocations, and the current second. 	****/
/***************************************************************************/
void init_seed (struct htx_data *htx_ds, struct random_seed_t *seed)
{
	char msg[200];
	time_t time_v;
	unsigned rc, pid;
	static unsigned short invocation_count;

	(void) time(&time_v);
	pid = (unsigned) getpid();
	seed->xsubi[0] = (unsigned short) pid;
	seed->xsubi[2] = (unsigned short) time_v;
	rc = pthread_mutex_lock(&cache_mutex);
	if (rc) {
		sprintf(msg, "Mutex lock failed in process INIT_SEED, rc = %d\n", rc);
		user_msg(htx_ds, rc, 0, HARD, msg);
		exit(rc);
	}
	seed->xsubi[1] = invocation_count++;
	rc = pthread_mutex_unlock(&cache_mutex);
	if (rc) {
		sprintf(msg, "Mutex unlock failed in process INIT_SEED, rc = %d\n", rc);
		user_msg(htx_ds, rc, 0, HARD, msg);
		exit(rc);
	}
	memset(&(seed->drand_struct), 0, sizeof(struct drand48_data));
}

/**************************************************/
/****	returns random number of 4 bytes	  *****/
/**************************************************/
unsigned int get_random_number (struct random_seed_t *seed)
{
	long rand_no;

	(void) nrand48_r(seed->xsubi, &(seed->drand_struct), &rand_no);
	return((rand_no & UINT_MAX));
}

/**********************************************/
/***	Update num_oper for SEQ seek_type	***/
/***	if num_oper defined in rule is 0.	***/
/**********************************************/
void update_num_oper(struct thread_context *tctx)
{
	unsigned long long rc, oper = 1;

	if (tctx->direction == OUT || tctx->starting_block == TOP) {
		rc = tctx->max_blkno - tctx->min_blkno + 1;
	} else {
		rc = tctx->max_blkno - tctx->first_blk + 1;
	}
	if (strchr(tctx->oper, '[') != NULL) {
		if (tctx->compare_enabled == 'y') {
			oper = tctx->num_writes;
		} else {
			oper = tctx->num_reads;
		}
	}
	tctx->num_oper[SEQ] = rc / ((tctx->num_blks + tctx->blk_hop) * oper);

	if (rc % ((tctx->num_blks + tctx->blk_hop) * oper)) {
		tctx->num_oper[SEQ]++;
	}
}

/****************************************************************/
/****	check for wrap around on sequential operations	  	  ***/
/****************************************************************/
int wrap (struct thread_context *tctx)
{
	int rc = 0;

	/* In case of w[n]r[m]c kind of operation with random length, we will not do the partial
	 * transfer if hitting block limit as this is generating core dump due to change in dlen
	 * (see defect 969131). we will set do_partial to 2. In do_boundary_check(), it will just
	 * simply call init_blkno.
	 */
	if (tctx->direction == UP) {
		/* If num_blks goes beyong max_blkno. Need to reset or do partial transfer */
		if (tctx->blkno[1] + tctx->num_blks - 1 > tctx->max_blkno) {
			if ((tctx->num_writes > 1 && tctx->transfer_sz.increment == -1) ||
                (tctx->blkno[1] > tctx->max_blkno)) {
				tctx->do_partial = 2;
            } else {
				tctx->do_partial = 1;
			}
			rc = 1;
		}
	} else if (tctx->direction == DOWN) {
		/* if min_blkno is less than 0 or min_blkno, need to do reset or do partial transfer */
		if (tctx->blkno[2] < tctx->min_blkno || (long long) tctx->blkno[2] < 0) {
			if ((tctx->num_writes > 1 && tctx->transfer_sz.increment == -1) ||
                (((long long)(tctx->blkno[2]) + tctx->num_blks - 1) < (long long)tctx->min_blkno)) {
                tctx->do_partial = 2;
		    } else {
                tctx->do_partial = 1;
			}
			rc = 1;
		}
	} else if (tctx->direction == IN) {
		if ((tctx->blkno[1] > (tctx->min_blkno + (tctx->max_blkno - tctx->min_blkno + 1) / 2)) ||
			 (tctx->blkno[2] < (tctx->min_blkno + (tctx->max_blkno - tctx->min_blkno + 1) / 2))) {
			rc = 1;
		}
	} else {
		if ((tctx->blkno[1] + tctx->num_blks -1) > tctx->max_blkno ||
			(tctx->blkno[2] < tctx->min_blkno)) {
			rc = 1;
		}
	}
	return rc;
}

/***********************************/
/****	Do boundary checkings	****/
/***********************************/
void do_boundary_check (struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
	if ( tctx->direction == IN ) {
		/* If the direction is "IN" then we want to make sure that we don't */
		/* run off the end of the disk, either at the top or the bottom of  */
		/* disk. If we set the lba to an unusable lba then we just reset it */
		/* back to the original starting point and begin again.             */
		if ((tctx->blkno[0] < tctx->min_blkno) || ((long long)tctx->blkno[0] < 0) ||
			((tctx->blkno[0] + tctx->num_blks -1) > tctx->max_blkno)) {
			init_blkno(htx_ds, tctx);
		}
	} else if (tctx->direction == OUT) {
		/* If the direction is "OUT" then we want to make sure that   */
		/* we don't try to access a bad lba.      */
		if (tctx->blkno[0] > tctx->max_blkno || tctx->blkno[0] < tctx->min_blkno ||
           (long long)tctx->blkno[0] < 0) {
			init_blkno(htx_ds, tctx);
		}
	} else if (wrap(tctx)) {
		/* On a sequential transfer going from the bottom of the disk 		*/
		/* to the top or from the top to the bottom of the disk and there   */
		/* is less than num_blks left & partial xfer is required to finish. */
		/* Then re-calc the residual and adjust the data length, if         */
		/* less than num_blks left and partial xfer not required, then      */
		/* reset back to the starting block number.                         */
		if (tctx->do_partial == 1) {
			if (tctx->direction == UP) {
				tctx->num_blks = (tctx->max_blkno - tctx->blkno[1] + 1);
				tctx->blkno[2] = tctx->blkno[0] + tctx->num_blks;
			} else if (tctx->direction == DOWN) {
				tctx->num_blks = tctx->num_blks - (int)(tctx->min_blkno - tctx->blkno[2]);
				tctx->blkno[0] = tctx->blkno[2] = tctx->min_blkno;
				tctx->blkno[1] = tctx->blkno[0] + tctx->num_blks;
			} else { /* unexpected direction - init blk to be safe */
				prt_msg(htx_ds, tctx, loop, 0, INFO, "Code bug!! execution should not come here. Calling init_blkno.");
				init_blkno(htx_ds, tctx);
			}
			tctx->saved_dlen = tctx->dlen;
			tctx->dlen = tctx->num_blks * dev_info.blksize;
		} else { /* do_patial can either be 0 or 2 */
			init_blkno(htx_ds, tctx);
			tctx->do_partial = 0;
		}
	}
}

/************************************/
/***	Do fencepost check		*****/
/************************************/
void do_fencepost_check (struct htx_data *htx_ds, struct thread_context *tctx)
{
	/* If num_blks lie within fencepost of the BWRC zone of current thread,
	 * do nothing and return. Otherwise, call initialize blkno.
	 */
	if ((tctx->blkno[0] >= lba_fencepost[tctx->BWRC_zone_index].min_lba) && ((tctx->blkno[0] + tctx->num_blks - 1) <= lba_fencepost[tctx->BWRC_zone_index].max_lba)) { /* means num_blks lie within fencepost of 1 BWRC zone */
		return; /* We are good to do operation. do nothing and return */
	} else {
		/* defect SW441918 - In case of variable length, transfer size may go beyond LBA fencepost.
		 * If so, change the transfer size and then call init_blkno
		 */
		 if (tctx->transfer_sz.increment == -1) {
			 tctx->dlen = tctx->transfer_sz.min_len;
			 tctx->num_blks = tctx->dlen / dev_info.blksize;
		 }
		 init_blkno(htx_ds, tctx);
	}
}

/****************************************************/
/*****	Function to wait for fencepost updation *****/
/*****	to catch up for BWRC-zone of current    *****/
/*****	thread so that it can do first opertion	*****/
/****************************************************/
void wait_for_fencepost_catchup (struct thread_context *tctx)
{

	/****************************************************************/
	/* If there are BWRC threads defined and operation for current  */
	/* thread is not BWRC, We need to make sure that lba_fencepost  */
	/* of BWRC zone in which current thread lies has reached to a   */
	/* point where current thread can do initial operation,         */
	/* otherwise, hold the thread from running                      */
	/****************************************************************/
	while(exit_flag == 0 && int_signal_flag == 'N') {
		if ((tctx->first_blk >= lba_fencepost[tctx->BWRC_zone_index].min_lba) && ((tctx->first_blk + tctx->num_blks - 1) <= lba_fencepost[tctx->BWRC_zone_index].max_lba)) {
			break;
		}
		sleep(10);
	}
}

/***************************************************/
/**** 	Function to allocate read/write buffers	****/
/***************************************************/
int allocate_buffers (struct htx_data *htx_ds, struct thread_context *tctx, unsigned int alignment, int malloc_count)
{
	int count, num_mallocs, err_no;
	int bufrem;
	char msg[256], err_str[ERR_STR_SZ];
    unsigned long long size;

	/* For mode performance, We allocate all buffer at once. So, malloc_count
	 * represents total no. of mallocs to be done. Array elements of tctx->wptr/tctx->rptr
	 * gets updated from count 0 to malloc_count.
	 * For mode validation, We allocate only 1 buffer at a time. So, malloc_count represent
	 * which array element of tctx->wptr/tctx->rptr needs to be updated.
	 */
	if (tctx->mode == PERFORMANCE) {
		count = 0;
		num_mallocs = malloc_count;
		size = tctx->transfer_sz.max_len + alignment + 128;
	} else {
		count = malloc_count;
		num_mallocs = count + 1;
		size = tctx->dlen + alignment + 128;
	}
	for (; count < num_mallocs; count++) {
		tctx->wptr[count] = (char *) malloc(size);
		tctx->rptr[count] = (char *) malloc(size);
		if (tctx->wptr[count] == NULL || tctx->rptr[count] == NULL ) {
			err_no = errno;
			STRERROR_R(err_no, err_str, ERR_STR_SZ);
			sprintf(msg, "Thread_id = %s, Malloc error - %d(%s), size - %llx, count - %d\n",
			tctx->id, err_no, err_str, size, count);
			user_msg(htx_ds, err_no, 0, HARD, msg);
			return count;
		} else {
			malloc_count = malloc_count + 1;
		}
		tctx->strt_wptr[count] = tctx->wptr[count];
		tctx->strt_rptr[count] = tctx->rptr[count];
		if(alignment) {
			bufrem = (int) ((unsigned long long)tctx->wptr[count] % alignment);
			if (bufrem) {
				tctx->strt_wptr[count] = tctx->wptr[count] + (alignment - bufrem);
			}
			bufrem = (int)((unsigned long long)tctx->rptr[count] % alignment);
			if (bufrem) {
				tctx->strt_rptr[count] = tctx->rptr[count] + (alignment - bufrem);
			}
		}
	}
	return 0;
}

/*******************************************/
/*** Function to free read/write buffers ***/
/*******************************************/
void free_buffers(int *malloc_count, struct thread_context *tctx)
{
	int i;
	for (i = 0; i < *malloc_count + 1; i++) {
		if (tctx->wptr[i] != NULL) {
			free(tctx->wptr[i]);
			tctx->wptr[i] = NULL;
		}
		if (tctx->rptr[i] != NULL) {
			free(tctx->rptr[i]);
			tctx->rptr[i] = NULL;
		}
	}
	*malloc_count = 0;
}

/**********************************************/
/***	Function to free pattern buffers	***/
/**********************************************/
void free_pattern_buffers (struct thread_context *tctx)
{
	if (tctx->pattern_buffer != NULL) {
		free(tctx->pattern_buffer);
		tctx->pattern_buffer = NULL;
	}
}

/************************************************/
/****	set read buffer to binary 0xbb		*****/
/************************************************/
void clrbuf (char buf[], unsigned long long dlen)
{
	memset(buf, 0xbb, dlen);
}

/**************************************************/
/***	Build Header section of write buffer	***/
/**************************************************/
void bld_header (struct htx_data *htx_ds, struct thread_context *tctx, unsigned char *wbuf, unsigned long long lba,
 				char algorithm, unsigned short write_stamp, unsigned int cur_time)
{
	int i, cksum = 0;
	char blk_write;

	/* increase header size to 64 bytes */
	/** Format of the 48-byte header
	 ** 8 bytes = LBA number
	 ** 4 bytes = Time stamp (seconds since hxestorage started)
	 ** 10 bytes = ASCII device name
	 ** 12 bytes = Stanza ID generating this buffer - increment
	 ** 14 bytes = ASCII host name - increment
	 ** 2 bytes = state table write stamping if it is enabled, otherwise WriteStamping for hotness.
	 ** 8 bytes = RESERVED
	 ** 4 bytes = Buffer Signature (BUFSIG)
	 ** 2 bytes = Checksum
	 ** Total Overhead = 48 bytes (24 shorts)
	 ** The modulo-16 sum of all bytes should be 0.
	 **/

	/* Retrieve the number of seconds since Epoch (00:00:00 GMT, 1/1/1970) */
	cur_time = (unsigned int)(time(0) & 0xFFFFFFFFu);

	UPDATE_LBA((wbuf + LBA_POS), lba); /* 8 bytes LBA no */
	UPDATE_TIME_STAMP((wbuf + TIME_STAMP_POS), &cur_time); /* 4 bytes time-stamp */
	UPDATE_DEV_NAME(wbuf + DEV_NAME_POS);
	UPDATE_STANZA_ID((wbuf + STANZA_ID_POS), tctx->id); /* 8 byte Stanza Name */
	UPDATE_HOSTNAME(wbuf + HOSTNAME_POS); /* 12 byte Host name */
	if (enable_state_table == YES) {
        blk_write = get_write_status(lba);
        if (tctx->num_writes_remaining > 0) {
            blk_write = (blk_write + 1) % (1 << BITS_PER_BLOCK);
            if (blk_write == 0) {
                blk_write++;
            }
        }
        UPDATE_WRITE_STAMP((wbuf + WRITE_STAMP_POS), (unsigned short) blk_write);
    } else {
        UPDATE_WRITE_STAMP((wbuf + WRITE_STAMP_POS), write_stamp); /* Identify each write on same LBA sets uniquely */
	}
	RESERVE_BYTES(wbuf + RESERVED_BYTES_POS);
	UPDATE_BUFSIG(wbuf + BUFSIG_POS); /* 4 byte Buffer Signature */
	/* Calculate the checksum value by adding up the above values
	 * and calculating the two's complement so that overall sum is 0.
	 */
	for (i = 0, cksum = 0; i < (OVERHEAD - 1); i++ ) {
		cksum += *((unsigned short *)wbuf + i);
	}
	UPDATE_CHECKSUM((wbuf + CHECKSUM_POS), cksum); /* 2 byte Check-Sum */
}

/******************************************************/
/*   Calls bld_buf for building write buffer if state */
/*   table is not enabled. Otherwise, bld_buf will be */
/*   called from seg_lock, once the lock is taken.    */
/******************************************************/
int bldbuf (struct htx_data *htx_ds, struct thread_context *tctx, unsigned short write_stamping)
{
    int rc = 0;
#ifdef __CAPI_FLASH__
    if ((enable_state_table == NO) && (strcasecmp(tctx->oper, "U") != 0)) {
        rc = bld_buf(htx_ds, tctx, write_stamping);
    }
#else
    if (enable_state_table == NO) {
        rc = bld_buf(htx_ds, tctx, write_stamping);
    }
#endif
    return rc;
}

/*******************************************/
/****	build write buffer sequences	****/
/*******************************************/
int bld_buf (struct htx_data *htx_ds, struct thread_context *tctx, unsigned short write_stamping)
{
	unsigned long long lba, lba_count;
	long long i, j, k;
	unsigned int *uint_ptr, cur_time;
	char *char_ptr, msg[256];
	int bufsize;
	register unsigned int lba_low16;
	char algorithm;
	unsigned short *wbuf = (unsigned short *) tctx->wbuf;
	lba_count = tctx->num_blks;
    lba = tctx->blkno[0];

	/* Retrieve the number of seconds since Epoch (00:00:00 GMT, 1/1/1970) */
	cur_time = (unsigned int)(time(0) & 0xFFFFFFFFu);

	/* DPRINT("Inside bldbuf - id: %s, wbuf: 0x%llx\n", tctx->id, (unsigned long long)wbuf); */
	if (tctx->pattern_id[0] == '#') {
		algorithm = tctx->pattern_id[3];
		for ( ; lba_count > 0; ++lba, --lba_count ) {
			for (i = 1, j = 0; j <= num_sub_blks_psect; i++, j++) {
				/**********************************/
				/**** Build 48-bytes of header ****/
				/**********************************/
				bld_header(htx_ds, tctx, (unsigned char *)wbuf, lba, algorithm, write_stamping, cur_time);

				/****************************************************************/
				/*	Done with header population, adjust my input buffer and		*/
				/*	begin the pattern generation. 								*/
				/****************************************************************/
				wbuf += OVERHEAD;

				/********************************************************************/
				/*	We write a unique pattern in case num_sub_blks_psect is defined */
				/********************************************************************/
				if (num_sub_blks_psect) {
					/************************************************/
					/* bytes  description							*/
					/* -----  -----------							*/
					/* 0-1	  algorithm								*/
					/* 2-3	  counter (Example 0x0001, 0x0002 ...)  */
					/* 4-7	  LBA  Counter. (recreatable pattern)	*/
					/* 8-15	  Device name.							*/
					/************************************************/
					for (j=((dev_info.blksize /(num_sub_blks_psect +1) - HEADER_SIZE)/16); j; j--) {
						uint_ptr = (unsigned int *)wbuf;
						*wbuf = algorithm - '0';
						*(wbuf+1)=i;
						*(uint_ptr + 1) = lba+i;
						i++;
						char_ptr = (char *) (((unsigned int *)uint_ptr)+2);
						strncpy(char_ptr, dev_info.diskname, 8);
					} /* end j loop */
				} else {
					bufsize = dev_info.blksize - HEADER_SIZE; /* need buffer size in bytes */
					lba_low16 =  lba & 0xffff ;
					switch (algorithm) {
					case '3':
						generate_pattern3(wbuf, bufsize, lba_low16);
						break;
					case '4':
						generate_pattern4(wbuf, bufsize);
						break;
					case '5':
						generate_pattern5(wbuf, bufsize);
						break;
					case '7':
						generate_pattern7((char *)wbuf, bufsize, tctx->align);
						break;
					case '8':
						generate_pattern8(htx_ds, tctx, (char *)wbuf, bufsize);
						break;
					case '9':
						generate_pattern9(htx_ds, tctx, (char *) wbuf, bufsize);
						break;
					case 'A':
						generate_patternA(htx_ds, tctx, (char *)wbuf, bufsize);
						break;
					default:
						strcpy( msg, "Exerciser internal error - default switch taken in\n" );
						sprintf( msg + strlen( msg ), "bldbuf: pattern_id = '%s'.", tctx->pattern_id);
						user_msg(htx_ds, -1, 0, HARD, msg);
						memset( wbuf, '?', tctx->dlen );
						break;
				} /* end switch */
					wbuf += (bufsize/sizeof(unsigned short));
				} /* end if num_sub_blks_psect */
			} /* end j loop */
		} /* end lba_count loop */
	} else if (strncmp(tctx->pattern_id, "0x", 2) == 0) {
		for (; lba_count > 0; --lba_count) {
			for (j = dev_info.blksize; j; j -= tctx->pattern_size) {
				for (k=0; k < (tctx->pattern_size / 2); k++) { /* divided by 2 as wbuf is of type unsigned short */
					*wbuf++ = *(unsigned short *) &tctx->pattern_buffer[2 * k];
				}
			}
		}
	} else {
		memcpy(wbuf, tctx->pattern_buffer, tctx->dlen);
	}
	return 0;
}

/******************************************/
/***	Function to generate pattern 3	***/
/******************************************/
void generate_pattern3 (unsigned short *buf, int bufsize, register unsigned int lba_low16)
{
	int i, j;

	/****************************************************************/
	/* PATTERN_ID = 3												*/
	/* bytes  description											*/
	/* -----  ----------------------------							*/
	/* 0 -1   relative word number (ex. 0000, 0001, 0002, ...)		*/
	/* 2 - 3  recreateable pattern (ex. add the one to the starting */
	/*		  	(LBA to get your pattern).							*/
	/****************************************************************/

	for (i = 1, j = (bufsize / 2); j; j -= 2) {
		*buf++ = i++;
		*buf++ = lba_low16++;
	}
}

/******************************************/
/***    Function to generate pattern 4  ***/
/******************************************/
void generate_pattern4 (unsigned short *buf, int bufsize)
{
	int i;

	for (i = (bufsize / 2); i; i--) {
		*buf++ = 0xa5a5;
	}
}

/******************************************/
/***    Function to generate pattern 5  ***/
/******************************************/
void generate_pattern5 (unsigned short *buf, int bufsize)
{
	int i, j;

	for (i = 1, j = (bufsize / 2); j; j -= 2) {
		*buf++ = i++;
		*buf++ = 0xa5a5;
	}
}

/******************************************/
/***    Function to generate pattern 7  ***/
/******************************************/
void generate_pattern7 (char *buf, int bufsize, unsigned int line_length)
{
	/*********************************************************
	 * algorithm 7 choosen. This algorithm will generate the buffer depending
	 * on the width of the scsi bus. Say the width is 16 bits so the data
	 * transfer should be ffff 0000 ffff 0000 ... This will generate the
 	 * highest electrical stress on the scsi bus.
	 * Pattern Generated .......
	 * <----------- line_length -------------->
	 * FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * 0000000000000000000000000000000000000000
	 * FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * 0000000000000000000000000000000000000000
	 *********************************************************/

	int i;

	while (bufsize) {
		for(i = 0; ((i < line_length) && (bufsize)); i++) {
			*((char*)buf) = 0xff;
			buf++;
			bufsize--;
		}
		for(i = 0; ((i < line_length) && (bufsize)); i++) {
			*(char*)buf = 0x00;
			buf++;
			bufsize--;
		}
	}
}

/******************************************/
/***    Function to generate pattern 8  ***/
/******************************************/
int generate_pattern8 (struct htx_data *htx_ds, struct thread_context *tctx, char *buf, int bufsize)
{
	/*******************************************************
	 * Even line marches 1 from left to right
	 * Odd line marches 0 from right to left
	 * Pattern Generated ........
	 * <--line_length->
	 * fffffffffffffffe
	 * 4000000000000000
	 * fffffffffffffffd
	 * 2000000000000000
	 * fffffffffffffffb
	 *******************************************************/

	unsigned long long HEXFF = 0xFFFFffffFFFFffffULL;
	unsigned long long HEXZERO = 0x0000000000000000ULL;
	unsigned long long LEAD_ONE = 0x8000000000000000ULL;
	unsigned long long TRAIL_ZERO = 0x1 ;
	unsigned int num_lines, left_line, line;
	unsigned int dword_size, num_dwords, line_length;
	unsigned long long * pattern, * strt_pattern;
	unsigned long long pattern_8;
	char msg[512];

	line_length = tctx->align;
	/* We are going to write line_length data in one shot
	 * make sure bufsize is multiple of line_length, if not
	 * take care of line_left separately.
	 */
	num_lines = bufsize / line_length;
	left_line = bufsize % line_length;
	/* Check how many dwords each line spans */
	dword_size = sizeof(unsigned long long);
	num_dwords = line_length / dword_size;
	if(line_length % dword_size) {
		sprintf(msg, "generate_pattern8: Error : line_length = %d not multiple of dwords=%d \n", line_length, dword_size);
		user_msg(htx_ds, 0, 0, SOFT, msg);
		return(-1);
	}
	strt_pattern = (unsigned long long *) malloc(dword_size * num_dwords);
	if(strt_pattern == (unsigned long long *)NULL) {
		sprintf(msg, "unable to get strt_pattern, errno = %d \n", errno);
		user_msg(htx_ds, 0, 0, SOFT, msg);
		return(-1);
	}
	if(num_lines) {
		for(line = 0; line < num_lines; line++ ) {
			if ((line % 2) ==0) { /* Even Lines */
				pattern = strt_pattern;
				memset(pattern, 0x00, dword_size * num_dwords);
				pattern_8 = (HEXZERO | tctx->or_mask );
				pattern[tctx->begin_dword] = pattern_8;
				tctx->or_mask = tctx->or_mask >> 1;
				if(tctx->or_mask == 0 ) {
					/* One dwork finished, reset mask and move to next dword */
					tctx->or_mask = LEAD_ONE;
					tctx->begin_dword = (tctx->begin_dword + 1) % num_dwords;
				}

			} else { /* Odd lines */
				pattern = strt_pattern;
				memset(pattern, 0xFF, dword_size * num_dwords);
				pattern_8 = (HEXFF & ~(tctx->and_mask));
				pattern[tctx->trailing_dword]= pattern_8;
				tctx->and_mask = tctx->and_mask << 1;
				if (tctx->and_mask == 0) {
					/* One dwork finished, reset mask and move to next dword */
					tctx->and_mask = TRAIL_ZERO;
					if(tctx->trailing_dword > 0) {
						tctx->trailing_dword --;
					} else {
						tctx->trailing_dword = num_dwords - 1;
					}
				}
			}
			memcpy(buf, pattern, dword_size * num_dwords);
			buf += (dword_size * num_dwords);
		}
	}
	if(left_line) {
		num_dwords = left_line / dword_size;
		/* This is the even line left */
		pattern = strt_pattern;
		memset(pattern, 0x00, dword_size * num_dwords);
		pattern_8 = (HEXZERO | tctx->or_mask );
		pattern[(tctx->begin_dword % num_dwords)] = pattern_8;
		tctx->or_mask = tctx->or_mask >> 1;
		if(tctx->or_mask == 0 ) {
			/* One dwork finished, reset mask and move to next dword */
			tctx->or_mask = LEAD_ONE;
			tctx->begin_dword = (tctx->begin_dword + 1) % num_dwords;
		}
		memcpy(buf, pattern, dword_size * num_dwords);
		buf += (dword_size * num_dwords);
	}
	free(strt_pattern);
	return(0);
}

/******************************************/
/***    Function to generate pattern 9  ***/
/******************************************/
int generate_pattern9 (struct htx_data *htx_ds, struct thread_context *tctx, char *buf, int bufsize)
{
	char msg[256];

	/*******************************************************
	 * Even line has high entropy random data
	 * odd line marches 1 from right to left
	 * Pattern Generated ........
	 * <--line_length->
 	 * 3c3c3c3c3c3c3c3c
	 * 0000000000000001
	 * 55555555aa555a5a
 	 * 0000000000000002
	 *****************************************************/

	unsigned int num_lines, left_line, line;
	unsigned int dword_size, num_dwords;
	unsigned long long * pattern, * strt_pattern;
	unsigned long long trail_zero = 0x1;
	unsigned long long pattern_9;
	unsigned int line_length;

	line_length = tctx->align;
	/* We are going to write line_length data in one shot
	 * make sure bufsize is multiple of line_length, if not
	 * take care of line_left separately.
	 */
	num_lines = bufsize / line_length;
	left_line = bufsize % line_length;
	/* Check how many dwords each line spans */
	dword_size = sizeof(unsigned long long);
	num_dwords = line_length / dword_size;
	if(line_length % dword_size) {
		sprintf(msg, "generate_pattern9: Error : line_length = %d not multiple of dwords=%d \n", line_length, dword_size);
		user_msg(htx_ds, 0, 0, SOFT, msg);
		return(-1);
	}
	strt_pattern = (unsigned long long *) malloc(dword_size * num_dwords);
	if(strt_pattern == (unsigned long long *)NULL) {
		sprintf(msg, "unable to get strt_pattern, errno = %d \n", errno);
		user_msg(htx_ds, 0, 0, SOFT, msg);
		return(-1);
	}
	if(num_lines) {
		(void) nrand48_r(tctx->data_seed.xsubi, &(tctx->seed.drand_struct), (signed long *)tctx->rand_index);
		tctx->rand_index = tctx->rand_index % tctx->pattern_size;
		for(line = 0; line < num_lines; line++) {
			if((line%2) ==0) { /* Even Lines */
				if((tctx->rand_index + dword_size * num_dwords) < tctx->pattern_size) {
					pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
					tctx->rand_index += dword_size * num_dwords;
				} else {
					tctx->rand_index = 0;
					pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
				}
			} else { /* odd lines */
				pattern = strt_pattern;
				memset(pattern, 0x00, dword_size * num_dwords);
				pattern_9 = tctx->and_mask;
				pattern[tctx->trailing_dword] = pattern_9;
				tctx->and_mask = tctx->and_mask << 1;
				if (tctx->and_mask == 0) {
					tctx->and_mask = trail_zero;
					if (tctx->trailing_dword > 0) {
						tctx->trailing_dword--;
					} else {
						tctx->trailing_dword = num_dwords - 1;
					}
				}
			}
			memcpy(buf, pattern, dword_size * num_dwords);
			buf += dword_size * num_dwords;
		}
	}
	if(left_line) {
		num_dwords = left_line / dword_size;
		/* This is the even line left */
		pattern = strt_pattern;
		memset(pattern, 0x00, dword_size * num_dwords);
		if ((tctx->rand_index + dword_size * num_dwords) < tctx->pattern_size) {
			pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
			tctx->rand_index += dword_size * num_dwords;
		} else {
			tctx->rand_index = 0;
			pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
		}
		memcpy(buf, pattern, dword_size * num_dwords);
		buf += (dword_size * num_dwords);
	}
	free(strt_pattern);
	return(0);
}

/******************************************/
/***    Function to generate pattern A  ***/
/******************************************/
int generate_patternA(struct htx_data *htx_ds, struct thread_context *tctx, char *buf, int bufsize)
{
	/*******************************************************
	 * Even line has high entropy random data
	 * Odd line marches 0 from right to left
	 *****************************************************/
	unsigned long long HEXFF = 0xFFFFffffFFFFffffULL;
	unsigned int num_lines, left_line, line;
	unsigned int dword_size, num_dwords, line_length;
	unsigned long long *pattern, *strt_pattern;
	unsigned long long trail_zero = 0x1;
	unsigned long long pattern_A;
	char msg[256];

	line_length = tctx->align;
	/* We are going to write line_length data in one shot
	 * make sure bufsize is multiple of line_length, if not
	 * take care of line_left separately.
	 */
	num_lines = bufsize / line_length;
	left_line = bufsize % line_length;
	/* Check how many dwords each line spans */
	dword_size = sizeof(unsigned long long);
	num_dwords = line_length / dword_size;
	if(line_length % dword_size) {
		sprintf(msg, "generate_patternA: Error : line_length = %d not multiple of dwords=%d \n", line_length, dword_size);
		user_msg(htx_ds, 0, 0, SOFT, msg);
		return(-1);
	}
	strt_pattern = (unsigned long long *) malloc(dword_size * num_dwords);
	if(strt_pattern == (unsigned long long *)NULL) {
		sprintf(msg, "unable to get strt_pattern, errno = %d \n", errno);
		user_msg(htx_ds, 0, 0, SOFT, msg);
		return(-1);
	}
	if(num_lines) {
        (void) nrand48_r(tctx->data_seed.xsubi, &(tctx->data_seed.drand_struct), (signed long *)tctx->rand_index);
        tctx->rand_index = tctx->rand_index % tctx->pattern_size;
		for(line = 0; line < num_lines; line++) {
			pattern = strt_pattern;
			if((line%2) ==0) { /* Even Lines */
				if((tctx->rand_index + dword_size * num_dwords) < tctx->pattern_size) {
					pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
					tctx->rand_index += dword_size * num_dwords;
				} else {
					tctx->rand_index = 0;
					pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
				}
			} else {
				pattern_A = (HEXFF & ~(tctx->and_mask));
				memset(pattern, 0xFF, dword_size * num_dwords);
				pattern[tctx->trailing_dword]= pattern_A;
				tctx->and_mask = tctx->and_mask << 1;
				if (tctx->and_mask == 0) {
					/* One dwork finished, reset mask and move to next dword */
					tctx->and_mask = trail_zero;
					if (tctx->trailing_dword > 0) {
						tctx->trailing_dword--;
					} else {
						tctx->trailing_dword = num_dwords - 1;
					}
				}
			}
			memcpy(buf, pattern, dword_size * num_dwords);
			buf += (dword_size * num_dwords);
		}
	}
	if(left_line) {
		num_dwords = left_line / dword_size;
		/* This is the even line left */
		if ((tctx->rand_index + dword_size * num_dwords) < tctx->pattern_size) {
			pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
			tctx->rand_index += dword_size * num_dwords;
		} else {
			tctx->rand_index = 0;
			pattern =(unsigned long long *)(tctx->pattern_buffer + tctx->rand_index);
		}
		memcpy(buf, pattern, dword_size * num_dwords);
		buf += (dword_size * num_dwords);
	}
	free(strt_pattern);
	return(0);
}

/**************************************************************************/
/* format information message                                             */
/**************************************************************************/
void info_msg(struct htx_data *htx_ds, struct thread_context *tctx, unsigned long long loop, char *msg_text)
{
  	int i;
	char msg[512], direction[6];

	if (tctx->cur_seek_type == RANDOM) { /* Random access addresses */
		sprintf(msg_text,
			"%s  numopers=%lld  loop=%lld  blk=%#llx tansfer size=%lld\n "
			"min_blkno=%#llx max_blkno=%#llx oper=%s, RANDOM access\n"
			"Seed Values= %d, %d, %d\n"
			"Data Pattern Seed Values = %d, %d, %d\n",
			tctx->id, tctx->num_oper[RANDOM], loop, tctx->blkno[0], tctx->dlen,
			tctx->min_blkno, tctx->max_blkno, tctx->oper, tctx->parent_seed[0], tctx->parent_seed[1],
			tctx->parent_seed[2], tctx->parent_data_seed[0], tctx->parent_data_seed[1],
			tctx->parent_data_seed[2]);
  	} else { /* SEQ access addresses */
     	if (tctx->direction == UP) {
			strcpy(direction, "UP");
		} else if (tctx->direction == DOWN) {
			strcpy(direction, "DOWN");
		} else if (tctx->direction == IN) {
			strcpy(direction, "IN");
		} else {
			strcpy(direction, "OUT");
		}
		if (tctx->transfer_sz.increment != 0) { /* len is RANDOM */
			sprintf(msg_text,
				"%s  numopers=%lld  loop=%lld  blk=%#llx transfer size=%lld\n "
				"dir=%s min_blkno=%#llx max_blkno=%#llx oper=%s\n"
				"Seed Values= %d, %d, %d\n"
				"Data Pattern Seed Values = %d, %d, %d\n",
				tctx->id, tctx->num_oper[SEQ], loop, tctx->blkno[0], tctx->dlen, direction,
				tctx->min_blkno, tctx->max_blkno, tctx->oper, tctx->parent_seed[0], tctx->parent_seed[1],
				tctx->parent_seed[2], tctx->parent_data_seed[0],
				tctx->parent_data_seed[1], tctx->parent_data_seed[2]);
		} else {
			sprintf(msg_text,
			"%s numopers=%lld loop=%lld blk=%#llx len=%lld dir=%s "
			"min_blkno=%#llx max_blkno=%#llx\n",
			tctx->id, tctx->num_oper[SEQ], loop, tctx->blkno[0], tctx->dlen, direction,
			tctx->min_blkno, tctx->max_blkno);
		}
	}
	if (total_BWRC_threads != 0) {
		strcat(msg_text, "BWRC LBA fencepost Detail:\n");
		strcat(msg_text, "th_num                min_lba	               max_lba		status\n");
		for (i = 0; i < total_BWRC_threads; i++) {
			sprintf(msg, "%6d	%16llx	%16llx	%c\n", i, lba_fencepost[i].min_lba, lba_fencepost[i].max_lba, lba_fencepost[i].status);
			strcat(msg_text, msg);
		}
	}
}

/**************************************************************************/
/* send user defined message to HTX with more information                 */
/**************************************************************************/
void prt_msg(struct htx_data *htx_ds, struct thread_context *tctx, int loop,
        int err, int sev, char *text)
{
  	char msg[MSG_TEXT_SIZE], msg1[128];
	char err_str[ERR_STR_SZ];
#ifndef __HTX_LINUX__
    int rc = 0;
#endif

	/* If continue on err is set to yes means continue on all IO error.
     * if set to DATACHECK, continue only for READ and stop for any other type of error.
     */
     if ((dev_info.cont_on_err == YES && sev == HARD && tctx->io_err_flag > NO_IO_ERR) ||
        (dev_info.cont_on_err == DATACHECK && sev == HARD && tctx->io_err_flag == READ_ERR)) {
        sev = SOFT;
    } else {
        if ( dev_info.crash_on_anyerr ) {
	    #ifdef __HTX_LINUX__
	        do_trap_htx64( 0xDEADDEED, (unsigned long)loop, (unsigned long)tctx->blkno, (unsigned long)htx_ds, (unsigned long)tctx, 0, 0, 0);
	    #else
	        trap( 0xDEADDEED, loop, tctx->blkno, htx_ds, tctx);
	    #endif
	    }
	}
	info_msg(htx_ds, tctx, loop, msg);
	strcat(msg, text);
	if (err > 0) {
		STRERROR_R(err, err_str, ERR_STR_SZ);
		sprintf(msg1, "errno: %d(%s)\n", err, err_str);
		strcat(msg, msg1);
	}
#ifndef __HTX_LINUX__
    rc = pthread_mutex_lock(&log_mutex);
	if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "Mutex lock failed in function prt_msg, rc %d (%s)", rc, err_str);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
#endif
    hxfmsg(htx_ds, err, sev, msg);
#ifndef __HTX_LINUX__
    rc = pthread_mutex_unlock(&log_mutex);
	if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "Mutex unlock failed in function prt_msg, rc %d (%s)", rc, err_str);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
#endif
    return;
}

/**************************************************************************/
/* send a user defined message to HTX                                     */
/**************************************************************************/
void user_msg(struct htx_data *htx_ds, int err, int io_err_flag, int sev, char *text)
{
	char msg1[128], msg[MSG_TEXT_SIZE], err_str[ERR_STR_SZ];
#ifndef __HTX_LINUX__
    int rc = 0;
#endif

	/* If continue on err is set to yes means continue on all IO error
	 * if set to DATACHECK, continue only for READ and stop for any other type of error.
	 */
	if ((dev_info.cont_on_err == YES && sev == HARD && io_err_flag > NO_IO_ERR) ||
	     (dev_info.cont_on_err == DATACHECK && sev == HARD && io_err_flag == READ_ERR)) {
	    sev = SOFT;
    } else {
        if ((sev == HARD) && (dev_info.crash_on_anyerr)) {
	        #ifdef __HTX_LINUX__
	        do_trap_htx64(0xDEADDEED, (unsigned long)htx_ds, 0, 0, 0, 0, 0, 0);
	        #else
		    trap(0xDEADDEED, htx_ds);
	        #endif
	    }
    }
	strcpy(msg, text);
	if (err > 0) {
		STRERROR_R(err, err_str, ERR_STR_SZ);
		sprintf(msg1, "errno: %d (%s)\n", err, err_str);
		strcat(msg, msg1);
	}
#ifndef __HTX_LINUX__
    rc = pthread_mutex_lock(&log_mutex);
	if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "Mutex lock failed in function user_msg, rc %d (%s)", rc, err_str);
        hxfmsg(htx_ds, rc, HARD, msg);
        exit(1);
    }
#endif
    hxfmsg(htx_ds, err, sev, msg);
#ifndef __HTX_LINUX__
    rc = pthread_mutex_unlock(&log_mutex);
	if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "Mutex unlock failed in function user_msg, rc %d (%s)", rc, err_str);
        hxfmsg(htx_ds, rc, HARD, msg);
        exit(1);
    }
#endif
}

#ifndef __HTX_LINUX__
int get_lun(struct htx_data *htx_ds, uchar *dev_name)
{
	int  i, rc, parent_fildes;
	char sstring[256];
	struct Class *cusdev; /* customized devices class ptr */
	struct CuDv cusobj;
	struct CuAt *catt;
	struct CuPath cupath;
	char tmp[20];
	char dev_path[]="/dev/";

	if ( odm_initialize() == -1 ) {
		return(-1);
	}

	if ( (int)(cusdev = odm_open_class(CuDv_CLASS)) == -1 ) {
		odm_close_class(CuDv_CLASS);
		odm_terminate();
		return(-2);
		}

	sprintf(sstring, "name = %s", (ulong)dev_name);
	rc = (int)odm_get_first(cusdev, sstring, &cusobj);
	if ( rc == 0 ) {
		odm_close_class(CuDv_CLASS);
		odm_terminate();
		return(-3);
	} else if ( rc == -1 ) {
		odm_close_class(CuDv_CLASS);
		odm_terminate();
		return(-4);
	}
	if ( odm_close_class(cusdev) == -1 ) {
		odm_terminate();
		return(-5);
	}
	odm_terminate();

	/*************************************************************************/
	/**	ioctl with IOCINFO is not valid for usb/flash. this should not get  **/
	/**	called for USB/Flash dev.											**/
	/*************************************************************************/
	if ((strncmp(cusobj.parent,"usb",3) == 0 ) ||
		(strncmp(cusobj.parent,"flash",5) == 0))  {
		/* Will keep the old SCSI logic as it is, */
		strcpy(dev_info.disk_parent, cusobj.parent);
		dev_info.disk_parent[strlen(cusobj.parent) ] = '\0';
	} else {
		strcpy(dev_info.disk_parent, cusobj.parent);
		dev_info.disk_parent[strlen(cusobj.parent) ] = '\0';
		strcpy(tmp, dev_path);
		strcpy(&tmp[strlen(dev_path)], dev_info.disk_parent);

		/* First find out what kind of adapter is our parent */
		parent_fildes = open(tmp, O_RDONLY);  /* Open device */
		if (parent_fildes == -1 ) {
			return -1;
		}
		rc = do_ioctl(htx_ds, parent_fildes, IOCINFO, &(dev_info.parent_info)); /* get device information */
		if (rc == -1) {
			return(-6);
		}
		while (close (parent_fildes) == -1);

		if (dev_info.parent_info.devsubtype == DS_SAS) {
			/* New Code to support SAS adapters */
			struct Class *cusdev;
			if (odm_initialize() == -1) {
				return(-7);
			}
			if ((int)(cusdev = odm_open_class(CuPath_CLASS)) == -1 ) {
				odm_close_class(CuPathAt_CLASS);
				odm_terminate();
				return(-8);
			}
			sprintf(sstring, "name = %s", (ulong)dev_name);
			rc = (int)odm_get_first(cusdev, sstring, &cupath);
			if (rc == 0) {
				odm_close_class(CuPath_CLASS);
				odm_terminate();
				return(-9);
			} else if ( rc == -1 ) {
				odm_close_class(CuPath_CLASS);
				odm_terminate();
				return(-10);
			}
			if ( odm_close_class(cusdev) == -1 ) {
				odm_terminate();
				return(-11);
			}
			odm_terminate();

			strcpy(tmp, cupath.connection);
			tmp[strlen(cupath.connection)] = '\0';
			strcpy(dev_info.disk_parent, cupath.parent);
			dev_info.disk_parent[strlen(cupath.parent)] = '\0';
		}

		/*
		 * The tmp string contains a string of the form 10,0
		 * So, the code snippet below tries to get the scsi id
		 * and lun id from connwhere and then get the scsi id
		 * and lun id of its parent. The parent id's will be used
		 * only in case of pass-thru commands implemented in
		 * read_verify_disk and write_verify_disk functions in
		 * io_oper.c file. See defect 388686 for details.
		 */
		sscanf(tmp, "%llx,%llx", &dev_info.scsi_id, &dev_info.lun_id);
		rc = get_parent_lun(dev_info.disk_parent);
	}
	return 0;
}

int get_parent_lun(uchar *dev_name)
{
	int  i, rc;
	char sstring[256]; /* search criteria pointer      */
	struct Class *cusdev; /* customized devices class ptr */
	struct CuDv cusobj;
	char tmp[20];

	if ( odm_initialize() == -1 ) {
		return(-1);
	}
	if ((int)(cusdev = odm_open_class(CuDv_CLASS)) == -1 ) {
		odm_close_class(CuDv_CLASS);
		odm_terminate();
		return(-1);
	}
	sprintf(sstring, "name = %s", (ulong)dev_name);
	rc = (int)odm_get_first(cusdev, sstring, &cusobj);
	if (rc == 0 ) {
		odm_close_class(CuDv_CLASS);
		odm_terminate();
		return(-1);
	} else if (rc == -1) {
		odm_close_class(CuDv_CLASS);
		odm_terminate();
		 return(-1);
	}
	if (odm_close_class(cusdev) == -1) {
		odm_terminate();
		return(-1);
	}
	odm_terminate();

	i = 0;
	while (isdigit(cusobj.location[i])) {
		tmp[i] = cusobj.location[i];
		i++;
	}
	tmp[i] = '\0';
	dev_info.scsi_id_parent = atoi(tmp);
	dev_info.lun_id_parent = atoi(&(cusobj.location[i+1]));
	return(0);
}

/*************************************************************************/
/* dump_iocmd is a debug routine that dump the sc_iocmd structure       */
/*************************************************************************/
void dump_iocmd(struct htx_data *htx_ds, struct sc_passthru *s)
{
	int i;
	char msg[512];

	sprintf(msg, "\nDump of the struct sc_passthru about to be sent:\n\n");
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "Version     = %d", s->version);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "data_length    = %llu (0x%llx); ", s->data_length, s->data_length);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "buffer = 0x%lx\n", s->buffer);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "timeout_value = %u (0x%x); ", s->timeout_value, s->timeout_value);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "status_validity = %u (0x%02x)\n", s->status_validity, s->status_validity);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "scsi_bus_status= %u (0x%02x); ", s->scsi_bus_status, s->scsi_bus_status);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "adapter_status = %u (0x%02x)\n", s->adapter_status, s->adapter_status);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "adap_q_status  = %u (0x%02x); ", s->adap_q_status, s->adap_q_status);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "q_tag_msg = %u (0x%02x)\n", s->q_tag_msg, s->q_tag_msg);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "flags          = %u (0x%02x); ", s->flags, s->flags);
	user_msg(htx_ds, 0, 0, INFO, msg);

	sprintf(msg, "command_length = %u (0x%02x)\n", s->command_length, s->command_length);
	user_msg(htx_ds, 0, 0, INFO, msg);

	for (i = 0; i < 10; i++) {
		sprintf(msg, "scsi_cdb[%2d] = 0x%02x\n", i, s->scsi_cdb[i]);
		user_msg(htx_ds, 0, 0, INFO, msg);
	}
}
#endif

/*****************************************************************************
** seg_lock()
** This routine locks a segment described by a starting and ending LBA
** number.  it will sleep waiting for another thread to unlock a segment
** that overlaps the one we want to lock.  The seg_lock function increments
** the number of segment collisions it encountered while trying to lock
** the segment.  This count is zeroed at the start of a rule-file scan and
** printed to the HTX log at the end of a scan.
******************************************************************************/
void seg_lock(struct htx_data *htx_ds, struct thread_context *tctx, unsigned long long flba, unsigned long long llba)
{
	int i,j, k = 0, l = 0, rc, locked = 0, lba_limit_found=0;
	char msg[256], err_str[ERR_STR_SZ];
	int match_found;
	unsigned long long seg_llba;/* Individual segment llba value */
	unsigned long long seg_flba;/* Individual segment flba value */

	/* tid = pthread_self(); */
	for(i = 0; i < 4; i++) {
		tctx->seg_table_num[i] = -1;
		tctx->seg_table_index[i] = -1;
	}

	/* This loop scans through all the segment tables to figure out the the given lba range falls in which table */
	for( i = 0; i < seg_info.num_of_seg_tables; i++) {
		if(flba >= (seg_info.seg_table + i)->LBA_lower_limit && flba <= (seg_info.seg_table + i)->LBA_upper_limit) {

			tctx->seg_table_num[k] = i;
			seg_flba = flba;	/* flba falls in current segment */

			if(llba > (seg_info.seg_table + i)->LBA_upper_limit){
				/* llba doesnot fall in current segment. Hence update segment table lba upper limit to seg_llba */
				seg_llba = (seg_info.seg_table + i)->LBA_upper_limit;
			} else {
				seg_llba = llba;		/* llba falls in current segment */
				lba_limit_found = 1;	/* llba found. Tag that breaks further loop iterations
										to find out llba */
			}
		} else if(flba < (seg_info.seg_table + i)->LBA_lower_limit) {

			tctx->seg_table_num[k] = i;
			seg_flba = (seg_info.seg_table + i)->LBA_lower_limit;	/* flba falls in previous segment */

			if(llba > (seg_info.seg_table + i)->LBA_upper_limit){
				/* llba doesnot fall in current segment. Hence update segment table lba upper limit to seg_llba */
				seg_llba = (seg_info.seg_table + i)->LBA_upper_limit;
			} else {
				seg_llba = llba;		/* llba falls in current segment */
				lba_limit_found = 1;	/* llba found. Tag that breaks further loop iterations
										to find out llba */
			}
		} else {
			continue;	/* flba and llba range doesnot fall in current segment.
						continue loop iteration to find the range in next segments */
		}

		rc = pthread_mutex_lock(&(seg_info.seg_table + i)->segment_mutex); /* Segment lock for respective table */
		if (rc) {
			STRERROR_R(rc, err_str, ERR_STR_SZ);
			sprintf(msg, "Mutex lock failed in process SEG_LOCK, rc %d (%s)", rc, err_str);
			user_msg(htx_ds, rc, 0, HARD, msg);
			exit(1);
		}

		/* Repeat until we are able to lock our segment... */
		locked = 0;
		while ( !locked ) {
			match_found = 0;	/* If set to 1 mentions that IO is already happening in given lba range */
			tctx->seg_table_index[l] = -1;	/* Variable that holds the segment table index of an individual thread */

			/* Scan the locked segments to see if ours overlaps one already locked */
			/* DPRINiT("seg_lock: tid: 0x%llx, seg_table_entries: %d, flba: %llx, llba: %llx\n", (unsigned long long)tctx->tid, seg_table_entries, flba, llba); */

			for (j = 0; j < seg_table_entries[i]; j++) {
				if ((seg_info.seg_table + i)->seg_table_data[j].in_use & 0x1) {
				/* DPRINT("In here - tid: 0x%llx, j: %d\n", (unsigned long long)tctx->tid, j); */
					if ((((seg_info.seg_table + i)->seg_table_data[j].flba < flba) && ((seg_info.seg_table + i)->seg_table_data[j].llba >= flba)) || (((seg_info.seg_table + i)->seg_table_data[j].flba > flba) && ((seg_info.seg_table + i)->seg_table_data[j].flba <= llba)) || ((seg_info.seg_table + i)->seg_table_data[j].flba == flba)) {
						match_found = 1;	/* Current segment table index is in use and found our flba and llba value overlaps */
						break;
					}
				} else if (tctx->seg_table_index[l] == -1) {
					tctx->seg_table_index[l] = j;	/* Store the unused index */
				}
			}

			if (match_found == 0) { /* means our segment doesn't overlap any locked segment, so lock it. */
				if (tctx->seg_table_index[l] == -1 ) {
					if (j < (MAX_THREADS + MAX_BWRC_THREADS)) {
						tctx->seg_table_index[l] = j;
						pthread_mutex_init(&((seg_info.seg_table + i)->seg_table_data[j].time_mutex), DEFAULT_MUTEX_ATTR_PTR);
						seg_table_entries[i]++;
					} else {
						printf("**********seg_lock: All segment table entries are used.\n");
						sprintf(msg, "All segment table entries are used.\n");
						user_msg(htx_ds, -1, 0, HARD, msg);
						exit(1);
					}
				}

				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].flba = flba;		/* flba value */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].llba = llba;		/* llba value */
				/* flba value in given segment. Useful while debugging when flba-llba range falls across different segment tables. */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].seg_flba = seg_flba;
				/* flba value in given segment. Useful while debugging when flba-llba range falls across different segment tables. */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].seg_llba = seg_llba;
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].tid = tctx->tid;		/* thread id */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].th_context_addr = (unsigned long long) tctx; /* thread context addr*/
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].hang_count = 0;		/* Hang count */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].time_index = 0;      /* index into time array */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].io_in_progress = 0;  /* IO not started yet */
				(seg_info.seg_table + i)->seg_table_data[tctx->seg_table_index[l]].in_use = 1;			/* entry is valid */

				locked = 1;

				/* DPRINT("id: %s, tid: 0x%llx, locked_index: %d\n", tctx->id, (unsigned long long)tctx->tid, tctx->seg_table_index[i]); */

				/* in case state table is enabled, write buffer need to be build after taking
				* the lock, otherwise, due to race condition between various threads write
				* status bits comparison in header might give miscompare
				*/
				if (enable_state_table == YES) {
					bld_buf(htx_ds, tctx, UNDEFINED);
				}
			} else {
				/* Our segment overlaps one already locked.  Wait for another thread to
				* unlock, so we can check again. We are guaranteed to get a wakeup
				* since the fact that another segment is locked means at least one
				* one other thread is running.
				*/
				collisions++;
				/* DPRINT("tid: 0x%llx, match_found, i: %d going to wait on condition\n", (unsigned long long)tctx->tid, i);  */

				/* in case state table is enabled, write buffer need to be build after taking
				* the lock, otherwise, due to race condition between various threads write
				* status bits comparison in header might give miscompare
				*/

				if (enable_state_table == YES) {
					bld_buf(htx_ds, tctx, UNDEFINED);
				}

				rc = pthread_cond_wait(&(seg_info.seg_table + i)->segment_do_oper, &(seg_info.seg_table + i)->segment_mutex);
				if (rc) {
					STRERROR_R(rc, err_str, ERR_STR_SZ);
					sprintf(msg, "Cond_wait failed in process SEG_LOCK, rc %d (%s)", rc, err_str);
					user_msg(htx_ds, rc, 0, HARD, msg);
					exit(1);
				}
			}
		}

		/* Unlocking mutex once we found a non used index under segment table */
		rc = pthread_mutex_unlock(&(seg_info.seg_table + i)->segment_mutex);
		if (rc) {
			STRERROR_R(rc, err_str, ERR_STR_SZ);
			sprintf(msg, "Mutex unlock failed in process SEG_LOCK, rc %d (%s)", rc, err_str);
			user_msg(htx_ds, rc, 0, HARD, msg);
			exit(1);
		}

		/* Loop completion indicator. We have found last lba value in the current segment.Break from Loop */
		if(lba_limit_found == 1) {
			lba_limit_found = 0;
			break;
		}

		if(tctx->seg_table_num[k] != -1) {
			k++;
			l++;
		}
	}
}

/********************************************************/
/** seg_unlock()										*/
/** This routine unlocks a previously locked segment.	*/
/*********************************************************/
void seg_unlock(struct htx_data *htx_ds, struct thread_context *tctx, unsigned long long flba, unsigned long long llba)
{
	int i = 0, rc = 0;
	pthread_t tid;
	char msg[256], err_str[ERR_STR_SZ];

	tid = pthread_self();


	while(tctx->seg_table_num[i] != -1) {
		 /* DPRINT("%s:%d - seg unlock req - tid: 0x%x, flba: 0x%llx, llba: 0x%llx\n", __FUNCTION__, __LINE__, tid, flba, llba);  */

		rc = pthread_mutex_lock(&(seg_info.seg_table + tctx->seg_table_num[i])->segment_mutex);
		if (rc) {
			STRERROR_R(rc, err_str, ERR_STR_SZ);
			sprintf(msg, "Mutex lock failed in process SEG_UNLOCK, rc %d (%s)", rc, err_str);
			user_msg(htx_ds, rc, 0, HARD, msg);
			exit(1);
		}

		/* Scan the table looking for our segment... */
		if (((seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].in_use & 0x1) &&
			((seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].flba == flba) && ((seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].llba == llba) && ((seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].tid == tid)) {


			/* DPRINT("Inside seg_unlock: tid: 0x%x, id: %s, unlocked_index: %d\n", tid, tctx->id, tctx->seg_table_index[i]);  */
			/* Set in_use variable to 0 to indicate that current seg_table index is free to use */
			(seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].in_use = 0;
		} else {
			/* Something's gone seriously wrong - couldn't find the segment */
			sprintf(msg, "seg_unlock(): can't find segment! seg_table index at exit: %d\n" "flba=0x%llx, llba=0x%llx, tid=0x%llx,\n",
					tctx->seg_table_index[i], flba, llba, (unsigned long long)tid);
			user_msg(htx_ds, -1, 0, HARD, msg);
			exit(1);
		}

		/* Loop completion indicator. We have found last lba value in the current segment.Update lba fencepost */
		if (strcasecmp(tctx->oper, "BWRC") == 0) {
			if (lba_fencepost[tctx->fencepost_index].direction == UP) {
				lba_fencepost[tctx->fencepost_index].max_lba = llba;
			} else if (lba_fencepost[tctx->fencepost_index].direction == DOWN) {
				lba_fencepost[tctx->fencepost_index].min_lba = flba;
			}
			lba_fencepost[tctx->fencepost_index].last_update_time = time(0);
			/* DPRINT("%s:%d - id: %s, lba_fencepost - min_lba: 0x%llx, max_lba: 0x%llx\n", __FUNCTION__, __LINE__, tctx->id,
			lba_fencepost[tctx->fencepost_index].min_lba, lba_fencepost[tctx->fencepost_index].max_lba); */
		}

		/* DPRINT("tid: %x, before broadcasting\n", tid); */
		/* Broadcast to indicate other waiting threads to get lock*/
		rc =  pthread_cond_broadcast(&(seg_info.seg_table + tctx->seg_table_num[i])->segment_do_oper);
		if (rc) {
			STRERROR_R(rc, err_str, ERR_STR_SZ);
			sprintf(msg, "Broadcast failed in process SEG_UNLOCK, rc = %d (%s)", rc, err_str);
			user_msg(htx_ds, rc, 0, HARD, msg);
			exit(1);
		}

		rc = pthread_mutex_unlock(&(seg_info.seg_table + tctx->seg_table_num[i])->segment_mutex);
		if (rc) {
			STRERROR_R(rc, err_str, ERR_STR_SZ);
			sprintf(msg, "Mutex unlock failed in process SEG_UNLOCK, rc %d (%s)", rc, err_str);
			user_msg(htx_ds, rc, 0, HARD, msg);
			exit(1);
		}
		i++;
	}
}

/*****************************************************************************/
/** Hang_monitor - routine to watch for hung I/O threads.                    */
/** This routine scans through the segment lock table, checking the start    */
/** time of each pending I/O.  When an I/O in the table exceeds an integral  */
/** multiple of hang_time seconds, the table is printed in an HTX message    */
/** and it's associated hang counter variable is incremented.  If none of    */
/** the hang counters exceed a threshold, then the HTX message is constructed*/
/** with an error status of SOFT.  If any I/O exceeds the threshold for hang */
/** count, then the error status is set to HARD.                             */
/** This routine is started as an independent thread by main() and runs for  */
/** the duration of the exerciser.                                           */
/*****************************************************************************/
void hang_monitor (struct htx_data *htx_ds)
{
		#define MSG1_LIMIT      2048

        int i, j, rc, err_level, hung_switch;
        int threshold_exceeded=0, size, count = 0;
        pid_t pid;
        time_t current_time, oldest_time, temp_time;
        char msg[256], msg1[MSG1_LIMIT + 25], file_name[256];
        char err_str[ERR_STR_SZ], cmd[256], *dump_ptr = NULL;
        FILE *dump_fp;
        struct stat buffer;

        pid = getpid(); /* Get our process ID */
        while (1) {
        if (exit_flag == 1 || int_signal_flag == 'Y') {
            break;
        }

        oldest_time = current_time = time(0);
        hung_switch = 0;
		for(i = 0; i < seg_info.num_of_seg_tables; i++) {
        	/* Acquire the segment table mutex so table won't change on us... */
                if ((err_level = pthread_mutex_lock(&(seg_info.seg_table + i)->segment_mutex))) {
                        STRERROR_R(err_level, err_str, ERR_STR_SZ);
                        sprintf(msg, "Mutex lock failed in hang_monitor(), Segment table-%d, rc %d (%s)\n"
                                                "Hung I/O monitor for disk exerciser terminating.",
                                                i,err_level, err_str);
                        user_msg(htx_ds, err_level, 0, HARD, msg);
                        return;
                }
                /**************************************************************************/
                /** After each scan of the table, we will go to sleep until it's time to  */
                /** scan the table again.  However we want to adjust the amount of time we*/
                /** sleep so that we wake up in time to check the oldest thread in the    */
                /** table for the hang condition.  Thus, if the hang_time var is set for  */
                /** 5 seconds, and the oldest thread in the table started 1 second ago, we*/
                /** only want to sleep for 5-1=4 seconds so that we check that thread as  */
                /** close to the 5 second mark as possible.                               */
                /**************************************************************************/

                for (j = 0; j < seg_table_entries[i]; j++) {
                        if ((seg_info.seg_table + i)->seg_table_data[j].in_use & 0x1) {
 							/* take the lock and check if IO was on progress, then only look for hung IO.
							   Otherwise, go to next index. */
							err_level = pthread_mutex_lock (&((seg_info.seg_table + i)->seg_table_data[j].time_mutex));
							if (err_level != 0) {
								STRERROR_R(err_level, err_str, ERR_STR_SZ);
								sprintf(msg, "Mutex lock for time_mutex failed in hang_monitor(), rc: %d(%s)\n", err_level, err_str);
								user_msg(htx_ds, err_level, 0, HARD, msg);
							}
							if ((seg_info.seg_table + i)->seg_table_data[j].io_in_progress & IO_IN_PROGRESS) {
                               /****************************************************************************/
                                /** Check if I/O time exceeds a multiple of the hang_time var.  The object  */
                                /** is to only issue a new alert when a multiple of hang_time has been      */
                                /** exceeded.  Thus if hang_time is 5 seconds, we'll issue an alert message */
                                /** at 5, 10, 15, ... seconds.  The multiplier is provided by the hang_count*/
                                /** var for that I/O, which initially is 0 when the I/O is started and is   */
                                /** incremented each time a hang is detected.                               */
                                /****************************************************************************/
   							    if ((current_time - (seg_info.seg_table + i)->seg_table_data[j].last_update_time) >= (hang_time * ((seg_info.seg_table + i)->seg_table_data[j].hang_count + 1))) {
                                        /* Found a hung I/O!  Incr it's hang_count and check the threshold. */
                                        (seg_info.seg_table + i)->seg_table_data[j].hang_count = (current_time - (seg_info.seg_table + i)->seg_table_data[j].last_update_time) / hang_time;
                                        if ((seg_info.seg_table + i)->seg_table_data[j].hang_count > threshold) {
                                                threshold_exceeded = 1;
                                        }
										hung_switch++;
										break;
                                }
						    }
                            /**********************************************************************/
                            /** Record the oldest I/O in this period.  Adjust I/Os that have been */
                            /** already marked hung by their hang count for purposes of figuring  */
                            /** the oldest I/O.  This will be used in calculating the length of   */
                            /** time to sleep between table scans.                                */
                            /**********************************************************************/
                            if ((temp_time = (seg_info.seg_table + i)->seg_table_data[j].last_update_time + ((seg_info.seg_table + i)->seg_table_data[j].hang_count * hang_time)) < oldest_time) {
                             	oldest_time = temp_time;
                            }
							err_level = pthread_mutex_unlock (&((seg_info.seg_table + i)->seg_table_data[j].time_mutex));
							if (err_level != 0) {
								STRERROR_R(err_level, err_str, ERR_STR_SZ);
								sprintf(msg, "Mutex unlock for time_mutex failed in hang_monitor(), rc: %d(%s)\n", err_level, err_str);
								user_msg(htx_ds, err_level, 0, HARD, msg);
							}
                        }
					}

					/* Release the segment table mutex */
		            if ((err_level = pthread_mutex_unlock(&(seg_info.seg_table + i)->segment_mutex))) {
                    	STRERROR_R(err_level, err_str, ERR_STR_SZ);
                       	sprintf(msg, "Mutex unlock failed in hang_monitor(), Segment table-%d, rc %d (%s), Hung I/O monitor for disk exerciser terminating.",
                                          i, err_level, err_str);
                       	user_msg (htx_ds, err_level, 0, HARD, msg);
                       	return;
                	}
	   				if (hung_switch) {
                        if ((temp_time = (seg_info.seg_table + i)->seg_table_data[j].last_update_time + ((seg_info.seg_table + i)->seg_table_data[j].hang_count * hang_time)) < oldest_time) {
                         	oldest_time = temp_time;
                        }
						break;
					}

				}

				if (hung_switch) {
                        sprintf(msg1, "Hung I/O alert! Segment table-%d,  Detected %d I/O(s) hung.\nCurrent time: " "%d; hang criteria: %d secs, Hard hang threshold: %d\n"
                                                "Process ID: 0x%x\n" "       1st lba       Blocks       Kernel    Hang   Duration\n"
                                                "        (Hex)        (Hex)        Thread    Cnt    (Secs)\n",
                                                i,hung_switch, (unsigned int) current_time, hang_time, threshold, pid);
                        if (threshold_exceeded) {
                                sprintf(msg, "** Threshold of %d secs on one or more I/Os exceeded!\n", threshold * hang_time);
                                strcat(msg1, msg);
                                err_level = HARD;
                        } else {
                                err_level = SOFT;
                        }
                        sprintf(msg, "%#16llx    %6llx     %8llx      %d    %d \n", (seg_info.seg_table + i)->seg_table_data[j].flba, (seg_info.seg_table + i)->seg_table_data[j].llba - (seg_info.seg_table + i)->seg_table_data[j].flba + 1,
                                        (unsigned long long)(seg_info.seg_table + i)->seg_table_data[j].tid, (seg_info.seg_table + i)->seg_table_data[j].hang_count, (unsigned int) (current_time - (seg_info.seg_table + i)->seg_table_data[j].last_update_time));

                        strcat(msg1, msg);

                        /* Dump the procstack in a file then in a buffer and pass the buffer address to KDB */
                        sprintf (file_name, "%s/procstack_dump_%d", htx_ds->htx_exer_log_dir, count);
                    #ifdef __HTX_LINUX__
                        sprintf (cmd, "gstack %d > %s", pid, file_name);
                    #else
                        sprintf (cmd, "procstack %d > %s", pid, file_name);
                    #endif
                        rc = system (cmd);
                        if (!WIFEXITED(rc)) {
                            sprintf(msg, "command to execute procstack dump failed. rc: %d.\n", rc);
                            user_msg (htx_ds, -1,  0, HARD, msg);
                        } else {
                            if (count < 20) {
                                count++;
                            }
                            rc = stat (file_name, &buffer);
                            if ( rc == -1) {
                                err_level = errno;
                                sprintf (msg, "Could not get dumpfile size. errno: %d\n", errno);
                                user_msg (htx_ds, err_level, 0, HARD, msg);
                            } else {
                                size = buffer.st_size;
                                dump_ptr = malloc (size + 128);
                                if (dump_ptr == NULL) {
                                    err_level = errno;
                                    sprintf (msg, "Could not allocate memory for procstack dump. errno: %d\n", errno);
                                    user_msg (htx_ds, err_level, 0, HARD, msg);
                                } else {
                                    dump_fp = fopen(file_name, "r");
                                    if (dump_fp == NULL) {
                                        err_level = errno;
                                        sprintf (msg, "Could not open dumpfile. errno: %d\n", errno);
                                        user_msg (htx_ds, err_level, 0, HARD, msg);
                                    } else {
                                        rc = fread (dump_ptr, size, 1, dump_fp);
                                        fclose(dump_fp);
                                    }
                                }
                            }
                        }

                        if (dev_info.crash_on_hang) {
                        #ifdef __HTX_LINUX__
                                do_trap_htx64(0xBEEFDEAD, (unsigned long)(seg_info.seg_table + i)->seg_table_data, seg_table_entries[i], pid, (unsigned long)msg1,
                                (unsigned long)htx_ds, (unsigned long)dump_ptr, 0);
                        #else
                                trap(0xBEEFDEAD, (seg_info.seg_table + i)->seg_table_data, seg_table_entries[i], pid, msg1, htx_ds, dump_ptr);
                        #endif
                        }

                        user_msg (htx_ds, -1, 0, err_level, msg1);

						err_level = pthread_mutex_unlock (&((seg_info.seg_table + i)->seg_table_data[j].time_mutex));
						if (err_level != 0) {
							STRERROR_R(err_level, err_str, ERR_STR_SZ);
							sprintf(msg, "Mutex unlock for time_mutex failed in hang_monitor(), rc: %d(%s)\n", err_level, err_str);
							user_msg(htx_ds, err_level, 0, HARD, msg);
						}
                        if (dump_ptr != NULL) {
                             free (dump_ptr);
                             dump_ptr = NULL;
                        }
                }
                /* Sleep until time to check again... */
                temp_time = hang_time - (current_time - oldest_time);
                sleep(temp_time <= 0 ? hang_time : temp_time);
        }
}

/***********************************************************/
/*	Function to check lba alignment. Set it to multiple	   */
/* 	of num_blks if it is not.							   */
/***********************************************************/
void check_alignment(struct htx_data *htx_ds, struct thread_context *tctx)
{
	char msg[512];

	if (tctx->lba_align > 0 && tctx->num_blks % tctx->lba_align != 0) {
		sprintf(msg, "For thread with thread_id: %s, lba_align specified is not multiple of num_blks. "
					"Setting it to multiple of num_blks", tctx->id);
		user_msg(htx_ds, 0, 0, INFO, msg);
		if (tctx->lba_align < tctx->num_blks) {
			tctx->lba_align = tctx->num_blks;
		} else {
			tctx->lba_align -= (tctx->num_blks % tctx->lba_align);
		}
	}
}

/************************************************************/
/* 	Function to get buffer alignment defined for thread 	*/
/************************************************************/
unsigned int get_buf_alignment(struct thread_context *tctx)
{
	unsigned int alignment;

	if (tctx->align > 0) {  /* If align is specified then IO buffers are aligned to this */
		alignment = tctx->align;
	} else if (tctx->pattern_id[0] == '#' && /* These algorighms needs a min alignment */
			  (tctx->pattern_id[3] == '7' ||
			   tctx->pattern_id[3] == '8' ||
			   tctx->pattern_id[3] == '9' ||
			   tctx->pattern_id[3] == 'A')) {
		tctx->align = alignment = 16;
	} else {
		alignment = 0;
	}
	return alignment;
}

/**************************************************************/
/***	Function to update some section of HEADER in case 	***/
/***	hotness is defined. TIMEPSTAMP, WRITE_STAMPING and	***/
/***	CHECKSUM is updated.								***/
/**************************************************************/
int update_header(struct thread_context *tctx, register unsigned short write_stamping)
{
	char *wbuf = (char *) tctx->wbuf;
	unsigned long long lba, lba_count, j;
	int i;
	unsigned int cksum, cur_time;

    /* If state table is enabled, return as whole buffer will be build
     * again after taking the segment lock.
     */
    if (enable_state_table == YES) {
        return 0;
    }
	lba = tctx->blkno[0];
	cur_time = (unsigned int)(time(0) & 0xFFFFFFFFu);
	for (lba_count = tctx->num_blks; lba_count > 0; ++lba, --lba_count ) {
		for (j = 0; j <= num_sub_blks_psect; j++) {
			UPDATE_TIME_STAMP((wbuf + TIME_STAMP_POS), &cur_time); /* 4 bytes time-stamp */
			UPDATE_WRITE_STAMP((wbuf + WRITE_STAMP_POS), write_stamping); /* Identify each write on same LBA sets uniquely */
			for (i = 0, cksum = 0; i < (OVERHEAD - 1); i++ ) {
				cksum += *((unsigned short *)wbuf + i);
			}
			UPDATE_CHECKSUM((wbuf + CHECKSUM_POS), cksum); /* 2 byte Check-Sum */
			wbuf += dev_info.blksize / (num_sub_blks_psect + 1);
		}
	}
	return 0;
}

/************************************************/
/* Function used by main thread to update the   */
/* information for cache threads.               */
/************************************************/
void update_cache_threads_info (struct thread_context *tctx)
{
    if (strcasecmp(tctx->oper, "CARR") == 0 || strcasecmp(tctx->oper, "CARW") == 0) {
        c_th_info[tctx->th_num].buf = tctx->rbuf;
    } else {
        c_th_info[tctx->th_num].buf = tctx->wbuf;
    }
    c_th_info[tctx->th_num].dlen = tctx->dlen;

    c_th_info[tctx->th_num].DMA_running = 1;
    c_th_info[tctx->th_num].cache_cond = 10;
}

/************************************************/
/* Function to wait for all cache threads to    */
/* complete once testcase is done.              */
/************************************************/
void wait_for_cache_threads_completion(struct htx_data *htx_ds, struct thread_context *tctx)
{
    int rc = 0;
    char err_str[ERR_STR_SZ], msg[128];;

    c_th_info[tctx->th_num].DMA_running = 0;
    c_th_info[tctx->th_num].testcase_done = 1;
    while (c_th_info[tctx->th_num].num_cache_threads_waiting != c_th_info[tctx->th_num].cache_threads_created) {
        usleep(10000);
    }
    rc = pthread_mutex_lock(&c_th_info[tctx->th_num].cache_mutex);
    if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "Mutex lock failed in function wait_for_cache_threads_completion, rc %d (%s)", rc, err_str);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(1);

    }
    rc = pthread_cond_broadcast(&c_th_info[tctx->th_num].do_oper_cond);
    if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "pthread_cond_broadcast failed in function wait_for_cache_threads_completion, rc %d (%s)", rc, err_str);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
    rc = pthread_mutex_unlock(&c_th_info[tctx->th_num].cache_mutex);
    if (rc) {
        STRERROR_R(rc, err_str, ERR_STR_SZ);
        sprintf(msg, "Mutex unlock failed in function wait_for_cache_threads_completion, rc %d (%s)", rc, err_str);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
    while (c_th_info[tctx->th_num].cache_threads_created > 0) {
        usleep(10000);
    }
}

char get_write_status(unsigned long long block_num)
{
    int byte_num, byte_offset;
    char value, *p;

    p = (char *) s_tbl.blk_write_status;
    byte_num = (int) (block_num / BLOCK_PER_BYTE);
    byte_offset = (block_num % BLOCK_PER_BYTE) * BITS_PER_BLOCK;
    value = (p[byte_num] >> ((BLOCK_PER_BYTE * BITS_PER_BLOCK) - byte_offset - BITS_PER_BLOCK)) & WRITE_STATUS_MASK;
    /* printf("get_write_status - value: %d\n", (int)value); */
    return value;
}

void set_write_status(unsigned long long  block_num, char value)
{
    int byte_num, byte_offset, mask;
    char *p;

    p = (char *) s_tbl.blk_write_status;
    byte_num = (int) (block_num / BLOCK_PER_BYTE);
    byte_offset = (block_num % BLOCK_PER_BYTE) * BITS_PER_BLOCK;
    mask = (~(WRITE_STATUS_MASK << ((BLOCK_PER_BYTE * BITS_PER_BLOCK) - byte_offset - BITS_PER_BLOCK))) & 0xFF;
    value = value << ((BLOCK_PER_BYTE * BITS_PER_BLOCK) - byte_offset - BITS_PER_BLOCK);
    /* printf("set_write_status - value: %d\n", (int)value); */
    p[byte_num] = (p[byte_num] & mask) | value;
    /* printf("set_write_status - value: %d\n", p[byte_num]); */
}

void update_state_table (unsigned long long block_num, int num_blks)
{
    int i;
    char write_status;

    for (i = 0; i < num_blks; i++) {
        write_status = get_write_status (block_num);
        write_status = (write_status + 1) % (1 << BITS_PER_BLOCK);
        if (write_status == 0) {
            write_status++;
        }
        set_write_status(block_num, write_status);
        block_num++;
    }
}
/************************************************************************************
* Function that calculates number of segment tables based on max threads and maxlba *
************************************************************************************/

void segment_table_init() {
	int i = 0;                      /* Loop Variable */
	unsigned long long prev = 0;    /* Temp Variable used to assign LBA Limits */
	int total_max_thread_cnt = 0;   /* Highest thread count defined in rule file stanza + Maximum BWRC thread count */
	unsigned long long seg_table_size[52];		/* Segment table size defined in terms of LBA count */

	total_max_thread_cnt = max_thread_cnt + total_BWRC_threads;

	/* SEG_TABLE_LIMIT refers to thread count. For Eg: If SEG_TABLE_LIMIT is 20,
	it means for every 20 threads we need to create a segment table */
	seg_info.num_of_seg_tables = (total_max_thread_cnt / SEG_TABLE_LIMIT);

	if( (total_max_thread_cnt % SEG_TABLE_LIMIT) != 0) {
		seg_info.num_of_seg_tables++;
	}

	seg_info.seg_table = (struct segment_table *)malloc((seg_info.num_of_seg_tables) * (sizeof(struct segment_table)));

	for(i = 0; i < seg_info.num_of_seg_tables; i++) {
	    	pthread_mutex_init(&((seg_info.seg_table + i)->segment_mutex), DEFAULT_MUTEX_ATTR_PTR);
		pthread_cond_init(&((seg_info.seg_table + i)->segment_do_oper), DEFAULT_COND_ATTR_PTR);
	}

	for(i = 0; i < seg_info.num_of_seg_tables; i++) {
		/* Average segment table size of individual segment tables */
		seg_table_size[i] = ((dev_info.maxblk + 1) / seg_info.num_of_seg_tables);
	}

	for(i = 0; i < ((dev_info.maxblk + 1) % (seg_info.num_of_seg_tables)); i++) {
		++seg_table_size[i];
	}

	for(i = 0; i < seg_info.num_of_seg_tables; i++) {
		(seg_info.seg_table + i)->LBA_lower_limit = prev;
		(seg_info.seg_table + i)->LBA_upper_limit = prev + seg_table_size[i] - 1;
		prev = prev + seg_table_size[i];
	}
}

#ifndef __CAPI_FLASH__
void update_aio_req_queue(int index, struct thread_context *tctx, char *buf)
{
    tctx->aio_req_queue[index].aio_req.aio_fildes = tctx->fd;
    tctx->aio_req_queue[index].aio_req.aio_offset = tctx->blkno[0] * dev_info.blksize;
    tctx->aio_req_queue[index].aio_req.aio_nbytes = tctx->dlen;
    tctx->aio_req_queue[index].aio_req.aio_buf = buf;
    tctx->aio_req_queue[index].status = AIO_INPROGRESS;
    tctx->aio_req_queue[index].op = tctx->oper[0];
}

int wait_for_aio_completion(struct htx_data *htx_ds, struct thread_context *tctx, char flag)
{
    int rc =0, index = 0;
    int i, err_no = 0;
    char msg[256];

    if (tctx->cur_async_io < tctx->num_async_io) {
        index = tctx->cur_async_io;
    } else {
        while (tctx->cur_async_io) {
            for (i = 0; i < tctx->num_async_io; i++) {
                if (tctx->aio_req_queue[i].status == AIO_INPROGRESS) {
                    rc = aio_error(&tctx->aio_req_queue[i].aio_req);
                    if (rc != AIO_INPROGRESS) {
                        tctx->cur_async_io--;
                        if (rc > 0) {
                            err_no = errno;
                            tctx->aio_req_queue[i].status = AIO_ERROR;
                            if (tctx->aio_req_queue[i].op == 'R') {
                                htx_ds->bad_reads++;
                            } else {
                                htx_ds->bad_writes++;
                            }
                            sprintf(msg, "error in aio_error(), errno: %d\n", errno);
                            user_msg(htx_ds, err_no, 0, HARD, msg);
                            index = -1;
                            return index;
                        } else if (rc == 0) { /* means IO completed */
                            tctx->aio_req_queue[i].status = AIO_COMPLETED;
                            rc = aio_return(&tctx->aio_req_queue[i].aio_req);
                            if (rc == -1) {
                                err_no = errno;
                                if (tctx->aio_req_queue[i].op == 'R') {
                                    htx_ds->bad_reads++;
                                } else {
                                    htx_ds->bad_writes++;
                                }
                                sprintf(msg, "error in aio_return, errno: %d\n", err_no);
                                user_msg(htx_ds, err_no, 0, HARD, msg);
                                index = -1;
                                return index;
                            } else if (rc != tctx->aio_req_queue[i].aio_req.aio_nbytes) {
                                sprintf(msg, "Attempted bytes is not equal to actual bytes\n");
                                user_msg(htx_ds, err_no, 0, HARD, msg);
                                index = -1;
                                return index;
                            } else {
                                if (tctx->aio_req_queue[i].op == 'R') {
                                    htx_ds->bytes_read += tctx->aio_req_queue[index].aio_req.aio_nbytes;
                                    htx_ds->good_reads++;
                                } else {
                                    htx_ds->bytes_writ += tctx->aio_req_queue[index].aio_req.aio_nbytes;
                                    htx_ds->good_writes++;
                                }
                            }
                            if (flag == AIO_SINGLE) {
                                index = i;
                                return index;
                            }
                        }
                    }
                }
            }
        }
    }
    return index;
}
#endif

