#include <sys/time.h>

// for FPS estimation.
class Fps
{
private:
	int frame;
	int mod;
	struct timeval earlier;
	struct timeval later;
	struct timeval interval;
	long long diff;
	void tic(){
		if(gettimeofday(&earlier,NULL))
			perror("first gettimeofday()");
	}

	void toc(){
		if(gettimeofday(&later,NULL))
			perror("first gettimeofday()");
		else
			diff = timeval_diff(&interval, &later, &earlier);
	}
	long long
		timeval_diff(struct timeval *difference,
				struct timeval *end_time,
				struct timeval *start_time
				)   
		{
			struct timeval temp_diff;

			if(difference==NULL)
			{
				difference=&temp_diff;
			}

			difference->tv_sec =end_time->tv_sec -start_time->tv_sec ;
			difference->tv_usec=end_time->tv_usec-start_time->tv_usec;

			/* Using while instead of if below makes the code slightly more robust. */

			while(difference->tv_usec<0)
			{
				difference->tv_usec+=1000000;
				difference->tv_sec -=1;
			}

			return 1000000LL*difference->tv_sec+
				difference->tv_usec;

		} /* timeval_diff() */

public:
	void next(FILE *stream){
		frame++;
		if( frame%mod == 0){
			toc();
			if( stream != NULL ){
				double fps = (1000000*(double)mod)/diff;
				fprintf(stream, "frames: %i, dt: %4.1f ms \tfps: %4.2f\n", mod , diff/1000.0, fps );
			}
			tic();
		}
	}

	Fps():frame(0),mod(500){ 
			tic();
		}
	Fps(int f):frame(0),mod(f){ 
			tic();
		}
};

