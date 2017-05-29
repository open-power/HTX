/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* htx72F src/htx/usr/lpp/htx/inc/hxerng.h 1.3                            */
/*                                                                        */
/* Licensed Materials - Property of IBM                                   */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2011,2017              */
/* All Rights Reserved                                                    */
/*                                                                        */
/* US Government Users Restricted Rights - Use, duplication or            */
/* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.      */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/* %Z%%M%       %I%  %W% %G% %U%                                          */

#ifndef _HXERNG_H_
#define _HXERNG_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

#ifndef __HTX_LINUX__
#include <sys/systemcfg.h>
#include "hcall.h"
#endif

/* HTX related header files */
#include "hxihtx64.h"

/* Random testing function related header files
#include "test_utils.h" */

#ifdef __HTX_LINUX__
#else
#endif

struct dev_info {
	char device_name[50];
	char run_type[4];
	char rule_file_name[200];
} dinfo;

#define		TRUE		1
#define		FALSE		0

#define		NUM_TESTS	15
#define		EXTRA_THS	2

#ifdef DEBUG
#define DPRINT(x, format, ...) fprintf(x, format, ##__VA_ARGS__)
#define FFLUSH(x) fflush(x)
#else
#define DPRINT(x, format, ...)
#define FFLUSH(x)
#endif

/* Definition of all types of tests */
#define		READ_RNO							0
#define		FREQUENCY							1
#define		BLOCK_FREQUENCY						2
#define		RUNS								3
#define		LONGEST_RUN_OF_ONES					4
#define		MATRIX_RANK							5
#define		DISCRETE_FOURIER_TRANSFORM			6
#define		NON_OVERLAPPING_TEMPLATE_MATCHING	7
#define		OVERLAPPING_TEMPLATE_MATCHING		8
#define		UNIVERSAL							9
#define		APPROXIMATE_ENTROPY					10
#define		CUMULATIVE_SUM						11
#define		RANDOM_EXCURSIONS					12
#define		RANDOM_EXCURSIONS_VARIANT			13
#define		SERIAL								14
#define		LINEAR_COMPLEXITY					15
#define		RNG_RETRY							50

/* Shared seg key */
#define		BIT_SHM_KEY			0x10010000
#define		BYTE_SHM_KEY		0x10020000


/* Random number stream size */
#define		STREAM_SIZE			1048576
#define		MAX_SLOTS			64

/* Maximum number of rules supported */
#define 	MAX_NUM_RULES		10
#define		MAX_RULE_LINE_SIZE	150

/* Rule file parameters */
struct rule_parameters {
	char rule_name[20];
	int stream_len;
	int blockFrequency_block_len;
	int nonOverlappingTemplate_block_len;
	int overlappingTemplate_block_len;
	int approxEntropy_block_len;
	int serial_block_len;
	int linearComplexity_sequence_len;
	int tests_to_run[NUM_TESTS];
	int num_oper;
	char rnd_num_type[20];
	char rnd_32[5];
};

/* function pointer for all the client functions */
typedef int (*worker)(void *);

void sigterm_hdl(void);
void sigusr1_hdl(void);
void clean_up(void);
int check_rng_available(void);
int allocate_mem(void);
int parse_line(char *);
int get_line( char *, int, FILE *, char, int *);
int get_rule(int *, FILE *, struct rule_parameters *);
int get_PVR(void);
int read_rf(void);
int apply_rf_default(void);

unsigned long long read_rn(void);
void read_rn_32_bit(unsigned long long *temp);
void read_rn_64_bit(unsigned long long *temp);
void read_rn_raw(unsigned long long *temp);
void read_rn_for_p8_and_earlier(unsigned long long *temp);
int read_rn_func(void *);
int convert_bit_2_byte(void *);
extern int get_cpu_version(void);
extern int get_true_cpu_version(void);

#ifdef __HTX_OPEN_SOURCE__
double Frequency(char *, int);
double BlockFrequency(char *, int, int);
double Runs(char *, int);
double Serial(char *, int, int);
double Universal(char *, int);
double LongestRunOfOnes(char *, int);
double Rank(char *, int);
double LinearComplexity(char *, int, int);
double DiscreteFourierTransform(char *, int);
double CumulativeSums(char *, int);
double ApproximateEntropy(char *, int, int);
double NonOverlappingTemplateMatchings(char *, int, int);
double OverlappingTemplateMatchings(char *, int, int);
double RandomExcursions(char *, int);
double RandomExcursionsVariant(char *, int);
int freq(void *);
int block_freq(void *);
int runs(void *);
int longest_run_of_ones(void *);
int rank(void *);
int dft(void *);
int non_overlapping_tm(void *);
int overlapping_tm(void *);
int universal(void *);
int approx_entropy(void *);
int cusum(void *);
int random_excursion(void *);
int random_excursion_var(void *);
int serial(void *);
int linear_complexity(void *);
#endif

int h_cop_random_k(caddr_t, caddr_t);
extern unsigned long long read_rn_32(void);
extern unsigned long long read_rn_64_cond(void);
extern unsigned long long read_rn_64_raw(void);

/* Global string to print messages */
char msg[1000];

#endif /* _HXERNG_H_ */
