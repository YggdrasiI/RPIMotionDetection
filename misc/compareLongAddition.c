/* Compare performance of 
 * operation on long data types 
 * and long long.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>

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


int main(int argn, char** argv ){

	const unsigned int iLoop = 1<<24;
	const unsigned int outLoop = 1<<15;
	const unsigned int arrLen = 1<<10;

	unsigned int a = UINT_MAX;
	volatile/*! force pushing register value into memory */ int b = INT_MAX;
	unsigned long long c = ~0;
	long long d = ~0>>1;

	unsigned int *arrUI = (unsigned int*) calloc(arrLen,sizeof(unsigned int));
	unsigned long long *arrULL = (unsigned long long*) calloc(arrLen,sizeof(unsigned long long));

	printf("unsigned int: %u %f\n", a, log2(a) );
	printf("int: %i %f\n", b, log2(b) );
	printf("unsigned long long: %llu %f\n", c, log2(c) );
	printf("long long: %lli %f\n", d, log2(d) );

#define WRAPPER( CMD ) \
		unsigned int o = outLoop; \
		while( --o ){ \
			unsigned int i = iLoop; \
			while( --i ){ \
				CMD; \
			} \
			update_fps();\
		}\

	if( argn>1 && argv[1][0] == '6' ){
		printf("\nTest long long speed for array (no simple register store)\n");
		int pos = 0;
		WRAPPER( pos=(pos+1234567)%arrLen;arrULL[pos] += pos; )
	}else if( argn>1 && argv[1][0] == '5' ){
		printf("\nTest unsigned int for array (no simple register store)\n");
		int pos = 0;
		WRAPPER( pos=(pos+1234567)%arrLen;arrUI[pos] += pos; )
	}else if( argn>1 && argv[1][0] == '4' ){
		printf("\nTest long long speed\n");
		WRAPPER( d += d; )
	}else if( argn>1 && argv[1][0] == '3' ){
		printf("\nTest unsigned long long speed\n");
		WRAPPER( c += c; )
	}else if( argn>1 && argv[1][0] == '2' ){
		printf("\nTest long (=int) speed. Variable marked volatile\n");
		WRAPPER( b += b; )
	}else{
		printf("\nTest unsigned long (=uint) speed\n");
		WRAPPER( a += a; )
	}

	printf("Output to prevent optimizations. %u %i %u %i\n", a, b, c, d);
	return 0;
}
