#include "test_utils.h"  

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                A P P R O X I M A T E  E N T R O P Y   T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double ApproximateEntropy(char *epsilon, int n, int m)
{
	int				i, j, k, r, blockSize, seqLength, powLen, index;
	double			sum, numOfBlocks, ApEn[2], apen, chi_squared, p_value;
	unsigned int	*P;
	
	seqLength = n;
	r = 0;
	
	for ( blockSize=m; blockSize<=m+1; blockSize++ ) {
		if ( blockSize == 0 ) {
			ApEn[0] = 0.00;
			r++;
		}
		else {
			numOfBlocks = (double)seqLength;
			powLen = (int)pow(2, blockSize+1)-1;
			if ( (P = (unsigned int*)calloc(powLen,sizeof(unsigned int)))== NULL ) {
#if 0
				sprintf("Approximate Entropy:Insufficient Memory for Work Space\n");
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
				return(1);
			}
			for ( i=1; i<powLen-1; i++ )
				P[i] = 0;
			for ( i=0; i<numOfBlocks; i++ ) { /* COMPUTE FREQUENCY */
				k = 1;
				for ( j=0; j<blockSize; j++ ) {
					k <<= 1;
					if ( (int)epsilon[(i+j) % seqLength] == 1 )
						k++;
				}
				P[k-1]++;
			}
			/* DISPLAY FREQUENCY */
			sum = 0.0;
			index = (int)pow(2, blockSize)-1;
			for ( i=0; i<(int)pow(2, blockSize); i++ ) {
				if ( P[index] > 0 )
					sum += P[index]*log(P[index]/numOfBlocks);
				index++;
			}
			sum /= numOfBlocks;
			ApEn[r] = sum;
			r++;
			free(P);
		}
	}
	apen = ApEn[0] - ApEn[1];
	
	chi_squared = 2.0*seqLength*(log(2) - apen);
	p_value = cephes_igamc(pow(2, m-1), chi_squared/2.0);
	
#if 0
	if ( m > (int)(log(seqLength)/log(2)-5) ) {
		fprintf(stats[TEST_APEN], "\t\tNote: The blockSize = %d exceeds recommended value of %d\n", m,
			MAX(1, (int)(log(seqLength)/log(2)-5)));
		fprintf(stats[TEST_APEN], "\t\tResults are inaccurate!\n");
		fprintf(stats[TEST_APEN], "\t\t--------------------------------------------\n");
	}
#endif
	
	return( p_value < ALPHA ? (p_value) : 0 );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                    B L O C K  F R E Q U E N C Y  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double BlockFrequency(char *epsilon, int n, int M)
{
	int		i, j, N, blockSum;
	double	p_value, sum, pi, v, chi_squared;
	
	N = n/M; 		/* # OF SUBSTRING BLOCKS      */
	sum = 0.0;
	
	for ( i=0; i<N; i++ ) {
		blockSum = 0;
		for ( j=0; j<M; j++ )
			blockSum += epsilon[j+i*M];
		pi = (double)blockSum/(double)M;
		v = pi - 0.5;
		sum += v*v;
	}
	chi_squared = 4.0 * M * sum;
	p_value = cephes_igamc(N/2.0, chi_squared/2.0);

	return( p_value < ALPHA ? (p_value) : 0 );
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                                         C E P H E S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double cephes_igamc(double a, double x)
{
	double ans, ax, c, yc, r, t, y, z;
	double pk, pkm1, pkm2, qk, qkm1, qkm2;

	if ( (x <= 0) || ( a <= 0) )
		return( 1.0 );

	if ( (x < 1.0) || (x < a) )
		return( 1.e0 - cephes_igam(a,x) );

	ax = a * log(x) - x - cephes_lgam(a);

	if ( ax < -MAXLOG ) {
		/* printf("igamc: UNDERFLOW\n"); */
		return 0.0;
	}
	ax = exp(ax);

	/* continued fraction */
	y = 1.0 - a;
	z = x + y + 1.0;
	c = 0.0;
	pkm2 = 1.0;
	qkm2 = x;
	pkm1 = x + 1.0;
	qkm1 = z * x;
	ans = pkm1/qkm1;

	do {
		c += 1.0;
		y += 1.0;
		z += 2.0;
		yc = y * c;
		pk = pkm1 * z  -  pkm2 * yc;
		qk = qkm1 * z  -  qkm2 * yc;
		if ( qk != 0 ) {
			r = pk/qk;
			t = fabs( (ans - r)/r );
			ans = r;
		}
		else
			t = 1.0;
		pkm2 = pkm1;
		pkm1 = pk;
		qkm2 = qkm1;
		qkm1 = qk;
		if ( fabs(pk) > big ) {
			pkm2 *= biginv;
			pkm1 *= biginv;
			qkm2 *= biginv;
			qkm1 *= biginv;
		}
	} while ( t > MACHEP );

	return ans*ax;
}

double cephes_igam(double a, double x)
{
	double ans, ax, c, r;

	if ( (x <= 0) || ( a <= 0) )
		return 0.0;

	if ( (x > 1.0) && (x > a ) )
		return 1.e0 - cephes_igamc(a,x);

	/* Compute  x**a * exp(-x) / gamma(a)  */
	ax = a * log(x) - x - cephes_lgam(a);
	if ( ax < -MAXLOG ) {
		return 0.0;
	}
	ax = exp(ax);

	/* power series */
	r = a;
	c = 1.0;
	ans = 1.0;

	do {
		r += 1.0;
		c *= x/r;
		ans += c;
	} while ( c/ans > MACHEP );

	return ans * ax/a;
}

/* Logarithm of gamma function */
double cephes_lgam(double x)
{
	double	p, q, u, w, z;
	int		i;

	sgngam = 1;

	if ( x < -34.0 ) {
		q = -x;
		w = cephes_lgam(q); /* note this modifies sgngam! */
		p = floor(q);
		if ( p == q ) {
lgsing:
			goto loverf;
		}
		i = (int)p;
		if ( (i & 1) == 0 )
			sgngam = -1;
		else
			sgngam = 1;
		z = q - p;
		if ( z > 0.5 ) {
			p += 1.0;
			z = p - q;
		}
		z = q * sin( PI * z );
		if ( z == 0.0 )
			goto lgsing;
		/*      z = log(PI) - log( z ) - w;*/
		z = log(PI) - log( z ) - w;
		return z;
	}

	if ( x < 13.0 ) {
		z = 1.0;
		p = 0.0;
		u = x;
		while ( u >= 3.0 ) {
			p -= 1.0;
			u = x + p;
			z *= u;
		}
		while ( u < 2.0 ) {
			if ( u == 0.0 )
				goto lgsing;
			z /= u;
			p += 1.0;
			u = x + p;
		}
		if ( z < 0.0 ) {
			sgngam = -1;
			z = -z;
		}
		else
			sgngam = 1;
		if ( u == 2.0 )
			return( log(z) );
		p -= 2.0;
		x = x + p;
		p = x * cephes_polevl( x, (double *)B, 5 ) / cephes_p1evl( x, (double *)C, 6);

		return log(z) + p;
	}

	if ( x > MAXLGM ) {
loverf:

		return sgngam * MAXNUM;
	}

	q = ( x - 0.5 ) * log(x) - x + log( sqrt( 2*PI ) );
	if ( x > 1.0e8 )
		return q;

	p = 1.0/(x*x);
	if ( x >= 1000.0 )
		q += ((   7.9365079365079365079365e-4 * p
		        - 2.7777777777777777777778e-3) *p
				+ 0.0833333333333333333333) / x;
	else
		q += cephes_polevl( p, (double *)A, 4 ) / x;

	return q;
}

double cephes_polevl(double x, double *coef, int N)
{
	double	ans;
	int		i;
	double	*p;

	p = coef;
	ans = *p++;
	i = N;

	do
		ans = ans * x  +  *p++;
	while ( --i );

	return ans;
}

double cephes_p1evl(double x, double *coef, int N)
{
	double	ans;
	double	*p;
	int		i;

	p = coef;
	ans = x + *p++;
	i = N-1;

	do
		ans = ans * x  + *p++;
	while ( --i );

	return ans;
}

double cephes_erf(double x)
{
	static const double two_sqrtpi = 1.128379167095512574;
	double	sum = x, term = x, xsqr = x * x;
	int		j = 1;

	if ( fabs(x) > 2.2 )
		return 1.0 - cephes_erfc(x);

	do {
		term *= xsqr/j;
		sum -= term/(2*j+1);
		j++;
		term *= xsqr/j;
		sum += term/(2*j+1);
		j++;
	} while ( fabs(term)/sum > rel_error );

	return two_sqrtpi*sum;
}

double cephes_erfc(double x)
{
	static const double one_sqrtpi = 0.564189583547756287;
	double	a = 1, b = x, c = x, d = x*x + 0.5;
	double	q1, q2 = b/d, n = 1.0, t;

	if ( fabs(x) < 2.2 )
		return 1.0 - cephes_erf(x);
	if ( x < 0 )
		return 2.0 - cephes_erfc(-x);

	do {
		t = a*n + b*x;
		a = b;
		b = t;
		t = c*n + d*x;
		c = d;
		d = t;
		n += 0.5;
		q1 = q2;
		q2 = b/d;
	} while ( fabs(q1-q2)/q2 > rel_error );

	return one_sqrtpi*exp(-x*x)*q2;
}


double cephes_normal(double x)
{
	double arg, result, sqrt2=1.414213562373095048801688724209698078569672;

	if (x > 0) {
		arg = x/sqrt2;
		result = 0.5 * ( 1 + erf(arg) );
	}
	else {
		arg = -x/sqrt2;
		result = 0.5 * ( 1 - erf(arg) );
	}

	return( result);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                           C U M U L A T I V E  S U M S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double CumulativeSums(char *epsilon, int n)
{
	int		S, sup, inf, z, zrev, k;
	double	sum1, sum2, p_value_forward, p_value_backward;

	S = 0;
	sup = 0;
	inf = 0;
	for ( k=0; k<n; k++ ) {
		epsilon[k] ? S++ : S--;
		if ( S > sup )
			sup++;
		if ( S < inf )
			inf--;
		z = (sup > -inf) ? sup : -inf;
		zrev = (sup-S > S-inf) ? sup-S : S-inf;
	}
	
	// forward
	sum1 = 0.0;
	for ( k=(-n/z+1)/4; k<=(n/z-1)/4; k++ ) {
		sum1 += cephes_normal(((4*k+1)*z)/sqrt(n));
		sum1 -= cephes_normal(((4*k-1)*z)/sqrt(n));
	}
	sum2 = 0.0;
	for ( k=(-n/z-3)/4; k<=(n/z-1)/4; k++ ) {
		sum2 += cephes_normal(((4*k+3)*z)/sqrt(n));
		sum2 -= cephes_normal(((4*k+1)*z)/sqrt(n));
	}

	p_value_forward = 1.0 - sum1 + sum2;
	
	if ( isNegative(p_value_forward) || isGreaterThanOne(p_value_forward) ) {
#if 0
		sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
	}

		
	// backwards
	sum1 = 0.0;
	for ( k=(-n/zrev+1)/4; k<=(n/zrev-1)/4; k++ ) {
		sum1 += cephes_normal(((4*k+1)*zrev)/sqrt(n));
		sum1 -= cephes_normal(((4*k-1)*zrev)/sqrt(n));
	}
	sum2 = 0.0;
	for ( k=(-n/zrev-3)/4; k<=(n/zrev-1)/4; k++ ) {
		sum2 += cephes_normal(((4*k+3)*zrev)/sqrt(n));
		sum2 -= cephes_normal(((4*k+1)*zrev)/sqrt(n));
	}
	p_value_backward = 1.0 - sum1 + sum2;

	if ( isNegative(p_value_backward) || isGreaterThanOne(p_value_backward) ) {
#if 0
		sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
	}


	return( ((p_value_forward < ALPHA) || (p_value_backward < ALPHA)) ? (p_value_forward) : 0 );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         D I S C R E T E  F O U R I E R  T R A N S F O R M  T E S T 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Notes from RFB: 

	 Looks like the user-level routines are:

	 Real FFT

	 void __ogg_fdrffti(int n, double *wsave, int *ifac)
	 void __ogg_fdrfftf(int n,double *r,double *wsave,int *ifac)
	 void __ogg_fdrfftb(int n, double *r, double *wsave, int *ifac)

		 __ogg_fdrffti == initialization
		 __ogg_fdrfftf == forward transform
		 __ogg_fdrfftb == backward transform

		 Parameters are
		 n == length of sequence
		 r == sequence to be transformed (input)
			 == transformed sequence (output)
		 wsave == work array of length 2n (allocated by caller)
		 ifac == work array of length 15 (allocated by caller)

	 Cosine quarter-wave FFT

	 void __ogg_fdcosqi(int n, double *wsave, int *ifac)
	 void __ogg_fdcosqf(int n,double *x,double *wsave,int *ifac)
	 void __ogg_fdcosqb(int n,double *x,double *wsave,int *ifac)
*/

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggSQUISH SOFTWARE CODEC SOURCE CODE.*
 *                                                                  * 
 ********************************************************************

	file: fft.c
	function: Fast discrete Fourier and cosine transforms and inverses
	author: Monty <xiphmont@mit.edu>
	modifications by: Monty
	last modification date: Jul 1 1996

 ********************************************************************/

/* These Fourier routines were originally based on the Fourier
	 routines of the same names from the NETLIB bihar and fftpack
	 fortran libraries developed by Paul N. Swarztrauber at the National
	 Center for Atmospheric Research in Boulder, CO USA.	They have been
	 reimplemented in C and optimized in a few ways for OggSquish. */

/* As the original fortran libraries are public domain, the C Fourier
	 routines in this file are hereby released to the public domain as
	 well.	The C routines here produce output exactly equivalent to the
	 original fortran routines.  Of particular interest are the facts
	 that (like the original fortran), these routines can work on
	 arbitrary length vectors that need not be powers of two in
	 length. */

#define STIN static

static void drfti1(int n, double *wa, int *ifac){
	static int ntryh[4] = { 4,2,3,5 };
	static double tpi = 6.28318530717958647692528676655900577;
	double arg,argh,argld,fi;
	int ntry=0,i,j=-1;
	int k1, l1, l2, ib;
	int ld, ii, ip, is, nq, nr;
	int ido, ipm, nfm1;
	int nl=n;
	int nf=0;

 L101:
	j++;
	if (j < 4)
		ntry=ntryh[j];
	else
		ntry+=2;

 L104:
	nq=nl/ntry;
	nr=nl-ntry*nq;
	if (nr!=0) goto L101;

	nf++;
	ifac[nf+1]=ntry;
	nl=nq;
	if(ntry!=2)goto L107;
	if(nf==1)goto L107;

	for (i=1;i<nf;i++){
		ib=nf-i+1;
		ifac[ib+1]=ifac[ib];
	}
	ifac[2] = 2;

 L107:
	if(nl!=1)goto L104;
	ifac[0]=n;
	ifac[1]=nf;
	argh=tpi/n;
	is=0;
	nfm1=nf-1;
	l1=1;

	if(nfm1==0)return;

	for (k1=0;k1<nfm1;k1++){
		ip=ifac[k1+2];
		ld=0;
		l2=l1*ip;
		ido=n/l2;
		ipm=ip-1;

		for (j=0;j<ipm;j++){
			ld+=l1;
			i=is;
			argld=(double)ld*argh;
			fi=0.;
			for (ii=2;ii<ido;ii+=2){
				fi+=1.;
				arg=fi*argld;
				wa[i++]=cos(arg);
				wa[i++]=sin(arg);
			}
			is+=ido;
		}
		l1=l2;
	}
}

void __ogg_fdrffti(int n, double *wsave, int *ifac)
{

	if (n == 1) return;
	drfti1(n, wsave+n, ifac);
}

void __ogg_fdcosqi(int n, double *wsave, int *ifac)
{
	static double pih = 1.57079632679489661923132169163975;
	static int k;
	static double fk, dt;

	dt=pih/n;
	fk=0.;
	for(k=0;k<n;k++){
		fk+=1.;
		wsave[k] = cos(fk*dt);
	}

	__ogg_fdrffti(n, wsave+n,ifac);
}

STIN void dradf2(int ido,int l1,double *cc,double *ch,double *wa1)
{
	int i,k;
	double ti2,tr2;
	int t0,t1,t2,t3,t4,t5,t6;
	
	t1=0;
	t0=(t2=l1*ido);
	t3=ido<<1;
	for(k=0;k<l1;k++){
		ch[t1<<1]=cc[t1]+cc[t2];
		ch[(t1<<1)+t3-1]=cc[t1]-cc[t2];
		t1+=ido;
		t2+=ido;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	t2=t0;
	for(k=0;k<l1;k++){
		t3=t2;
		t4=(t1<<1)+(ido<<1);
		t5=t1;
		t6=t1+t1;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4-=2;
			t5+=2;
			t6+=2;
			tr2=wa1[i-2]*cc[t3-1]+wa1[i-1]*cc[t3];
			ti2=wa1[i-2]*cc[t3]-wa1[i-1]*cc[t3-1];
			ch[t6]=cc[t5]+ti2;
			ch[t4]=ti2-cc[t5];
			ch[t6-1]=cc[t5-1]+tr2;
			ch[t4-1]=cc[t5-1]-tr2;
		}
		t1+=ido;
		t2+=ido;
	}

	if(ido%2==1)return;

 L105:
	t3=(t2=(t1=ido)-1);
	t2+=t0;
	for(k=0;k<l1;k++){
		ch[t1]=-cc[t2];
		ch[t1-1]=cc[t3];
		t1+=ido<<1;
		t2+=ido;
		t3+=ido;
	}
}

STIN void dradf4(int ido,int l1,double *cc,double *ch,double *wa1, double *wa2,double *wa3)
{
	static double hsqt2 = .70710678118654752440084436210485;
	int i,k,t0,t1,t2,t3,t4,t5,t6;
	double ci2,ci3,ci4,cr2,cr3,cr4,ti1,ti2,ti3,ti4,tr1,tr2,tr3,tr4;
	t0=l1*ido;
	
	t1=t0;
	t4=t1<<1;
	t2=t1+(t1<<1);
	t3=0;

	for(k=0;k<l1;k++){
		tr1=cc[t1]+cc[t2];
		tr2=cc[t3]+cc[t4];
		ch[t5=t3<<2]=tr1+tr2;
		ch[(ido<<2)+t5-1]=tr2-tr1;
		ch[(t5+=(ido<<1))-1]=cc[t3]-cc[t4];
		ch[t5]=cc[t2]-cc[t1];

		t1+=ido;
		t2+=ido;
		t3+=ido;
		t4+=ido;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	for(k=0;k<l1;k++){
		t2=t1;
		t4=t1<<2;
		t5=(t6=ido<<1)+t4;
		for(i=2;i<ido;i+=2){
			t3=(t2+=2);
			t4+=2;
			t5-=2;

			t3+=t0;
			cr2=wa1[i-2]*cc[t3-1]+wa1[i-1]*cc[t3];
			ci2=wa1[i-2]*cc[t3]-wa1[i-1]*cc[t3-1];
			t3+=t0;
			cr3=wa2[i-2]*cc[t3-1]+wa2[i-1]*cc[t3];
			ci3=wa2[i-2]*cc[t3]-wa2[i-1]*cc[t3-1];
			t3+=t0;
			cr4=wa3[i-2]*cc[t3-1]+wa3[i-1]*cc[t3];
			ci4=wa3[i-2]*cc[t3]-wa3[i-1]*cc[t3-1];

			tr1=cr2+cr4;
			tr4=cr4-cr2;
			ti1=ci2+ci4;
			ti4=ci2-ci4;
			ti2=cc[t2]+ci3;
			ti3=cc[t2]-ci3;
			tr2=cc[t2-1]+cr3;
			tr3=cc[t2-1]-cr3;

			
			ch[t4-1]=tr1+tr2;
			ch[t4]=ti1+ti2;

			ch[t5-1]=tr3-ti4;
			ch[t5]=tr4-ti3;

			ch[t4+t6-1]=ti4+tr3;
			ch[t4+t6]=tr4+ti3;

			ch[t5+t6-1]=tr2-tr1;
			ch[t5+t6]=ti1-ti2;
		}
		t1+=ido;
	}
	if(ido%2==1)return;

 L105:
	
	t2=(t1=t0+ido-1)+(t0<<1);
	t3=ido<<2;
	t4=ido;
	t5=ido<<1;
	t6=ido;

	for(k=0;k<l1;k++){
		ti1=-hsqt2*(cc[t1]+cc[t2]);
		tr1=hsqt2*(cc[t1]-cc[t2]);
		ch[t4-1]=tr1+cc[t6-1];
		ch[t4+t5-1]=cc[t6-1]-tr1;
		ch[t4]=ti1-cc[t1+t0];
		ch[t4+t5]=ti1+cc[t1+t0];
		t1+=ido;
		t2+=ido;
		t4+=t3;
		t6+=ido;
	}
}

STIN void dradfg(int ido,int ip,int l1,int idl1,double *cc,double *c1,
				double *c2,double *ch,double *ch2,double *wa)
{

	static double tpi=6.28318530717958647692528676655900577;
	int idij,ipph,i,j,k,l,ic,ik,is;
	int t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;
	double dc2,ai1,ai2,ar1,ar2,ds2;
	int nbd;
	double dcp,arg,dsp,ar1h,ar2h;
	int idp2,ipp2;
	
	arg=tpi/(double)ip;
	dcp=cos(arg);
	dsp=sin(arg);
	ipph=(ip+1)>>1;
	ipp2=ip;
	idp2=ido;
	nbd=(ido-1)>>1;
	t0=l1*ido;
	t10=ip*ido;

	if(ido==1)goto L119;
	for(ik=0;ik<idl1;ik++)ch2[ik]=c2[ik];

	t1=0;
	for(j=1;j<ip;j++){
		t1+=t0;
		t2=t1;
		for(k=0;k<l1;k++){
			ch[t2]=c1[t2];
			t2+=ido;
		}
	}

	is=-ido;
	t1=0;
	if(nbd>l1){
		for(j=1;j<ip;j++){
			t1+=t0;
			is+=ido;
			t2= -ido+t1;
			for(k=0;k<l1;k++){
				idij=is-1;
				t2+=ido;
				t3=t2;
				for(i=2;i<ido;i+=2){
					idij+=2;
					t3+=2;
					ch[t3-1]=wa[idij-1]*c1[t3-1]+wa[idij]*c1[t3];
					ch[t3]=wa[idij-1]*c1[t3]-wa[idij]*c1[t3-1];
				}
			}
		}
	}else{

		for(j=1;j<ip;j++){
			is+=ido;
			idij=is-1;
			t1+=t0;
			t2=t1;
			for(i=2;i<ido;i+=2){
				idij+=2;
				t2+=2;
				t3=t2;
				for(k=0;k<l1;k++){
					ch[t3-1]=wa[idij-1]*c1[t3-1]+wa[idij]*c1[t3];
					ch[t3]=wa[idij-1]*c1[t3]-wa[idij]*c1[t3-1];
					t3+=ido;
				}
			}
		}
	}

	t1=0;
	t2=ipp2*t0;
	if(nbd<l1){
		for(j=1;j<ipph;j++){
			t1+=t0;
			t2-=t0;
			t3=t1;
			t4=t2;
			for(i=2;i<ido;i+=2){
				t3+=2;
				t4+=2;
				t5=t3-ido;
				t6=t4-ido;
				for(k=0;k<l1;k++){
					t5+=ido;
					t6+=ido;
					c1[t5-1]=ch[t5-1]+ch[t6-1];
					c1[t6-1]=ch[t5]-ch[t6];
					c1[t5]=ch[t5]+ch[t6];
					c1[t6]=ch[t6-1]-ch[t5-1];
				}
			}
		}
	}else{
		for(j=1;j<ipph;j++){
			t1+=t0;
			t2-=t0;
			t3=t1;
			t4=t2;
			for(k=0;k<l1;k++){
				t5=t3;
				t6=t4;
				for(i=2;i<ido;i+=2){
					t5+=2;
					t6+=2;
					c1[t5-1]=ch[t5-1]+ch[t6-1];
					c1[t6-1]=ch[t5]-ch[t6];
					c1[t5]=ch[t5]+ch[t6];
					c1[t6]=ch[t6-1]-ch[t5-1];
				}
				t3+=ido;
				t4+=ido;
			}
		}
	}

L119:
	for(ik=0;ik<idl1;ik++)c2[ik]=ch2[ik];

	t1=0;
	t2=ipp2*idl1;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1-ido;
		t4=t2-ido;
		for(k=0;k<l1;k++){
			t3+=ido;
			t4+=ido;
			c1[t3]=ch[t3]+ch[t4];
			c1[t4]=ch[t4]-ch[t3];
		}
	}

	ar1=1.;
	ai1=0.;
	t1=0;
	t2=ipp2*idl1;
	t3=(ip-1)*idl1;
	for(l=1;l<ipph;l++){
		t1+=idl1;
		t2-=idl1;
		ar1h=dcp*ar1-dsp*ai1;
		ai1=dcp*ai1+dsp*ar1;
		ar1=ar1h;
		t4=t1;
		t5=t2;
		t6=t3;
		t7=idl1;

		for(ik=0;ik<idl1;ik++){
			ch2[t4++]=c2[ik]+ar1*c2[t7++];
			ch2[t5++]=ai1*c2[t6++];
		}

		dc2=ar1;
		ds2=ai1;
		ar2=ar1;
		ai2=ai1;

		t4=idl1;
		t5=(ipp2-1)*idl1;
		for(j=2;j<ipph;j++){
			t4+=idl1;
			t5-=idl1;

			ar2h=dc2*ar2-ds2*ai2;
			ai2=dc2*ai2+ds2*ar2;
			ar2=ar2h;

			t6=t1;
			t7=t2;
			t8=t4;
			t9=t5;
			for(ik=0;ik<idl1;ik++){
	ch2[t6++]+=ar2*c2[t8++];
	ch2[t7++]+=ai2*c2[t9++];
			}
		}
	}

	t1=0;
	for(j=1;j<ipph;j++){
		t1+=idl1;
		t2=t1;
		for(ik=0;ik<idl1;ik++)ch2[ik]+=c2[t2++];
	}

	if(ido<l1)goto L132;

	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t4=t2;
		for(i=0;i<ido;i++)cc[t4++]=ch[t3++];
		t1+=ido;
		t2+=t10;
	}

	goto L135;

 L132:
	for(i=0;i<ido;i++){
		t1=i;
		t2=i;
		for(k=0;k<l1;k++){
			cc[t2]=ch[t1];
			t1+=ido;
			t2+=t10;
		}
	}

 L135:
	t1=0;
	t2=ido<<1;
	t3=0;
	t4=ipp2*t0;
	for(j=1;j<ipph;j++){

		t1+=t2;
		t3+=t0;
		t4-=t0;

		t5=t1;
		t6=t3;
		t7=t4;

		for(k=0;k<l1;k++){
			cc[t5-1]=ch[t6];
			cc[t5]=ch[t7];
			t5+=t10;
			t6+=ido;
			t7+=ido;
		}
	}

	if(ido==1)return;
	if(nbd<l1)goto L141;

	t1=-ido;
	t3=0;
	t4=0;
	t5=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t2;
		t3+=t2;
		t4+=t0;
		t5-=t0;
		t6=t1;
		t7=t3;
		t8=t4;
		t9=t5;
		for(k=0;k<l1;k++){
			for(i=2;i<ido;i+=2){
				ic=idp2-i;
				cc[i+t7-1]=ch[i+t8-1]+ch[i+t9-1];
				cc[ic+t6-1]=ch[i+t8-1]-ch[i+t9-1];
				cc[i+t7]=ch[i+t8]+ch[i+t9];
				cc[ic+t6]=ch[i+t9]-ch[i+t8];
			}
			t6+=t10;
			t7+=t10;
			t8+=ido;
			t9+=ido;
		}
	}
	return;

 L141:

	t1=-ido;
	t3=0;
	t4=0;
	t5=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t2;
		t3+=t2;
		t4+=t0;
		t5-=t0;
		for(i=2;i<ido;i+=2){
			t6=idp2+t1-i;
			t7=i+t3;
			t8=i+t4;
			t9=i+t5;
			for(k=0;k<l1;k++){
				cc[t7-1]=ch[t8-1]+ch[t9-1];
				cc[t6-1]=ch[t8-1]-ch[t9-1];
				cc[t7]=ch[t8]+ch[t9];
				cc[t6]=ch[t9]-ch[t8];
				t6+=t10;
				t7+=t10;
				t8+=ido;
				t9+=ido;
			}
		}
	}
}

STIN void drftf1(int n,double *c,double *ch,double *wa,int *ifac)
{
	int i,k1,l1,l2;
	int na,kh,nf;
	int ip,iw,ido,idl1,ix2,ix3;

	nf=ifac[1];
	na=1;
	l2=n;
	iw=n;

	for(k1=0;k1<nf;k1++){
		kh=nf-k1;
		ip=ifac[kh+1];
		l1=l2/ip;
		ido=n/l2;
		idl1=ido*l1;
		iw-=(ip-1)*ido;
		na=1-na;

		if(ip!=4)goto L102;

		ix2=iw+ido;
		ix3=ix2+ido;
		if(na!=0)
			dradf4(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1);
		else
			dradf4(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1);
		goto L110;

 L102:
		if(ip!=2)goto L104;
		if(na!=0)goto L103;

		dradf2(ido,l1,c,ch,wa+iw-1);
		goto L110;

	L103:
		dradf2(ido,l1,ch,c,wa+iw-1);
		goto L110;

	L104:
		if(ido==1)na=1-na;
		if(na!=0)goto L109;

		dradfg(ido,ip,l1,idl1,c,c,c,ch,ch,wa+iw-1);
		na=1;
		goto L110;

	L109:
		dradfg(ido,ip,l1,idl1,ch,ch,ch,c,c,wa+iw-1);
		na=0;

	L110:
		l2=l1;
	}

	if(na==1)return;

	for(i=0;i<n;i++)c[i]=ch[i];
}

void __ogg_fdrfftf(int n,double *r,double *wsave,int *ifac)
{
	if(n==1)return;
	drftf1(n,r,wsave,wsave+n,ifac);
}

STIN void dcsqf1(int n,double *x,double *w,double *xh,int *ifac)
{
	int modn,i,k,kc;
	int np2,ns2;
	double xim1;

	ns2=(n+1)>>1;
	np2=n;

	kc=np2;
	for(k=1;k<ns2;k++){
		kc--;
		xh[k]=x[k]+x[kc];
		xh[kc]=x[k]-x[kc];
	}

	modn=n%2;
	if(modn==0)xh[ns2]=x[ns2]+x[ns2];

	for(k=1;k<ns2;k++){
		kc=np2-k;
		x[k]=w[k-1]*xh[kc]+w[kc-1]*xh[k];
		x[kc]=w[k-1]*xh[k]-w[kc-1]*xh[kc];
	}

	if(modn==0)x[ns2]=w[ns2-1]*xh[ns2];

	__ogg_fdrfftf(n,x,xh,ifac);

	for(i=2;i<n;i+=2){
		xim1=x[i-1]-x[i];
		x[i]=x[i-1]+x[i];
		x[i-1]=xim1;
	}
}

void __ogg_fdcosqf(int n,double *x,double *wsave,int *ifac)
{
		static double sqrt2=1.4142135623730950488016887242097;
		double tsqx;

	switch(n){
	case 0:case 1:
		return;
	case 2:
		tsqx=sqrt2*x[1];
		x[1]=x[0]-tsqx;
		x[0]+=tsqx;
		return;
	default:
		dcsqf1(n,x,wsave,wsave+n,ifac);
		return;
	}
}

STIN void dradb2(int ido,int l1,double *cc,double *ch,double *wa1)
{
	int i,k,t0,t1,t2,t3,t4,t5,t6;
	double ti2,tr2;

	t0=l1*ido;
	
	t1=0;
	t2=0;
	t3=(ido<<1)-1;
	for(k=0;k<l1;k++){
		ch[t1]=cc[t2]+cc[t3+t2];
		ch[t1+t0]=cc[t2]-cc[t3+t2];
		t2=(t1+=ido)<<1;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t5=(t4=t2)+(ido<<1);
		t6=t0+t1;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4+=2;
			t5-=2;
			t6+=2;
			ch[t3-1]=cc[t4-1]+cc[t5-1];
			tr2=cc[t4-1]-cc[t5-1];
			ch[t3]=cc[t4]-cc[t5];
			ti2=cc[t4]+cc[t5];
			ch[t6-1]=wa1[i-2]*tr2-wa1[i-1]*ti2;
			ch[t6]=wa1[i-2]*ti2+wa1[i-1]*tr2;
		}
		t2=(t1+=ido)<<1;
	}

	if(ido%2==1)return;

L105:
	t1=ido-1;
	t2=ido-1;
	for(k=0;k<l1;k++){
		ch[t1]=cc[t2]+cc[t2];
		ch[t1+t0]=-(cc[t2+1]+cc[t2+1]);
		t1+=ido;
		t2+=ido<<1;
	}
}

