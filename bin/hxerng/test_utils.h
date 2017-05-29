#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_ 1


#if defined(__cplusplus)
extern "C" {
#endif

/*
 * AUTO DEFINES (DON'T TOUCH!)
 */

#ifndef	CSTRTD
typedef char *CSTRTD;
#endif
#ifndef	BSTRTD
typedef unsigned char *BSTRTD;
#endif

#ifndef	BYTE
typedef unsigned char BYTE;
#endif
#ifndef	UINT
typedef unsigned int UINT;
#endif
#ifndef	USHORT
typedef unsigned short USHORT;
#endif
#ifndef	ULONG
typedef unsigned long ULONG;
#endif
#ifndef	DIGIT
typedef USHORT DIGIT;	/* 16-bit word */
#endif
#ifndef	DBLWORD
typedef ULONG DBLWORD;  /* 32-bit word */
#endif

#ifndef	WORD64
typedef ULONG WORD64[2];  /* 64-bit word */
#endif

#if defined(__cplusplus)
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              M A C R O S
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define MAX(x,y)             ((x) <  (y)  ? (y)  : (x))
#define MIN(x,y)             ((x) >  (y)  ? (y)  : (x))
#define isNonPositive(x)     ((x) <= 0.e0 ?   1  : 0)
#define isPositive(x)        ((x) >  0.e0 ?   1 : 0)
#define isNegative(x)        ((x) <  0.e0 ?   1 : 0)
#define isGreaterThanOne(x)  ((x) >  1.e0 ?   1 : 0)
#define isZero(x)            ((x) == 0.e0 ?   1 : 0)
#define isOne(x)             ((x) == 1.e0 ?   1 : 0)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
                         G L O B A L  C O N S T A N T S
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define ALPHA							0.01	/* SIGNIFICANCE LEVEL */
#define MAXNUMOFTEMPLATES				148		/* APERIODIC TEMPLATES: 148=>temp_length=9 */
#define NUMOFTESTS						15		/* MAX TESTS DEFINED  */
#define NUMOFGENERATORS					10		/* MAX PRNGs */
#define MAXFILESPERMITTEDFORPARTITION	148
#define	TEST_FREQUENCY					1
#define	TEST_BLOCK_FREQUENCY			2
#define	TEST_CUSUM						3
#define	TEST_RUNS						4
#define	TEST_LONGEST_RUN				5
#define	TEST_RANK						6
#define	TEST_FFT						7
#define	TEST_NONPERIODIC				8
#define	TEST_OVERLAPPING				9
#define	TEST_UNIVERSAL					10
#define	TEST_APEN						11
#define	TEST_RND_EXCURSION				12
#define	TEST_RND_EXCURSION_VAR			13
#define	TEST_SERIAL						14
#define	TEST_LINEARCOMPLEXITY			15


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                   G L O B A L   D A T A  S T R U C T U R E S
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef unsigned char	BitSequence;


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
              U T I L I T Y  F U N C T I O N  P R O T O T Y P E S 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
int		displayGeneratorOptions(void);
int		generatorOptions(char** streamFile);
void	chooseTests(void);
void	fixParameters(void);
void	fileBasedBitStreams(char *streamFile);
void	readBinaryDigitsInASCIIFormat(FILE *fp, char *streamFile);
void	readHexDigitsInBinaryFormat(FILE *fp);
int		convertToBits(BYTE *x, int xBitLength, int bitsNeeded, int *num_0s, int *num_1s, int *bitsRead);
void	openOutputStreams(int option);
void	invokeTestSuite(int option, char *streamFile);
void	nist_test_suite(void);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
              C E P H E S  F U N C T I O N  P R O T O T Y P E S  A N D  D A T A 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double cephes_igamc(double a, double x);
double cephes_igam(double a, double x);
double cephes_lgam(double x);
double cephes_p1evl(double x, double *coef, int N);
double cephes_polevl(double x, double *coef, int N);
double cephes_erf(double x);
double cephes_erfc(double x);
double cephes_normal(double x);

static const double	rel_error = 1E-12;

static double MACHEP = 1.11022302462515654042E-16;		/* 2**-53 */
static double MAXLOG = 7.09782712893383996732224E2;		/* log(MAXNUM) */
static double MAXNUM = 1.7976931348623158E308;			/* 2**1024*(1-MACHEP) */
static double PI     = 3.14159265358979323846;			/* pi, duh! */

static double big = 4.503599627370496e15;
static double biginv =  2.22044604925031308085e-16;

static int sgngam = 0;

static unsigned short A[] = {
	0x6661,0x2733,0x9850,0x3f4a,
	0xe943,0xb580,0x7fbd,0xbf43,
	0x5ebb,0x20dc,0x019f,0x3f4a,
	0xa5a1,0x16b0,0xc16c,0xbf66,
	0x554b,0x5555,0x5555,0x3fb5
};
static unsigned short B[] = {
	0x6761,0x8ff3,0x8901,0xc095,
	0xb93e,0x355b,0xf234,0xc0e2,
	0x89e5,0xf890,0x3d73,0xc114,
	0xdb51,0xf994,0xbc82,0xc131,
	0xf20b,0x0219,0x4589,0xc13a,
	0x055e,0x5418,0x0c67,0xc12a
};
static unsigned short C[] = {
	/*0x0000,0x0000,0x0000,0x3ff0,*/
	0x12b2,0x1cf3,0xfd0d,0xc075,
	0xd757,0x7b89,0xaa0d,0xc0d0,
	0x4c9b,0xb974,0xeb84,0xc10a,
	0x0043,0x7195,0x6286,0xc131,
	0xf34c,0x892f,0x5255,0xc143,
	0xe14a,0x6a11,0xce4b,0xc13e
};

#define MAXLGM 2.556348e305

 

 
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   A P P R O X I M A T E  E N T R O P Y   T E S T  P R O T O T Y P E 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double ApproximateEntropy(char *epsilon, int n, int m);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        B L O C K  F R E Q U E N C Y  T E S T   T E S T  P R O T O T Y P E
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double BlockFrequency(char *epsilon, int n, int M);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
           C U M U L A T I V E  S U M S  T E S T   P R O T O T Y P E
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
double CumulativeSums(char *epsilon, int n);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                      D F T   T E S T      P R O T O T Y P E
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void __ogg_fdrfftf(int n,double *r,double *wsave,int *ifac);
void __ogg_fdcosqf(int n,double *x,double *wsave,int *ifac);
void __ogg_fdrffti(int n, double *wsave, int *ifac);
void __ogg_fdcosqi(int n, double *wsave, int *ifac);
void __ogg_fdrfftb(int n, double *r, double *wsave, int *ifac);
void __ogg_fdcosqb(int n,double *x,double *wsave,int *ifac);
static void drftb1(int n, double *c, double *ch, double *wa, int *ifac);
static void dradb2(int ido,int l1,double *cc,double *ch,double *wa1);
static void dcsqb1(int n,double *x,double *w,double *xh,int *ifac);
static void dradb3(int ido,int l1,double *cc,double *ch,double *wa1, double *wa2);
static void dradb4(int ido,int l1,double *cc,double *ch,double *wa1, double *wa2,double *wa3);
static void dradf2(int ido,int l1,double *cc,double *ch,double *wa1);
static void dradf4(int ido,int l1,double *cc,double *ch,double *wa1, double *wa2,double *wa3);
static void dradfg(int ido,int ip,int l1,int idl1,double *cc,double *c1,double *c2,double *ch,double *ch2,double *wa);
static void dradbg(int ido,int ip,int l1,int idl1,double *cc,double *c1,double *c2,double *ch,double *ch2,double *wa);
static void drftf1(int n,double *c,double *ch,double *wa,int *ifac);
static void dcsqf1(int n,double *x,double *w,double *xh,int *ifac);

double DiscreteFourierTransform(char *epsilon, int n);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
              F R E Q U E N C Y  T E S T  P R O T O T Y P E
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
double Frequency(char *epsilon, int n);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
      L I N E A R  C O M P L E X I T Y   T E S T  P R O T O T Y P E
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
double LinearComplexity(char *epsilon, int n, int M);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                      L O N G E S T  R U N S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
double LongestRunOfOnes(char *epsilon, int n);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
R A N K  A L G O R I T H M  R O U T I N E S
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define	MATRIX_FORWARD_ELIMINATION	0
#define	MATRIX_BACKWARD_ELIMINATION	1
int computeRank(int M, int Q, BitSequence **matrix);
void perform_elementary_row_operations(int flag, int i, int M, int Q, BitSequence **A);
int find_unit_element_and_swap(int flag, int i, int M, int Q, BitSequence **A);
int swap_rows(int i, int index, int Q, BitSequence **A);
int determine_rank(int m, int M, int Q, BitSequence **A);
BitSequence** create_matrix(int M, int Q);
void def_matrix(char *epsilon, int M, int Q, BitSequence **m,int k);
void delete_matrix(int M, BitSequence **matrix);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
          N O N O V E R L A P P I N G  T E M P L A T E  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double NonOverlappingTemplateMatchings(char * epsilon, int n, int m);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
               O V E R L A P P I N G  T E M P L A T E  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double OverlappingTemplateMatchings(char *epsilon, int n, int m);
double Pr(int u, double eta);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                     R A N D O M  E X C U R S I O N S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double RandomExcursions(char * epsilon, int n);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            R A N D O M  E X C U R S I O N S  V A R I A N T  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double RandomExcursionsVariant(char *epsilon, int n);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              R A N K  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Rank(char *epsilon, int n);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              R U N S  T E S T 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Runs(char *epsilon, int n);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              S E R I A L  T E S T 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Serial(char *epsilon, int n, int m);
double psi2(char *epsilon, int m, int n);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                         U N I V E R S A L  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Universal(char *epsilon, int n);


#endif /* _TEST_UTILS_H */
