/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */
#include <unistd.h>

#include <algorithm> //for std::sort

#include "Tracker.h"

bool oldest_sort_function (const cBlob &a,const cBlob &b) { return (a.duration>b.duration); }


Tracker::Tracker():m_frameId(0),m_max_radius(7), m_max_missing_duration(5), m_swap_mutex(0),
	m_use_N_oldest_blobs(0),
#ifdef WITH_HISTORY
	m_phistory_line_colors(NULL),
#endif
	m_minimal_frames_till_active(10)
{
	for(unsigned int i=0; i<MAXHANDS; i++) handids[i] = false;
	last_handid = 0;

#ifdef WITH_HISTORY
	m_phistory_line_colors = new float[MAX_HISTORY_LEN*4];
	float *rgba = m_phistory_line_colors;
	const float *end = m_phistory_line_colors+MAX_HISTORY_LEN*4;
	unsigned int seed = 255;
	float div = 1.0/255;
	while(rgba < end ){
		*rgba++ = div*(seed%256);
		*rgba++ = div*(127+(seed+100%128));
		*rgba++ = div*((seed+200)%256);
		*rgba++ = 1.0;
		seed += 5;
	}
#endif

}

Tracker::~Tracker(){
#ifdef WITH_HISTORY
	delete m_phistory_line_colors;
#endif
}

std::vector<cBlob>& Tracker::getBlobs()
{
	return blobs;
}

void Tracker::setMaxRadius(int max_radius){
	 m_max_radius = max_radius;
}
void Tracker::setMaxMissingDuration(int max_missing_duration){
	m_max_missing_duration = max_missing_duration;
}

void Tracker::setMinimalDurationFilter(int M_frames_minimal){
	m_minimal_frames_till_active = M_frames_minimal;
}

void Tracker::setOldestDurationFilter(int N_oldest_blobs){
	m_use_N_oldest_blobs = N_oldest_blobs;
}


void Tracker::getFilteredBlobs(int /*Trackfilter*/ filter, std::vector<cBlob> &output)
{
	/* I-Frames are without motions. Allow one missing frame. 
	 * A general skipping of I-Frames would be a better solution.
	 */
	for (unsigned int i = 0; i < blobs.size(); i++) {
		cBlob &b = blobs[i];
		if( ( filter & b.event )
				|| ( filter & TRACK_ALL )
				|| ( filter & TRACK_ALL_ACTIVE
					&& b.event & (BLOB_MOVE|BLOB_DOWN)
					&& b.missing_duration < m_max_missing_duration
					&& b.duration > m_minimal_frames_till_active ) 
			)
		{
			output.push_back(b);
		}
	}

	/* More filtering */

	/* Reduce output on n oldest blobs (and all blobs
	 * with the same duration as the n-th blob)
	 */
	if( filter & LIMIT_ON_N_OLDEST 
			&& m_use_N_oldest_blobs > 0
			&& output.size() > m_use_N_oldest_blobs )
	{
		std::sort (output.begin(), output.end(), oldest_sort_function);
		int last_index = m_use_N_oldest_blobs-1;
		std::vector<cBlob>::iterator it=output.begin()+(m_use_N_oldest_blobs-1);
		const int limit_duration = (*it).duration;
		++it;
		while( it!=output.end() && (*it).duration == limit_duration ){
			++it;
		}
		//printf("Remove %i blobs from list \n", output.end()-it);
		output.erase(it,output.end());
	}
}