STIN void dradb3(int ido,int l1,double *cc,double *ch,double *wa1, double *wa2)
{
	static double taur = -.5;
	static double taui = .86602540378443864676372317075293618;
	int i,k,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;
	double ci2,ci3,di2,di3,cr2,cr3,dr2,dr3,ti2,tr2;
	t0=l1*ido;

	t1=0;
	t2=t0<<1;
	t3=ido<<1;
	t4=ido+(ido<<1);
	t5=0;
	for(k=0;k<l1;k++){
		tr2=cc[t3-1]+cc[t3-1];
		cr2=cc[t5]+(taur*tr2);
		ch[t1]=cc[t5]+tr2;
		ci3=taui*(cc[t3]+cc[t3]);
		ch[t1+t0]=cr2-ci3;
		ch[t1+t2]=cr2+ci3;
		t1+=ido;
		t3+=t4;
		t5+=t4;
	}

	if(ido==1)return;

	t1=0;
	t3=ido<<1;
	for(k=0;k<l1;k++){
		t7=t1+(t1<<1);
		t6=(t5=t7+t3);
		t8=t1;
		t10=(t9=t1+t0)+t0;

		for(i=2;i<ido;i+=2){
			t5+=2;
			t6-=2;
			t7+=2;
			t8+=2;
			t9+=2;
			t10+=2;
			tr2=cc[t5-1]+cc[t6-1];
			cr2=cc[t7-1]+(taur*tr2);
			ch[t8-1]=cc[t7-1]+tr2;
			ti2=cc[t5]-cc[t6];
			ci2=cc[t7]+(taur*ti2);
			ch[t8]=cc[t7]+ti2;
			cr3=taui*(cc[t5-1]-cc[t6-1]);
			ci3=taui*(cc[t5]+cc[t6]);
			dr2=cr2-ci3;
			dr3=cr2+ci3;
			di2=ci2+cr3;
			di3=ci2-cr3;
			ch[t9-1]=wa1[i-2]*dr2-wa1[i-1]*di2;
			ch[t9]=wa1[i-2]*di2+wa1[i-1]*dr2;
			ch[t10-1]=wa2[i-2]*dr3-wa2[i-1]*di3;
			ch[t10]=wa2[i-2]*di3+wa2[i-1]*dr3;
		}
		t1+=ido;
	}
}

