/* Fast implementations of sqrt(a*a + b*b) for a,b in [-127,128]
 * See misc/testIntegerSqrt.c for performance test.
 *
 * The fastest algorithm (3) is inaccurate for high absolute values,
 * which should be no problem. 
 *
 * Select algorithm with NORM2_USE_ALGO
 *
 * Algorithm 1 - Use normal c function for integer sqrt.
 * Algorithm 2 - Use assembler optimized function for integer sqrt.
 * Algorithm 3 - Use lookup map for low values and approximation for higher values.
 *
 */

//#include <limits.h>
#include <stdlib.h>
#include <math.h>

#include "norm2.h"

unsigned int norm2_pow_map[256];//for -128,...,0,...,127
unsigned int norm2_root_map[129];//maps on sqrt(a)*2^16

/* Fill arrays on startup */
void norm2_init_arrays(){
	unsigned char i=255;
	while( i ){
		int8_t tmp = (char)i;
		int p = tmp*tmp;
		norm2_pow_map[i] = p;
		--i;	
	}
	norm2_pow_map[0] = 0;

	int n=128;
	while( n ){
		norm2_root_map[n] = sqrtf(n) * (1<<16);
		--n;
	}
	norm2_root_map[0] = 0;

}


/* 4 cycle/bit C routine
 * http://www.finesse.demon.co.uk/steven/sqrt.html
 *  */
#define iter1(N) \
	try = root + (1 << (N)); \
if (n >= try << (N))   \
{   n -= try << (N);   \
	root |= 2 << (N); \
}

inline unsigned int sqrt2 ( unsigned int n)
{
	unsigned int root = 0, try;
	iter1 (15);    iter1 (14);    iter1 (13);    iter1 (12);
	iter1 (11);    iter1 (10);    iter1 ( 9);    iter1 ( 8); 
	iter1 ( 7);    iter1 ( 6);    iter1 ( 5);    iter1 ( 4); 
	iter1 ( 3);    iter1 ( 2);    iter1 ( 1);    iter1 ( 0); 
	return root >> 1;
}


/*  3cyles/bit Method
 * http://www.finesse.demon.co.uk/steven/sqrt.html
 *  */
inline unsigned int sqrt_asm( const unsigned int n){
	volatile unsigned int root;
	unsigned int offset;
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
			: [root] "=&r" (root) 
			: [offset] "r" (offset), [n] "r" (n)
			:  
			);

	return root;
}


/* Only for values <= 2^15 defined/required.
 * Own algorithm (Olaf Schulz)
 */
/*inline*/ unsigned int mem_lookup(unsigned int s){
	if( s>>11 ){
		s =  *(norm2_root_map+(s>>8)) >> (16-4);
	}else if( s>>7 ){
		s =  *(norm2_root_map+(s>>4)) >> (16-2);
	}else {
		s = *(norm2_root_map+s) >> 16;
	}
	return s;
}


unsigned int norm2(const signed char a, const signed char b){
	unsigned int s = *(norm2_pow_map+(unsigned char)a)
		+ *(norm2_pow_map+(unsigned char)b );
	//unsigned int s = a*a+b*b;

#if NORM2_USE_ALGO == 1
	return sqrt2(s);
#endif
#if NORM2_USE_ALGO == 2
	return sqrt_asm(s);
#endif
#if NORM2_USE_ALGO == 3
	return mem_lookup(s);
#endif

	return -1;
}


