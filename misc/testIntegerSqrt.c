/*
 * Test of several routines for square root
 * function on integer space.
 *
 * Compiling: gcc -o testIntegerSqrt -O3 testIntegerSqrt.c -lm
 * */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>


//=============================================================
/* ; 3cyles/bit Method (fastest)
 * ; IN :  n 32 bit unsigned integer
 * ; OUT:  root = INT (SQRT (n))
 * ; TMP:  offset
 *
 * MOV    offset, #3 << 30
 * MOV    root, #1 << 30
 * [ unroll for i = 0 .. 15
 * CMP    n, root, ROR #2 * i
 * SUBHS  n, n, root, ROR #2 * i
 * ADC    root, offset, root, LSL #1
 * ]
 * BIC    root, root, #3 << 30  ; for rounding add: CMP n, root  ADC root, #1
 */
unsigned int sqrt_asm(unsigned int n){
	unsigned int root, offset;
	asm (
			" mov    %[offset], #3 << 30 \n\t"
			" mov    %[root], #1 << 30 \n\t"
			// Loop 0..15
			" cmp    %[n], %[root], ROR #2 *  0 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  0 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  1 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  1 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  2 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  2 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  3 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  3 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  4 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  4 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  5 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  5 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  6 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  6 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  7 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  7 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  8 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  8 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 *  9 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 *  9 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 * 10 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 * 10 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 * 11 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 * 11 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 * 12 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 * 12 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 * 13 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 * 13 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 * 14 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 * 14 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			" cmp    %[n], %[root], ROR #2 * 15 \n\t"
			" subhs  %[n], %[n], %[root], ROR #2 * 15 \n\t"
			" adc    %[root], %[offset], %[root], LSL #1 \n\t"
			//
			" bic    %[root], %[root], #3 << 30 \n\t"
			: [root] "=r" (root) /*oder =&r */
			: [offset] "r" (offset), [n] "r" (n)
			:  "r0" 
			);

	return root;
}


//=============================================================
// 4 cycle/bit C routine
#define iter1(N) \
	try = root + (1 << (N)); \
if (n >= try << (N))   \
{   n -= try << (N);   \
	root |= 2 << (N); \
}

unsigned int sqrt2 (unsigned int n)
{
	unsigned int root = 0, try;
	iter1 (15);    iter1 (14);    iter1 (13);    iter1 (12);
	iter1 (11);    iter1 (10);    iter1 ( 9);    iter1 ( 8); 
	iter1 ( 7);    iter1 ( 6);    iter1 ( 5);    iter1 ( 4); 
	iter1 ( 3);    iter1 ( 2);    iter1 ( 1);    iter1 ( 0); 
	return root >> 1;
}

//=============================================================
// Only for values <= 2^15 required because 2^15>= a*a + b*b for a,b ∈ Char
static unsigned int m_root[129];
unsigned int mem_lookup(unsigned int s){

#if 1
	if( s>>11 ){
		s =  *(m_root+(s>>8)) >> (16-4);
	}else if( s>>7 ){
		s =  *(m_root+(s>>4)) >> (16-2);
	}else {
		s = *(m_root+s) >> 16;
	}
#else
	//runde rest r/d immer zu 1 auf.
	if( s>>11 ){
		s =  *(m_root+(s>>8) +1) >> (16-4);
	}else if( s>>7 ){
		s =  *(m_root+(s>>4) +1) >> (16-2);
	}else {
		s = *(m_root+s) >> 16;
	}
#endif
	return s;
}



long long update_fps()
{
	static int frame_count = 0;
	static long long time_start = 0;
	long long time_diff = 0; //in milliseconds
	static long long time_now = 0;
	struct timeval te; 
	float fps;

	frame_count++;

	gettimeofday(&te, NULL);
	if( time_now ){
		time_diff = te.tv_sec * 1000LL + te.tv_usec / 1000 - time_now;
		time_now += time_diff;                                                             
	}else{                                                                                 
		time_now = te.tv_sec * 1000LL + te.tv_usec / 1000;
	}

	if (time_start == 0)
	{
		time_start = time_now;
	}
	else if (time_now - time_start > 5000)
	{
		fps = (float) frame_count / ((time_now - time_start) / 1000.0);
		frame_count = 0;
		time_start = time_now;
		printf("%3.2f FPS\n", fps); fflush(stdout);
	}
}

void testErrors(unsigned int N, unsigned int (*handler ) (unsigned int) ){

	int errorcount = 0;
	int errorcountHigh = 0;
	int errorsum = 0;
	int errorsumHigh = 0;

	unsigned int i,imax, s1, s2;
	s1 = UINT_MAX;
	s2 = UINT_MAX;

	for ( i=0,imax=N; i<imax; ++i){
		s1 = handler(i);

		s2 = sqrt2(i);
		if( s1 != s2 ){
			errorcount++;
			errorsum+= abs(s1-s2);
			//Measure errors > 1
			if( abs(s1-s2)>1){
				//printf("Square root evaluation error sqrt(%u) = %u != %u\n", i, s1, s2);
				errorcountHigh++;
				errorsumHigh += abs(s1-s2);
			}
		}else{
				//printf("Ok sqrt(%u) = %u != %u\n", i%129, s1, s2);
		}
	}

	printf("errors on %i positions ( %f% )\n", errorcount, 100*errorcount/(float)(1<<15));
	printf("Mean error:  %f\n", errorsum, errorsum/(float)(errorcount));
	printf("High error (>1) on %i positions ( %f% )\n", errorcountHigh, 100*errorcountHigh/(float)(1<<15));
	printf("Mean high error:  %f\n", errorsumHigh, errorsumHigh/(float)(errorcountHigh));

}

void testPerformance(unsigned int N, unsigned int (*handler ) (unsigned int) ){
	unsigned int i,imax, s1, s2;

	s1 = UINT_MAX;
	s2 = UINT_MAX;

	for ( i=0,imax=N; i<imax; ++i){
		//use i%2^15 because array based algorithm ist not defined for higher values.
		s1 = handler( i&0xFFFF  );
		if( i%(1<<16) == 0 ){
			update_fps();
		}
	}
}

int main (int argn, char** argv ) {
	unsigned int i,imax, s1, s2;

	unsigned int (*handler ) (unsigned int);
	handler = &sqrt2;
	int method = 0;
	int errorcount = 0;
	int errorcountHigh = 0;
	int errorsum = 0;
	int errorsumHigh = 0;

	if( argn>1 && argv[1][0] == '2' ){
		handler = &mem_lookup;
		printf("Use prefilled array routine\n");
		int n;
		for(n=0;n<129;++n){
			//m_root[n] = isqrt(n);
			m_root[n] = sqrtf( n) * (1<<16);
			//printf("m_root(%i)=%u \n",n,m_root[n]);
		}
	}else	if( argn>1 && argv[1][0] == '1' ){
		handler = &sqrt_asm;
		printf("Use assembler routine\n");
	}else{
		printf("Use c-code routine\n");
	}

	unsigned int N = 1<<15;

	testErrors(N, handler);

	N = 1<<31;
	testPerformance(N, handler);


	return 0;
}