STIN void dradb4(int ido,int l1,double *cc,double *ch,double *wa1, double *wa2,double *wa3)
{
	static double sqrt2=1.4142135623730950488016887242097;
	int i,k,t0,t1,t2,t3,t4,t5,t6,t7,t8;
	double ci2,ci3,ci4,cr2,cr3,cr4,ti1,ti2,ti3,ti4,tr1,tr2,tr3,tr4;
	t0=l1*ido;
	
	t1=0;
	t2=ido<<2;
	t3=0;
	t6=ido<<1;
	for(k=0;k<l1;k++){
		t4=t3+t6;
		t5=t1;
		tr3=cc[t4-1]+cc[t4-1];
		tr4=cc[t4]+cc[t4]; 
		tr1=cc[t3]-cc[(t4+=t6)-1];
		tr2=cc[t3]+cc[t4-1];
		ch[t5]=tr2+tr3;
		ch[t5+=t0]=tr1-tr4;
		ch[t5+=t0]=tr2-tr3;
		ch[t5+=t0]=tr1+tr4;
		t1+=ido;
		t3+=t2;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	for(k=0;k<l1;k++){
		t5=(t4=(t3=(t2=t1<<2)+t6))+t6;
		t7=t1;
		for(i=2;i<ido;i+=2){
			t2+=2;
			t3+=2;
			t4-=2;
			t5-=2;
			t7+=2;
			ti1=cc[t2]+cc[t5];
			ti2=cc[t2]-cc[t5];
			ti3=cc[t3]-cc[t4];
			tr4=cc[t3]+cc[t4];
			tr1=cc[t2-1]-cc[t5-1];
			tr2=cc[t2-1]+cc[t5-1];
			ti4=cc[t3-1]-cc[t4-1];
			tr3=cc[t3-1]+cc[t4-1];
			ch[t7-1]=tr2+tr3;
			cr3=tr2-tr3;
			ch[t7]=ti2+ti3;
			ci3=ti2-ti3;
			cr2=tr1-tr4;
			cr4=tr1+tr4;
			ci2=ti1+ti4;
			ci4=ti1-ti4;

			ch[(t8=t7+t0)-1]=wa1[i-2]*cr2-wa1[i-1]*ci2;
			ch[t8]=wa1[i-2]*ci2+wa1[i-1]*cr2;
			ch[(t8+=t0)-1]=wa2[i-2]*cr3-wa2[i-1]*ci3;
			ch[t8]=wa2[i-2]*ci3+wa2[i-1]*cr3;
			ch[(t8+=t0)-1]=wa3[i-2]*cr4-wa3[i-1]*ci4;
			ch[t8]=wa3[i-2]*ci4+wa3[i-1]*cr4;
		}
		t1+=ido;
	}

	if(ido%2 == 1)return;

 L105:

	t1=ido;
	t2=ido<<2;
	t3=ido-1;
	t4=ido+(ido<<1);
	for(k=0;k<l1;k++){
		t5=t3;
		ti1=cc[t1]+cc[t4];
		ti2=cc[t4]-cc[t1];
		tr1=cc[t1-1]-cc[t4-1];
		tr2=cc[t1-1]+cc[t4-1];
		ch[t5]=tr2+tr2;
		ch[t5+=t0]=sqrt2*(tr1-ti1);
		ch[t5+=t0]=ti2+ti2;
		ch[t5+=t0]=-sqrt2*(tr1+ti1);

		t3+=ido;
		t1+=t2;
		t4+=t2;
	}
}

STIN void dradbg(int ido,int ip,int l1,int idl1,double *cc,double *c1,
								double *c2,double *ch,double *ch2,double *wa)
{
	static double tpi=6.28318530717958647692528676655900577;
	int idij,ipph,i,j,k,l,ik,is,t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,
			t11,t12;
	double dc2,ai1,ai2,ar1,ar2,ds2;
	int nbd;
	double dcp,arg,dsp,ar1h,ar2h;
	int ipp2;

	t10=ip*ido;
	t0=l1*ido;
	arg=tpi/(double)ip;
	dcp=cos(arg);
	dsp=sin(arg);
	nbd=(ido-1)>>1;
	ipp2=ip;
	ipph=(ip+1)>>1;
	if(ido<l1)goto L103;
	
	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t4=t2;
		for(i=0;i<ido;i++){
			ch[t3]=cc[t4];
			t3++;
			t4++;
		}
		t1+=ido;
		t2+=t10;
	}
	goto L106;

 L103:
	t1=0;
	for(i=0;i<ido;i++){
		t2=t1;
		t3=t1;
		for(k=0;k<l1;k++){
			ch[t2]=cc[t3];
			t2+=ido;
			t3+=t10;
		}
		t1++;
	}

 L106:
	t1=0;
	t2=ipp2*t0;
	t7=(t5=ido<<1);
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		t6=t5;
		for(k=0;k<l1;k++){
			ch[t3]=cc[t6-1]+cc[t6-1];
			ch[t4]=cc[t6]+cc[t6];
			t3+=ido;
			t4+=ido;
			t6+=t10;
		}
		t5+=t7;
	}

	if (ido == 1)goto L116;
	if(nbd<l1)goto L112;

	t1=0;
	t2=ipp2*t0;
	t7=0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;

		t7+=(ido<<1);
		t8=t7;
		for(k=0;k<l1;k++){
			t5=t3;
			t6=t4;
			t9=t8;
			t11=t8;
			for(i=2;i<ido;i+=2){
				t5+=2;
				t6+=2;
				t9+=2;
				t11-=2;
				ch[t5-1]=cc[t9-1]+cc[t11-1];
				ch[t6-1]=cc[t9-1]-cc[t11-1];
				ch[t5]=cc[t9]-cc[t11];
				ch[t6]=cc[t9]+cc[t11];
			}
			t3+=ido;
			t4+=ido;
			t8+=t10;
		}
	}
	goto L116;

 L112:
	t1=0;
	t2=ipp2*t0;
	t7=0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		t7+=(ido<<1);
		t8=t7;
		t9=t7;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4+=2;
			t8+=2;
			t9-=2;
			t5=t3;
			t6=t4;
			t11=t8;
			t12=t9;
			for(k=0;k<l1;k++){
				ch[t5-1]=cc[t11-1]+cc[t12-1];
				ch[t6-1]=cc[t11-1]-cc[t12-1];
				ch[t5]=cc[t11]-cc[t12];
				ch[t6]=cc[t11]+cc[t12];
				t5+=ido;
				t6+=ido;
				t11+=t10;
				t12+=t10;
			}
		}
	}

