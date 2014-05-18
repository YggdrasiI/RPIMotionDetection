/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */
#include <unistd.h>

#include "Tracker.h"
#ifdef WITH_OPENGL
#include "apps/raspicam/Graphics.h"
#endif

Tracker::Tracker():m_max_radius(7), m_max_missing_duration(5), m_swap_mutex(0)
{
	for(int i=0; i<MAXHANDS; i++) handids[i] = false;
	last_handid = 0;
}

Tracker::~Tracker(){}

std::vector<cBlob>& Tracker::getBlobs()
{
	return blobs;
}

#ifdef WITH_OPENCV
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
#endif

#ifdef WITH_OPENGL
void Tracker::drawBlobsGL(int screenWidth, int screenHeight){

	//Wait and look mutex.
	while( m_swap_mutex ){
		usleep(1000);
	}
	m_swap_mutex = 1;

	for (int i = 0; i < blobs.size(); i++) {
		cBlob &b = blobs[i];
		float col[3];
#define C(r,g,b) {col[0]=(r)/255.0; col[1]=(g)/255.0; col[2]=(b)/255.0;}
		if( b.event == BLOB_DOWN ){
				C(255, 100, 100);
		}else if( b.event == BLOB_UP){
				C(40, 40, 255);
		}else if( b.event == BLOB_MOVE){
				C(40, 255, 40);
		}else if( b.event == BLOB_PENDING){
				C(150, 150, 150);
		}

		if( ( b.missing_duration == 0 
					&& b.duration > 5 )
				/* || b.duration > 1 */
				){
			//printf("Draw blob! [%i-%i] x [%i-%i]\n", b.min.x, b.max.x, b.min.y, b.max.y);
			float x0,y0,x1,y1;
			x0 = 2.0*b.min.x/screenWidth-1;
			y0 = 2.0*b.min.y/screenHeight-1;
			x1 = 2.0*b.max.x/screenWidth-1;
			y1 = 2.0*b.max.y/screenHeight-1;
			/* Flip x0 with x1 to get same flipping as video frame */
			x0 = -x0;
			x1 = -x1;

			//printf("  [%f-%f] x [%f-%f]\n", x0, x1, y0, y1);
			DrawBlobRect( col[0], col[1], col[2],
					x0,y0, x1, y1,
					NULL);
		}
	}

	m_swap_mutex = 0;
}
#endif
