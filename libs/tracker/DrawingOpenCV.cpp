/* 
 * Static drawing function for OpenCV.
 */
#include <unistd.h>

#include "Tracker.h"
#include "TrackerDrawingOpenCV.h"


#ifdef WITH_OCV
/* This method is a little bit outdated. The OpenGL variant is more actual. */
void tracker_drawBlobs(Tracker &tracker, cv::Mat &out, bool drawHistoryLines, std::vector<cBlob> *toDraw ){
	if( toDraw == NULL ){
		toDraw = &tracker.getBlobs();
	}

	for (unsigned int i = 0; i < toDraw->size(); i++) {
		cBlob &b = (*toDraw)[i];
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

		cv::rectangle(out,
				cv::Point((int)b.min.x,(int)b.min.y),
				cv::Point((int)b.max.x,(int)b.max.y),col,1);

		if( drawHistoryLines ){
#ifdef WITH_HISTORY
			if( b.history.get() != nullptr ){
				cv::Point p1((int)b.location.x,(int)b.location.y);
				cv::Point p2;

				std::deque<cBlob>::iterator it = b.history.get()->begin();
				const std::deque<cBlob>::iterator itEnd = b.history.get()->end();
				int time=0;
				for ( ; it != itEnd ; ++it){
					p2.x = (*it).location.x; 	p2.y = (*it).location.y; 	
					//cv::Scalar color(200+10*time,200+10*time,200+10*time);
					cv::Scalar color(30,30,200+10*time);
					cv::line(out,p1,p2,color,2);

					//printf("Draw line from (%i,%i) to (%i,%i)\n", p1.x,p1.y, p2.x,p2.y);
					p1 = p2;
					--time;
				}
			}
#else
			cv::line(out,
					cv::Point((int)b.origin.x,(int)b.origin.y),
					cv::Point((int)b.location.x,(int)b.location.y),cv::Scalar(200,200,200),1);
#endif
		}

	}
}
#endif