L116:
	ar1=1.;
	ai1=0.;
	t1=0;
	t9=(t2=ipp2*idl1);
	t3=(ip-1)*idl1;
	for(l=1;l<ipph;l++){
		t1+=idl1;
		t2-=idl1;

		ar1h=dcp*ar1-dsp*ai1;
		ai1=dcp*ai1+dsp*ar1;
		ar1=ar1h;
		t4=t1;
		t5=t2;
		t6=0;
		t7=idl1;
		t8=t3;
		for(ik=0;ik<idl1;ik++){
			c2[t4++]=ch2[t6++]+ar1*ch2[t7++];
			c2[t5++]=ai1*ch2[t8++];
		}
		dc2=ar1;
		ds2=ai1;
		ar2=ar1;
		ai2=ai1;

		t6=idl1;
		t7=t9-idl1;
		for(j=2;j<ipph;j++){
			t6+=idl1;
			t7-=idl1;
			ar2h=dc2*ar2-ds2*ai2;
			ai2=dc2*ai2+ds2*ar2;
			ar2=ar2h;
			t4=t1;
			t5=t2;
			t11=t6;
			t12=t7;
			for(ik=0;ik<idl1;ik++){
				c2[t4++]+=ar2*ch2[t11++];
				c2[t5++]+=ai2*ch2[t12++];
			}
		}
	}

	t1=0;
	for(j=1;j<ipph;j++){
		t1+=idl1;
		t2=t1;
		for(ik=0;ik<idl1;ik++)ch2[ik]+=ch2[t2++];
	}

	t1=0;
	t2=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		for(k=0;k<l1;k++){
			ch[t3]=c1[t3]-c1[t4];
			ch[t4]=c1[t3]+c1[t4];
			t3+=ido;
			t4+=ido;
		}
	}

	if(ido==1)goto L132;
	if(nbd<l1)goto L128;

	t1=0;
	t2=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		for(k=0;k<l1;k++){
			t5=t3;
			t6=t4;
			for(i=2;i<ido;i+=2){
				t5+=2;
				t6+=2;
				ch[t5-1]=c1[t5-1]-c1[t6];
				ch[t6-1]=c1[t5-1]+c1[t6];
				ch[t5]=c1[t5]+c1[t6-1];
				ch[t6]=c1[t5]-c1[t6-1];
			}
			t3+=ido;
			t4+=ido;
		}
	}
	goto L132;

 L128:
	t1=0;
	t2=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1;
		t4=t2;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4+=2;
			t5=t3;
			t6=t4;
			for(k=0;k<l1;k++){
				ch[t5-1]=c1[t5-1]-c1[t6];
				ch[t6-1]=c1[t5-1]+c1[t6];
				ch[t5]=c1[t5]+c1[t6-1];
				ch[t6]=c1[t5]-c1[t6-1];
				t5+=ido;
				t6+=ido;
			}
		}
	}

