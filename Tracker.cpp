/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */

#include "Tracker.h"

Tracker::Tracker():m_max_radius(7), m_max_missing_duration(5)
{
	for(int i=0; i<MAXHANDS; i++) handids[i] = false;
	last_handid = 0;
}

Tracker::~Tracker(){}

std::vector<cBlob>& Tracker::getBlobs()
{
	return blobs;
}

void Tracker::drawBlobs(cv::Mat &out){

	for (int i = 0; i < blobs.size(); i++) {
		cBlob &b = blobs[i];
		cv::Scalar col;
		if( b.event == BLOB_DOWN ){
				col = cv::Scalar(255, 100, 100);
		}else if( b.event == BLOB_UP){
				col = cv::Scalar(40, 40, 255);
		}else if( b.event == BLOB_MOVE){
				col = cv::Scalar(40, 255, 40);
		}else if( b.event == BLOB_PENDING){
				col = cv::Scalar(150, 150, 150);
		}

		if( b.missing_duration == 0 
				&& b.duration > 5
				){
#if 1
			cv::line(out,
					cv::Point((int)b.origin.x,(int)b.origin.y),
					cv::Point((int)b.location.x,(int)b.location.y),cv::Scalar(200,200,200),1);
#endif
			cv::rectangle(out,
					cv::Point((int)b.min.x,(int)b.min.y),
					cv::Point((int)b.max.x,(int)b.max.y),col,1);
		}
	}
}