L132:
	if(ido==1)return;

	for(ik=0;ik<idl1;ik++)c2[ik]=ch2[ik];

	t1=0;
	for(j=1;j<ip;j++){
		t2=(t1+=t0);
		for(k=0;k<l1;k++){
			c1[t2]=ch[t2];
			t2+=ido;
		}
	}

	if(nbd>l1)goto L139;

	is= -ido-1;
	t1=0;
	for(j=1;j<ip;j++){
		is+=ido;
		t1+=t0;
		idij=is;
		t2=t1;
		for(i=2;i<ido;i+=2){
			t2+=2;
			idij+=2;
			t3=t2;
			for(k=0;k<l1;k++){
	c1[t3-1]=wa[idij-1]*ch[t3-1]-wa[idij]*ch[t3];
	c1[t3]=wa[idij-1]*ch[t3]+wa[idij]*ch[t3-1];
	t3+=ido;
			}
		}
	}
	return;

 L139:
	is= -ido-1;
	t1=0;
	for(j=1;j<ip;j++){
		is+=ido;
		t1+=t0;
		t2=t1;
		for(k=0;k<l1;k++){
			idij=is;
			t3=t2;
			for(i=2;i<ido;i+=2){
				idij+=2;
				t3+=2;
				c1[t3-1]=wa[idij-1]*ch[t3-1]-wa[idij]*ch[t3];
				c1[t3]=wa[idij-1]*ch[t3]+wa[idij]*ch[t3-1];
			}
			t2+=ido;
		}
	}
}

STIN void drftb1(int n, double *c, double *ch, double *wa, int *ifac)
{
	int i,k1,l1,l2;
	int na;
	int nf,ip,iw,ix2,ix3,ido,idl1;

	nf=ifac[1];
	na=0;
	l1=1;
	iw=1;

	for(k1=0;k1<nf;k1++){
		ip=ifac[k1 + 2];
		l2=ip*l1;
		ido=n/l2;
		idl1=ido*l1;
		if(ip!=4)goto L103;
		ix2=iw+ido;
		ix3=ix2+ido;

		if(na!=0)
			dradb4(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1);
		else
			dradb4(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1);
		na=1-na;
		goto L115;

	L103:
		if(ip!=2)goto L106;

		if(na!=0)
			dradb2(ido,l1,ch,c,wa+iw-1);
		else
			dradb2(ido,l1,c,ch,wa+iw-1);
		na=1-na;
		goto L115;

	L106:
		if(ip!=3)goto L109;

		ix2=iw+ido;
		if(na!=0)
			dradb3(ido,l1,ch,c,wa+iw-1,wa+ix2-1);
		else
			dradb3(ido,l1,c,ch,wa+iw-1,wa+ix2-1);
		na=1-na;
		goto L115;

	L109:
/*		The radix five case can be translated later..... */
/*		if(ip!=5)goto L112;

		ix2=iw+ido;
		ix3=ix2+ido;
		ix4=ix3+ido;
		if(na!=0)
			dradb5(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1,wa+ix4-1);
		else
			dradb5(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1,wa+ix4-1);
		na=1-na;
		goto L115;

	L112:*/
		if(na!=0)
			dradbg(ido,ip,l1,idl1,ch,ch,ch,c,c,wa+iw-1);
		else
			dradbg(ido,ip,l1,idl1,c,c,c,ch,ch,wa+iw-1);
		if(ido==1)na=1-na;

	L115:
		l1=l2;
		iw+=(ip-1)*ido;
	}

	if(na==0)return;

	for(i=0;i<n;i++)c[i]=ch[i];
}

void __ogg_fdrfftb(int n, double *r, double *wsave, int *ifac)
{
	if (n == 1)return;
	drftb1(n, r, wsave, wsave+n, ifac);
}

STIN void dcsqb1(int n,double *x,double *w,double *xh,int *ifac)
{
	int modn,i,k,kc;
	int np2,ns2;
	double xim1;

	ns2=(n+1)>>1;
	np2=n;

	for(i=2;i<n;i+=2){
		xim1=x[i-1]+x[i];
		x[i]-=x[i-1];
		x[i-1]=xim1;
	}

	x[0]+=x[0];
	modn=n%2;
	if(modn==0)x[n-1]+=x[n-1];

	__ogg_fdrfftb(n,x,xh,ifac);

	kc=np2;
	for(k=1;k<ns2;k++){
		kc--;
		xh[k]=w[k-1]*x[kc]+w[kc-1]*x[k];
		xh[kc]=w[k-1]*x[k]-w[kc-1]*x[kc];
	}

	if(modn==0)x[ns2]=w[ns2-1]*(x[ns2]+x[ns2]);

	kc=np2;
	for(k=1;k<ns2;k++){
		kc--;
		x[k]=xh[k]+xh[kc];
		x[kc]=xh[k]-xh[kc];
	}
	x[0]+=x[0];
}

void __ogg_fdcosqb(int n,double *x,double *wsave,int *ifac)
{
	static double tsqrt2 = 2.8284271247461900976033774484194;
	double x1;

	if(n<2){
		x[0]*=4;
		return;
	}
	if(n==2){
		x1=(x[0]+x[1])*4;
		x[1]=tsqrt2*(x[0]-x[1]);
		x[0]=x1;
		return;
	}
	
	dcsqb1(n,x,wsave,wsave+n,ifac);
}


double DiscreteFourierTransform(char *epsilon, int n)
{
	double	p_value, upperBound, N_l, N_o, d, *m, *X, *wsave, *ifac;
	int		i, count;
	
	if ( ((X = (double*) calloc(n,sizeof(double))) == NULL) ||
		 ((wsave = (double *)calloc(2*n+15,sizeof(double))) == NULL) ||
		 ((ifac = (double *)calloc(15,sizeof(double))) == NULL) ||
		 ((m = (double*)calloc(n/2+1, sizeof(double))) == NULL) ) {
#if 0
			sprintf(msg,"Unable to allocate working arrays for the DFT.\n");
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
			if( X == NULL )
				free(X);
			if( wsave == NULL )
				free(wsave);
			if( ifac == NULL )
				free(ifac);
			if( m == NULL )
				free(m);
			return(0);
	}
	for ( i=0; i<n; i++ )
		X[i] = 2*(int)epsilon[i] - 1;
	
	__ogg_fdrffti(n, wsave, (int *)ifac);		/* INITIALIZE WORK ARRAYS */
	__ogg_fdrfftf(n, X, wsave, (int *)ifac);	/* APPLY FORWARD FFT */
	
	m[0] = sqrt(X[0]*X[0]);	    /* COMPUTE MAGNITUDE */
	
	for ( i=0; i<n/2; i++ ) {	   	    /* DISPLAY FOURIER POINTS */
		m[i+1] = sqrt(pow(X[2*i+1],2)+pow(X[2*i+2],2)); 
	}
	count = 0;				       /* CONFIDENCE INTERVAL */
	upperBound = sqrt(2.995732274*n);
	for ( i=0; i<n/2; i++ )
		if ( m[i] < upperBound )
			count++;
	N_l = (double) count;       /* number of peaks less than h = sqrt(3*n) */
	N_o = (double) 0.95*n/2.0;
	d = (N_l - N_o)/sqrt(n/2.0*0.95*0.05);
	p_value = erfc(fabs(d)/sqrt(2.0));

	free(X);
	free(wsave);
	free(ifac);
	free(m);

	return( p_value < ALPHA ? (p_value) : 0 );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                          F R E Q U E N C Y  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Frequency(char *epsilon, int n)
{
	int		i;
	double	f, s_obs, p_value, sum, sqrt2 = 1.41421356237309504880;
	
	sum = 0.0;
	for ( i=0; i<n; i++ )
		sum += 2*(int)epsilon[i]-1;
	s_obs = fabs(sum)/sqrt(n);
	f = s_obs/sqrt2;
	p_value = erfc(f);

	return( p_value < ALPHA ? (p_value) : 0 );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                      L I N E A R  C O M P L E X I T Y   T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double LinearComplexity(char *epsilon, int n, int M)
{
	int       i, ii, j, d, N, L, m, N_, parity, sign, K = 6;
	double    p_value, T_, mean, nu[7], chi2;
	double    pi[7] = { 0.01047, 0.03125, 0.12500, 0.50000, 0.25000, 0.06250, 0.020833 };
	BitSequence  *T, *P, *B_, *C;
	
	N = (int)floor(n/M);
	if ( ((B_ = (BitSequence *) calloc(M, sizeof(BitSequence))) == NULL) ||
		 ((C  = (BitSequence *) calloc(M, sizeof(BitSequence))) == NULL) ||
		 ((P  = (BitSequence *) calloc(M, sizeof(BitSequence))) == NULL) ||
		 ((T  = (BitSequence *) calloc(M, sizeof(BitSequence))) == NULL) ) {
#if 0
		sprintf("Linear Complexity:Insufficient Memory for Work Space\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		if ( B_!= NULL )
			free(B_);
		if ( C != NULL )
			free(C);
		if ( P != NULL )
			free(P);
		if ( T != NULL )
			free(T);
		return(0);
	}

	for ( i=0; i<K+1; i++ )
		nu[i] = 0.00;
	for ( ii=0; ii<N; ii++ ) {
		for ( i=0; i<M; i++ ) {
			B_[i] = 0;
			C[i] = 0;
			T[i] = 0;
			P[i] = 0;
		}
		L = 0;
		m = -1;
		d = 0;
		C[0] = 1;
		B_[0] = 1;
		
		/* DETERMINE LINEAR COMPLEXITY */
		N_ = 0;
		while ( N_ < M ) {
			d = (int)epsilon[ii*M+N_];
			for ( i=1; i<=L; i++ )
				d += C[i] * epsilon[ii*M+N_-i];
			d = d%2;
			if ( d == 1 ) {
				for ( i=0; i<M; i++ ) {
					T[i] = C[i];
					P[i] = 0;
				}
				for ( j=0; j<M; j++ )
					if ( B_[j] == 1 )
						P[j+N_-m] = 1;
				for ( i=0; i<M; i++ )
					C[i] = (C[i] + P[i])%2;
				if ( L <= N_/2 ) {
					L = N_ + 1 - L;
					m = N_;
					for ( i=0; i<M; i++ )
						B_[i] = T[i];
				}
			}
			N_++;
		}
		if ( (parity = (M+1)%2) == 0 ) 
			sign = -1;
		else 
			sign = 1;
		mean = M/2.0 + (9.0+sign)/36.0 - 1.0/pow(2, M) * (M/3.0 + 2.0/9.0);
		if ( (parity = M%2) == 0 )
			sign = 1;
		else 
			sign = -1;
		T_ = sign * (L - mean) + 2.0/9.0;
		
		if ( T_ <= -2.5 )
			nu[0]++;
		else if ( T_ > -2.5 && T_ <= -1.5 )
			nu[1]++;
		else if ( T_ > -1.5 && T_ <= -0.5 )
			nu[2]++;
		else if ( T_ > -0.5 && T_ <= 0.5 )
			nu[3]++;
		else if ( T_ > 0.5 && T_ <= 1.5 )
			nu[4]++;
		else if ( T_ > 1.5 && T_ <= 2.5 )
			nu[5]++;
		else
			nu[6]++;
	}
	chi2 = 0.00;
	for ( i=0; i<K+1; i++ )
		chi2 += pow(nu[i]-N*pi[i], 2) / (N*pi[i]);
	p_value = cephes_igamc(K/2.0, chi2/2.0);

	free(B_);
	free(P);
	free(C);
	free(T);
	
	return( p_value < ALPHA ? (p_value) : 0 );
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                      L O N G E S T  R U N S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double LongestRunOfOnes(char *epsilon, int n)
{
	double			pval, chi2, pi[7];
	int				run, v_n_obs, N, i, j, K, M, V[7];
	unsigned int	nu[7] = { 0, 0, 0, 0, 0, 0, 0 };

	if ( n < 6272 ) {
		K = 3;
		M = 8;
		V[0] = 1; V[1] = 2; V[2] = 3; V[3] = 4;
		pi[0] = 0.21484375;
		pi[1] = 0.3671875;
		pi[2] = 0.23046875;
		pi[3] = 0.1875;
	}
	else if ( n < 750000 ) {
		K = 5;
		M = 128;
		V[0] = 4; V[1] = 5; V[2] = 6; V[3] = 7; V[4] = 8; V[5] = 9;
		pi[0] = 0.1174035788;
		pi[1] = 0.242955959;
		pi[2] = 0.249363483;
		pi[3] = 0.17517706;
		pi[4] = 0.102701071;
		pi[5] = 0.112398847;
	}
	else {
		K = 6;
		M = 10000;
		V[0] = 10; V[1] = 11; V[2] = 12; V[3] = 13; V[4] = 14; V[5] = 15; V[6] = 16;
		pi[0] = 0.0882;
		pi[1] = 0.2092;
		pi[2] = 0.2483;
		pi[3] = 0.1933;
		pi[4] = 0.1208;
		pi[5] = 0.0675;
		pi[6] = 0.0727;
	}
	
	N = n/M;
	for ( i=0; i<N; i++ ) {
		v_n_obs = 0;
		run = 0;
		for ( j=0; j<M; j++ ) {
			if ( epsilon[i*M+j] == 1 ) {
				run++;
				if ( run > v_n_obs )
					v_n_obs = run;
			}
			else
				run = 0;
		}
		if ( v_n_obs < V[0] )
			nu[0]++;
		for ( j=0; j<=K; j++ ) {
			if ( v_n_obs == V[j] )
				nu[j]++;
		}
		if ( v_n_obs > V[K] )
			nu[K]++;
	}

	chi2 = 0.0;
	for ( i=0; i<=K; i++ )
		chi2 += ((nu[i] - N * pi[i]) * (nu[i] - N * pi[i])) / (N * pi[i]);

	pval = cephes_igamc((double)(K/2.0), chi2 / 2.0);

	if ( isNegative(pval) || isGreaterThanOne(pval) ) {
#if 0
		sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
	}

	return( pval < ALPHA ? (pval) : 0 );
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
R A N K  A L G O R I T H M  R O U T I N E S
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define	MATRIX_FORWARD_ELIMINATION	0
#define	MATRIX_BACKWARD_ELIMINATION	1

int computeRank(int M, int Q, BitSequence **matrix)
{
	int		i, rank, m=MIN(M,Q);
	
	/* FORWARD APPLICATION OF ELEMENTARY ROW OPERATIONS */ 
	for ( i=0; i<m-1; i++ ) {
		if ( matrix[i][i] == 1 ) 
			perform_elementary_row_operations(MATRIX_FORWARD_ELIMINATION, i, M, Q, matrix);
		else { 	/* matrix[i][i] = 0 */
			if ( find_unit_element_and_swap(MATRIX_FORWARD_ELIMINATION, i, M, Q, matrix) == 1 ) 
				perform_elementary_row_operations(MATRIX_FORWARD_ELIMINATION, i, M, Q, matrix);
		}
	}

	/* BACKWARD APPLICATION OF ELEMENTARY ROW OPERATIONS */ 
	for ( i=m-1; i>0; i-- ) {
		if ( matrix[i][i] == 1 )
			perform_elementary_row_operations(MATRIX_BACKWARD_ELIMINATION, i, M, Q, matrix);
		else { 	/* matrix[i][i] = 0 */
			if ( find_unit_element_and_swap(MATRIX_BACKWARD_ELIMINATION, i, M, Q, matrix) == 1 )
				perform_elementary_row_operations(MATRIX_BACKWARD_ELIMINATION, i, M, Q, matrix);
		}
	} 

	rank = determine_rank(m, M, Q, matrix);

	return rank;
}

void perform_elementary_row_operations(int flag, int i, int M, int Q, BitSequence **A)
{
	int		j, k;
	
	if ( flag == MATRIX_FORWARD_ELIMINATION ) {
		for ( j=i+1; j<M;  j++ )
			if ( A[j][i] == 1 ) 
				for ( k=i; k<Q; k++ ) 
					A[j][k] = (A[j][k] + A[i][k]) % 2;
	}
	else {
		for ( j=i-1; j>=0;  j-- )
			if ( A[j][i] == 1 )
				for ( k=0; k<Q; k++ )
					A[j][k] = (A[j][k] + A[i][k]) % 2;
	}
}

int find_unit_element_and_swap(int flag, int i, int M, int Q, BitSequence **A)
{ 
	int		index, row_op=0;
	
	if ( flag == MATRIX_FORWARD_ELIMINATION ) {
		index = i+1;
		while ( (index < M) && (A[index][i] == 0) ) 
			index++;
			if ( index < M )
				row_op = swap_rows(i, index, Q, A);
	}
	else {
		index = i-1;
		while ( (index >= 0) && (A[index][i] == 0) ) 
			index--;
			if ( index >= 0 )
				row_op = swap_rows(i, index, Q, A);
	}
	
	return row_op;
}

int swap_rows(int i, int index, int Q, BitSequence **A)
{
	int			p;
	BitSequence	temp;
	
	for ( p=0; p<Q; p++ ) {
		temp = A[i][p];
		A[i][p] = A[index][p];
		A[index][p] = temp;
	}
	
	return 1;
}

int determine_rank(int m, int M, int Q, BitSequence **A)
{
	int		i, j, rank, allZeroes;
	
	/* DETERMINE RANK, THAT IS, COUNT THE NUMBER OF NONZERO ROWS */
	
	rank = m;
	for ( i=0; i<M; i++ ) {
		allZeroes = 1; 
		for ( j=0; j<Q; j++)  {
			if ( A[i][j] == 1 ) {
				allZeroes = 0;
				break;
			}
		}
		if ( allZeroes == 1 )
			rank--;
	} 
	
	return rank;
}

BitSequence** create_matrix(int M, int Q)
{
	int			i;
	BitSequence	**matrix;
	
	if ( (matrix = (BitSequence **) calloc(M, sizeof(BitSequence *))) == NULL ) {
		printf("ERROR IN FUNCTION create_matrix:  Insufficient memory available.\n");
		
		return NULL;
	}
	else {
		for ( i=0; i<M; i++ ) {
			if ( (matrix[i] = calloc(Q, sizeof(BitSequence))) == NULL ) {
				printf("ERROR IN FUNCTION create_matrix: Insufficient memory for %dx%d matrix.\n", M, M);

				return NULL;
			}
		}
		return matrix;
	}
}

void def_matrix(char *epsilon, int M, int Q, BitSequence **m,int k)
{
	int		i,j;
	
	for ( i=0; i<M; i++ ) 
		for ( j=0; j<Q; j++ )
			m[i][j] = epsilon[k*(M*Q)+j+i*M];
}

void delete_matrix(int M, BitSequence **matrix)
{
	int		i;

	for ( i=0; i<M; i++ )
		free(matrix[i]);
	free(matrix);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
          N O N O V E R L A P P I N G  T E M P L A T E  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double NonOverlappingTemplateMatchings(char * epsilon, int n, int m)
{
#if 0
	int		numOfTemplates[100] = {0, 0, 2, 4, 6, 12, 20, 40, 74, 148, 284, 568, 1116,
						2232, 4424, 8848, 17622, 35244, 70340, 140680, 281076, 562152};
	/*----------------------------------------------------------------------------
	NOTE:  Should additional templates lengths beyond 21 be desired, they must 
	first be constructed, saved into files and then the corresponding 
	number of nonperiodic templates for that file be stored in the m-th 
	position in the numOfTemplates variable.
	----------------------------------------------------------------------------*/
	unsigned int	bit, W_obs, nu[6], *Wj = NULL; 
	FILE			*fp;
	double			sum, chi2, p_value, lambda, pi[6], varWj;
	int				i, j, jj, k, match, SKIP, M, N, K = 5;
	char			directory[100];
	BitSequence		*sequence = NULL;

	N = 8;
	M = n/N;

	if ( (Wj = (unsigned int*)calloc(N, sizeof(unsigned int))) == NULL ) {
#if 0
		sprintf(msg, "\tInsufficient memory for required work space in non-overlap tm.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		return(0);
	}
	lambda = (M-m+1)/pow(2, m);
	varWj = M*(1.0/pow(2.0, m) - (2.0*m-1.0)/pow(2.0, 2.0*m));
	sprintf(directory, "templates/template%d", m);

	if ( ((isNegative(lambda)) || (isZero(lambda))) ||
		 ((fp = fopen(directory, "r")) == NULL) ||
		 ((sequence = (BitSequence *) calloc(m, sizeof(BitSequence))) == NULL) ) {
		fprintf(stats[TEST_NONPERIODIC], "\tNONOVERLAPPING TEMPLATES TESTS ABORTED DUE TO ONE OF THE FOLLOWING : \n");
		fprintf(stats[TEST_NONPERIODIC], "\tLambda (%f) not being positive!\n", lambda);
		fprintf(stats[TEST_NONPERIODIC], "\tTemplate file <%s> not existing\n", directory);
		fprintf(stats[TEST_NONPERIODIC], "\tInsufficient memory for required work space.\n");
		if ( sequence != NULL )
			free(sequence);
	}
	else {
		fprintf(stats[TEST_NONPERIODIC], "\t\t  NONPERIODIC TEMPLATES TEST\n");
		fprintf(stats[TEST_NONPERIODIC], "-------------------------------------------------------------------------------------\n");
		fprintf(stats[TEST_NONPERIODIC], "\t\t  COMPUTATIONAL INFORMATION\n");
		fprintf(stats[TEST_NONPERIODIC], "-------------------------------------------------------------------------------------\n");
		fprintf(stats[TEST_NONPERIODIC], "\tLAMBDA = %f\tM = %d\tN = %d\tm = %d\tn = %d\n", lambda, M, N, m, n);
		fprintf(stats[TEST_NONPERIODIC], "-------------------------------------------------------------------------------------\n");
		fprintf(stats[TEST_NONPERIODIC], "\t\tF R E Q U E N C Y\n");
		fprintf(stats[TEST_NONPERIODIC], "Template   W_1  W_2  W_3  W_4  W_5  W_6  W_7  W_8    Chi^2   P_value Assignment Index\n");
		fprintf(stats[TEST_NONPERIODIC], "-------------------------------------------------------------------------------------\n");

		if ( numOfTemplates[m] < MAXNUMOFTEMPLATES )
			SKIP = 1;
		else
			SKIP = (int)(numOfTemplates[m]/MAXNUMOFTEMPLATES);
		numOfTemplates[m] = (int)numOfTemplates[m]/SKIP;
		
		sum = 0.0;
		for ( i=0; i<2; i++ ) {                      /* Compute Probabilities */
			pi[i] = exp(-lambda+i*log(lambda)-cephes_lgam(i+1));
			sum += pi[i];
		}
		pi[0] = sum;
		for ( i=2; i<=K; i++ ) {                      /* Compute Probabilities */
			pi[i-1] = exp(-lambda+i*log(lambda)-cephes_lgam(i+1));
			sum += pi[i-1];
		}
		pi[K] = 1 - sum;

		for( jj=0; jj<MIN(MAXNUMOFTEMPLATES, numOfTemplates[m]); jj++ ) {
			sum = 0;

			for ( k=0; k<m; k++ ) {
				fscanf(fp, "%d", &bit);
				sequence[k] = bit;
				fprintf(stats[TEST_NONPERIODIC], "%d", sequence[k]);
			}
			fprintf(stats[TEST_NONPERIODIC], " ");
			for ( k=0; k<=K; k++ )
				nu[k] = 0;
			for ( i=0; i<N; i++ ) {
				W_obs = 0;
				for ( j=0; j<M-m+1; j++ ) {
					match = 1;
					for ( k=0; k<m; k++ ) {
						if ( (int)sequence[k] != (int)epsilon[i*M+j+k] ) {
							match = 0;
							break;
						}
					}
					if ( match == 1 )
						W_obs++;
				}
				Wj[i] = W_obs;
			}
			sum = 0;
			chi2 = 0.0;                                   /* Compute Chi Square */
			for ( i=0; i<N; i++ ) {
				if ( m == 10 )
					fprintf(stats[TEST_NONPERIODIC], "%3d  ", Wj[i]);
				else
					fprintf(stats[TEST_NONPERIODIC], "%4d ", Wj[i]);
				chi2 += pow(((double)Wj[i] - lambda)/pow(varWj, 0.5), 2);
			}
			p_value = cephes_igamc(N/2.0, chi2/2.0);
		
			if ( isNegative(p_value) || isGreaterThanOne(p_value) )
				fprintf(stats[TEST_NONPERIODIC], "\t\tWARNING:  P_VALUE IS OUT OF RANGE.\n");

			fprintf(stats[TEST_NONPERIODIC], "%9.6f %f %s %3d\n", chi2, p_value, p_value < ALPHA ? "FAILURE" : "SUCCESS", jj);
			if ( SKIP > 1 )
				fseek(fp, (long)(SKIP-1)*2*m, SEEK_CUR);
			fprintf(results[TEST_NONPERIODIC], "%f\n", p_value);
		}
	}
	
	fprintf(stats[TEST_NONPERIODIC], "\n");
	if ( sequence != NULL )
		free(sequence);

	free(Wj);
	fclose(fp);
#endif
	return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
               O V E R L A P P I N G  T E M P L A T E  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double OverlappingTemplateMatchings(char *epsilon, int n, int m)
{
#if 0
	int				i, k, match;
	double			W_obs, eta, sum, chi2, p_value, lambda;
	int				M, N, j, K = 5;
	unsigned int	nu[6] = { 0, 0, 0, 0, 0, 0 };
	double			pi[6] = { 0.143783, 0.139430, 0.137319, 0.124314, 0.106209, 0.348945 };
	BitSequence		*sequence;

	M = 1032;
	N = n/M;
	
	if ( (sequence = (BitSequence *) calloc(m, sizeof(BitSequence))) == NULL ) {
		fprintf(stats[TEST_OVERLAPPING], "\t\t    OVERLAPPING TEMPLATE OF ALL ONES TEST\n");
		fprintf(stats[TEST_OVERLAPPING], "\t\t---------------------------------------------\n");
		fprintf(stats[TEST_OVERLAPPING], "\t\tTEMPLATE DEFINITION:  Insufficient memory, Overlapping Template Matchings test aborted!\n");
	}
	else
		for ( i=0; i<m; i++ )
			sequence[i] = 1;
	
	lambda = (double)(M-m+1)/pow(2,m);
	eta = lambda/2.0;
	sum = 0.0;
	for ( i=0; i<K; i++ ) {			/* Compute Probabilities */
		pi[i] = Pr(i, eta);
		sum += pi[i];
	}
	pi[K] = 1 - sum;

	for ( i=0; i<N; i++ ) {
		W_obs = 0;
		for ( j=0; j<M-m+1; j++ ) {
			match = 1;
			for ( k=0; k<m; k++ ) {
				if ( sequence[k] != epsilon[i*M+j+k] )
					match = 0;
			}
			if ( match == 1 )
				W_obs++;
		}
		if ( W_obs <= 4 )
			nu[(int)W_obs]++;
		else
			nu[K]++;
	}
	sum = 0;
	chi2 = 0.0;                                   /* Compute Chi Square */
	for ( i=0; i<K+1; i++ ) {
		chi2 += pow((double)nu[i] - (double)N*pi[i], 2)/((double)N*pi[i]);
		sum += nu[i];
	}
	p_value = cephes_igamc(K/2.0, chi2/2.0);

	fprintf(stats[TEST_OVERLAPPING], "\t\t    OVERLAPPING TEMPLATE OF ALL ONES TEST\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t-----------------------------------------------\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\tCOMPUTATIONAL INFORMATION:\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t-----------------------------------------------\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t(a) n (sequence_length)      = %d\n", n);
	fprintf(stats[TEST_OVERLAPPING], "\t\t(b) m (block length of 1s)   = %d\n", m);
	fprintf(stats[TEST_OVERLAPPING], "\t\t(c) M (length of substring)  = %d\n", M);
	fprintf(stats[TEST_OVERLAPPING], "\t\t(d) N (number of substrings) = %d\n", N);
	fprintf(stats[TEST_OVERLAPPING], "\t\t(e) lambda [(M-m+1)/2^m]     = %f\n", lambda);
	fprintf(stats[TEST_OVERLAPPING], "\t\t(f) eta                      = %f\n", eta);
	fprintf(stats[TEST_OVERLAPPING], "\t\t-----------------------------------------------\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t   F R E Q U E N C Y\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t  0   1   2   3   4 >=5   Chi^2   P-value  Assignment\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t-----------------------------------------------\n");
	fprintf(stats[TEST_OVERLAPPING], "\t\t%3d %3d %3d %3d %3d %3d  %f ",
		nu[0], nu[1], nu[2], nu[3], nu[4], nu[5], chi2);

	if ( isNegative(p_value) || isGreaterThanOne(p_value) )
		fprintf(stats[TEST_OVERLAPPING], "WARNING:  P_VALUE IS OUT OF RANGE.\n");

	free(sequence);
	fprintf(stats[TEST_OVERLAPPING], "%f %s\n\n", p_value, p_value < ALPHA ? "FAILURE" : "SUCCESS");
	fprintf(results[TEST_OVERLAPPING], "%f\n", p_value);
#endif
	return(0);
}

double Pr(int u, double eta)
{
	int		l;
	double	sum, p;
	
	if ( u == 0 )
		p = exp(-eta);
	else {
		sum = 0.0;
		for ( l=1; l<=u; l++ )
			sum += exp(-eta-u*log(2)+l*log(eta)-cephes_lgam(l+1)+cephes_lgam(u)-cephes_lgam(l)-cephes_lgam(u-l+1));
		p = sum;
	}
	return p;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                     R A N D O M  E X C U R S I O N S  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double RandomExcursions(char * epsilon, int n)
{
	int		b, i, j, k, J, x;
	int		cycleStart, cycleStop, *cycle = NULL, *S_k = NULL;
	int		stateX[8] = { -4, -3, -2, -1, 1, 2, 3, 4 };
	int		counter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	double	p_value, sum, constraint, nu[6][8];
	double	pi[5][6] = { {0.0000000000, 0.00000000000, 0.00000000000, 0.00000000000, 0.00000000000, 0.0000000000}, 
						 {0.5000000000, 0.25000000000, 0.12500000000, 0.06250000000, 0.03125000000, 0.0312500000},
						 {0.7500000000, 0.06250000000, 0.04687500000, 0.03515625000, 0.02636718750, 0.0791015625},
						 {0.8333333333, 0.02777777778, 0.02314814815, 0.01929012346, 0.01607510288, 0.0803755143},
						 {0.8750000000, 0.01562500000, 0.01367187500, 0.01196289063, 0.01046752930, 0.0732727051} };
	
	if ( ((S_k = (int *)calloc(n, sizeof(int))) == NULL) ||
		 ((cycle = (int *)calloc(MAX(1000, n/200), sizeof(int))) == NULL) ) {
#if 0
		sprintf(msg, "Random Excursions Test:  Insufficent Work Space Allocated.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif

		if ( S_k != NULL )
			free(S_k);
		if ( cycle != NULL )
			free(cycle);

		return(0);
	}
	
	J = 0; 					/* DETERMINE CYCLES */
	S_k[0] = 2*(int)epsilon[0] - 1;
	for( i=1; i<n; i++ ) {
		S_k[i] = S_k[i-1] + 2*epsilon[i] - 1;
		if ( S_k[i] == 0 ) {
			J++;
			if ( J > MAX(1000, n/128) ) {
#if 0
				sprintf(msg, "ERROR IN FUNCTION randomExcursions:  EXCEEDING THE MAX NUMBER OF CYCLES EXPECTED\n.");
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
				free(S_k);
				free(cycle);
				return(0);
			}
			cycle[J] = i;
		}
	}
	if ( S_k[n-1] != 0 )
		J++;
	cycle[J] = n;

	constraint = MAX(0.005*pow(n, 0.5), 500);
	if (J < constraint) {
#if 0
		sprintf(msg, "WARNING:RANDOM EXCURSION:INSUFFICIENT NUMBER OF CYCLES.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		if ( S_k != NULL )
			free(S_k);
		if ( cycle != NULL )
			free(cycle);

		return(0);
	}
	else {
		cycleStart = 0;
		cycleStop  = cycle[1];
		for ( k=0; k<6; k++ )
			for ( i=0; i<8; i++ )
				nu[k][i] = 0.;
		for ( j=1; j<=J; j++ ) {                           /* FOR EACH CYCLE */
			for ( i=0; i<8; i++ )
				counter[i] = 0;
			for ( i=cycleStart; i<cycleStop; i++ ) {
				if ( (S_k[i] >= 1 && S_k[i] <= 4) || (S_k[i] >= -4 && S_k[i] <= -1) ) {
					if ( S_k[i] < 0 )
						b = 4;
					else
						b = 3;
					counter[S_k[i]+b]++;
				}
			}
			cycleStart = cycle[j]+1;
			if ( j < J )
				cycleStop = cycle[j+1];
			
			for ( i=0; i<8; i++ ) {
				if ( (counter[i] >= 0) && (counter[i] <= 4) )
					nu[counter[i]][i]++;
				else if ( counter[i] >= 5 )
					nu[5][i]++;
			}
		}
		
		for ( i=0; i<8; i++ ) {
			x = stateX[i];
			sum = 0.;
			for ( k=0; k<6; k++ )
				sum += pow(nu[k][i] - J*pi[(int)fabs(x)][k], 2) / (J*pi[(int)fabs(x)][k]);
			p_value = cephes_igamc(2.5, sum/2.0);
			
			if ( isNegative(p_value) || isGreaterThanOne(p_value) ) {
#if 0
				sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE.\n");
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
			}

			if ( p_value < ALPHA ) {
				free(S_k);
				free(cycle);
				return(p_value);
			}				
		}
	} 
	free(S_k);
	free(cycle);
	return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            R A N D O M  E X C U R S I O N S  V A R I A N T  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double RandomExcursionsVariant(char *epsilon, int n)
{
	int		i, p, J, x, constraint, count, *S_k;
	int		stateX[18] = { -9, -8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	double	p_value;
	
	if ( (S_k = (int *)calloc(n, sizeof(int))) == NULL ) {
#if 0
		sprintf(msg, "RANDOM EXCURSIONS VARIANT: Insufficent memory allocated.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		return(0);
	}
	J = 0;
	S_k[0] = 2*(int)epsilon[0] - 1;
	for ( i=1; i<n; i++ ) {
		S_k[i] = S_k[i-1] + 2*epsilon[i] - 1;
		if ( S_k[i] == 0 )
			J++;
	}
	if ( S_k[n-1] != 0 )
		J++;

	constraint = (int)MAX(0.005*pow(n, 0.5), 500);
	if (J < constraint) {
#if 0
		sprintf(msg, "RANDOM EXCURSIONS VARIANT: INSUFFICIENT NUMBER OF CYCLES.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		if ( S_k != NULL ) {
			free(S_k);
		}
		return(0);
	}
	else {
		for ( p=0; p<=17; p++ ) {
			x = stateX[p];
			count = 0;
			for ( i=0; i<n; i++ )
				if ( S_k[i] == x )
					count++;
			p_value = erfc(fabs(count-J)/(sqrt(2.0*J*(4.0*fabs(x)-2))));

			if ( isNegative(p_value) || isGreaterThanOne(p_value) ) { 
#if 0
				sprintf(msg, "WARNING: P_VALUE IS OUT OF RANGE.\n");
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
			}
			
			if ( p_value < ALPHA ) {
				free(S_k);
				return(p_value);
			}
		}
	}
	free(S_k);
	return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              R A N K  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Rank(char *epsilon, int n)
{
	int			N, i, k, r;
	double		p_value, product, chi_squared, arg1, p_32, p_31, p_30, R, F_32, F_31, F_30;
	BitSequence	**matrix = create_matrix(32, 32);
	
	N = n/(32*32);

	r = 32;					/* COMPUTE PROBABILITIES */
	product = 1;
	for ( i=0; i<=r-1; i++ )
		product *= ((1.e0-pow(2, i-32))*(1.e0-pow(2, i-32)))/(1.e0-pow(2, i-r));

	p_32 = pow(2, r*(32+32-r)-32*32) * product;

	r = 31;
	product = 1;
	for ( i=0; i<=r-1; i++ )
		product *= ((1.e0-pow(2, i-32))*(1.e0-pow(2, i-32)))/(1.e0-pow(2, i-r));
	p_31 = pow(2, r*(32+32-r)-32*32) * product;

	p_30 = 1 - (p_32+p_31);

	F_32 = 0;
	F_31 = 0;
	for ( k=0; k<N; k++ ) {			/* FOR EACH 32x32 MATRIX   */
		def_matrix(epsilon, 32, 32, matrix, k);
		R = computeRank(32, 32, matrix);
		if ( R == 32 )
			F_32++;			/* DETERMINE FREQUENCIES */
		if ( R == 31 )
			F_31++;
	}
	F_30 = (double)N - (F_32+F_31);
		
	chi_squared =(pow(F_32 - N*p_32, 2)/(double)(N*p_32) +
				  pow(F_31 - N*p_31, 2)/(double)(N*p_31) +
				  pow(F_30 - N*p_30, 2)/(double)(N*p_30));
		
	arg1 = -chi_squared/2.e0;

	p_value = exp(arg1);
	if ( isNegative(p_value) || isGreaterThanOne(p_value) ) { 
#if 0
		sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
	}

	for ( i=0; i<32; i++ )				/* DEALLOCATE MATRIX  */
		free(matrix[i]);
	free(matrix);

	return( p_value < ALPHA ? (p_value) : 0 );
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              R U N S  T E S T 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Runs(char *epsilon, int n)
{
	int		S, k;
	double	pi, V, erfc_arg, p_value;

	S = 0;
	for ( k=0; k<n; k++ )
		if ( epsilon[k] )
			S++;
	pi = (double)S / (double)n;

	if ( fabs(pi - 0.5) > (2.0 / sqrt(n)) ) {
#if 0
		sprintf(msg, "PI ESTIMATOR CRITERIA NOT MET! PI = %f\n", pi);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		return(0);
	}
	else {

		V = 1;
		for ( k=1; k<n; k++ )
			if ( epsilon[k] != epsilon[k-1] )
				V++;
	
		erfc_arg = fabs(V - 2.0 * n * pi * (1-pi)) / (2.0 * pi * (1-pi) * sqrt(2*n));
		p_value = erfc(erfc_arg);
		
		if ( isNegative(p_value) || isGreaterThanOne(p_value) ) {
#if 0
			sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE.\n");
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		}

		return( p_value < ALPHA ? (p_value) : 0);
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                              S E R I A L  T E S T 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Serial(char *epsilon, int n, int m)
{
	double	p_value1, p_value2, psim0, psim1, psim2, del1, del2;
	
	psim0 = psi2(epsilon, m, n);
	psim1 = psi2(epsilon, m-1, n);
	psim2 = psi2(epsilon, m-2, n);
	del1 = psim0 - psim1;
	del2 = psim0 - 2.0*psim1 + psim2;
	p_value1 = cephes_igamc(pow(2, m-1)/2, del1/2.0);
	p_value2 = cephes_igamc(pow(2, m-2)/2, del2/2.0);
	
	return( ((p_value1 < ALPHA) || (p_value2 < ALPHA)) ? (p_value1) : 0 );
}

double psi2(char *epsilon, int m, int n)
{
	int				i, j, k, powLen;
	double			sum, numOfBlocks;
	unsigned int	*P;
	
	if ( (m == 0) || (m == -1) )
		return 0.0;
	numOfBlocks = n;
	powLen = (int)pow(2, m+1)-1;
	if ( (P = (unsigned int*)calloc(powLen,sizeof(unsigned int)))== NULL ) {
#if 0
		sprintf(msg, "Serial Test:  Insufficient memory available.\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		return 0.0;
	}
	for ( i=1; i<powLen-1; i++ )
		P[i] = 0;	  /* INITIALIZE NODES */
	for ( i=0; i<numOfBlocks; i++ ) {		 /* COMPUTE FREQUENCY */
		k = 1;
		for ( j=0; j<m; j++ ) {
			if ( epsilon[(i+j)%n] == 0 )
				k *= 2;
			else if ( epsilon[(i+j)%n] == 1 )
				k = 2*k+1;
		}
		P[k-1]++;
	}
	sum = 0.0;
	for ( i=(int)pow(2, m)-1; i<(int)pow(2, m+1)-1; i++ )
		sum += pow(P[i], 2);
	sum = (sum * pow(2, m)/(double)n) - (double)n;
	free(P);
	
	return sum;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
                         U N I V E R S A L  T E S T
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double Universal(char *epsilon, int n)
{
	int		i, j, p, L, Q, K;
	double	arg, sqrt2, sigma, phi, sum, p_value, c;
	long	*T, decRep;
	double	expected_value[17] = { 0, 0, 0, 0, 0, 0, 5.2177052, 6.1962507, 7.1836656,
				8.1764248, 9.1723243, 10.170032, 11.168765,
				12.168070, 13.167693, 14.167488, 15.167379 };
	double   variance[17] = { 0, 0, 0, 0, 0, 0, 2.954, 3.125, 3.238, 3.311, 3.356, 3.384,
				3.401, 3.410, 3.416, 3.419, 3.421 };
	
	/* * * * * * * * * ** * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	 * THE FOLLOWING REDEFINES L, SHOULD THE CONDITION:     n >= 1010*2^L*L       *
	 * NOT BE MET, FOR THE BLOCK LENGTH L.                                        *
	 * * * * * * * * * * ** * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	L = 5;
	if ( n >= 387840 )     L = 6;
	if ( n >= 904960 )     L = 7;
	if ( n >= 2068480 )    L = 8;
	if ( n >= 4654080 )    L = 9;
	if ( n >= 10342400 )   L = 10;
	if ( n >= 22753280 )   L = 11;
	if ( n >= 49643520 )   L = 12;
	if ( n >= 107560960 )  L = 13;
	if ( n >= 231669760 )  L = 14;
	if ( n >= 496435200 )  L = 15;
	if ( n >= 1059061760 ) L = 16;
	
	Q = 10*(int)pow(2, L);
	K = (int) (floor(n/L) - (double)Q);	 		    /* BLOCKS TO TEST */
	
	p = (int)pow(2, L);
	if ( (L < 6) || (L > 16) || ((double)Q < 10*pow(2, L)) ||
		 ((T = (long *)calloc(p, sizeof(long))) == NULL) ) {
#if 0
		sprintf(msg, "Error condition in Universla test.L=%d, Q=%lf\n", L, Q);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		return(0);
	}
	
	/* COMPUTE THE EXPECTED:  Formula 16, in Marsaglia's Paper */
	c = 0.7 - 0.8/(double)L + (4 + 32/(double)L)*pow(K, -3/(double)L)/15;
	sigma = c * sqrt(variance[L]/(double)K);
	sqrt2 = sqrt(2);
	sum = 0.0;
	for ( i=0; i<p; i++ )
		T[i] = 0;
	for ( i=1; i<=Q; i++ ) {		/* INITIALIZE TABLE */
		decRep = 0;
		for ( j=0; j<L; j++ )
			decRep += epsilon[(i-1)*L+j] * (long)pow(2, L-1-j);
		T[decRep] = i;
	}
	for ( i=Q+1; i<=Q+K; i++ ) { 	/* PROCESS BLOCKS */
		decRep = 0;
		for ( j=0; j<L; j++ )
			decRep += epsilon[(i-1)*L+j] * (long)pow(2, L-1-j);
		sum += log(i - T[decRep])/log(2);
		T[decRep] = i;
	}
	phi = (double)(sum/(double)K);

	arg = fabs(phi-expected_value[L])/(sqrt2 * sigma);
	p_value = erfc(arg);
	if ( isNegative(p_value) || isGreaterThanOne(p_value) ) {
#if 0
		sprintf(msg, "WARNING:  P_VALUE IS OUT OF RANGE\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
	}
	
	free(T);

	return( p_value < ALPHA ? (p_value) : 0 );
}

