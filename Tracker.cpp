/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */
#include <unistd.h>

#include <algorithm> //for std::sort

#include "Tracker.h"
#ifdef WITH_OPENGL
#include "apps/raspicam/Graphics.h"
#endif

bool oldest_sort_function (const cBlob &a,const cBlob &b) { return (a.duration>b.duration); }


Tracker::Tracker():m_max_radius(7), m_max_missing_duration(5), m_swap_mutex(0),
	m_use_N_oldest_blobs(0),
	m_minimal_frames_till_active(10)
{
	for(unsigned int i=0; i<MAXHANDS; i++) handids[i] = false;
	last_handid = 0;
}

Tracker::~Tracker(){}

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
		printf("Remove %i blobs from list \n", output.end()-it);
		output.erase(it,output.end());
	}
}

#ifdef WITH_OCV
/* This method is a little bit outdated. The OpenGL variant is more actual. */
void Tracker::drawBlobs(cv::Mat &out, std::vector<cBlob> *toDraw ){
	if( toDraw == NULL ){
		toDraw = &blobs;
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

				printf("Draw line from (%i,%i) to (%i,%i)\n", p1.x,p1.y, p2.x,p2.y);
				p1 = p2;
				--time;
			}
		}
#else
		cv::line(out,
				cv::Point((int)b.origin.x,(int)b.origin.y),
				cv::Point((int)b.location.x,(int)b.location.y),cv::Scalar(200,200,200),1);
#endif
		cv::rectangle(out,
				cv::Point((int)b.min.x,(int)b.min.y),
				cv::Point((int)b.max.x,(int)b.max.y),col,1);
	}
}
#endif

#ifdef WITH_OPENGL
void Tracker::drawBlobsGL(int screenWidth, int screenHeight, std::vector<cBlob> *toDraw, GfxTexture *target){
	if( toDraw == NULL ){
		toDraw = &blobs;
	}

	//Wait and look mutex.
	while( m_swap_mutex ){
		usleep(900);
	}
	m_swap_mutex = 1;

	GLfloat points[toDraw->size()*12];
	GLfloat colors[toDraw->size()*24];
	int quadIndex = 0;
	GLfloat *p = &points[0];
	GLfloat *c = &colors[0];

	for (unsigned int i = 0; i < toDraw->size(); i++) {
		cBlob &b = (*toDraw).at(i);
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

		//printf("Draw blob! [%i-%i] x [%i-%i]\n", b.min.x, b.max.x, b.min.y, b.max.y);
		float x0,y0,x1,y1;
		/* Flip x0 with x1 to get same flipping as video frame */
		x0 = 1-2.0*b.min.x/screenWidth;
		y0 = 2.0*b.min.y/screenHeight-1;
		x1 = 1-2.0*b.max.x/screenWidth;
		y1 = 2.0*b.max.y/screenHeight-1;
		//x0 = -x0; x1 = -x1;
		//y0 = -y0; y1 = -y1;

		//printf("  [%f-%f] x [%f-%f]\n", x0, x1, y0, y1);
		//DrawBlobRect( col[0], col[1], col[2], x0,y0, x1, y1, NULL);

		// Triangle A-B-C
		*p++ = x0; *p++ = y0;
		*p++ = x0; *p++ = y1;
		*p++ = x1; *p++ = y0;

		// Triangle C-B-D
		*p++ = x1; *p++ = y0;
		*p++ = x0; *p++ = y1;
		*p++ = x1; *p++ = y1;

		// Colors for all 6 Vertices
		*c++ = col[0]; *c++ = col[1]; *c++ = col[2]; *c++ = 0.7;
		*c++ = col[0]; *c++ = col[1]; *c++ = col[2]; *c++ = 0.7;
		*c++ = col[0]; *c++ = col[1]; *c++ = col[2]; *c++ = 0.7;
		*c++ = col[0]; *c++ = col[1]; *c++ = col[2]; *c++ = 0.7;
		*c++ = col[0]; *c++ = col[1]; *c++ = col[2]; *c++ = 0.7;
		*c++ = col[0]; *c++ = col[1]; *c++ = col[2]; *c++ = 0.7;

		++quadIndex;

		drawHistory(screenWidth, screenHeight, b, target);
	}
	m_swap_mutex = 0;

	DrawBlobRects(&points[0], &colors[0], quadIndex, target);

}

#ifdef WITH_HISTORY
/* Connect midpoints of the saved history of a blob */
void Tracker::drawHistory( int screenWidth, int screenHeight, cBlob &blob, GfxTexture *target){
	if( blob.history.get() == nullptr ) return;

	unsigned int numPoints = 1+blob.history.get()->size();
	GLfloat points[numPoints*2];
	GLfloat colors[numPoints*4];
	GLfloat *p = &points[0];
	GLfloat *c = &colors[0];
	/* TODO: Use a static, prefilled array for the colors. */

	float x0,y0;
	//Use current blob as first point. Flip x coordinates
	x0 = 1-2.0*blob.location.x/screenWidth;
	y0 = 2.0*b.location.y/screenHeight-1;
	*p++ = x0; 	*p++ = y0; 	
	*c++ = 1.0; *c++ = 0.0; *c++ = 1.0; *c++ = 0.7;

	int pointIndex = 1;
	std::deque<cBlob>::iterator it = blob.history.get()->begin();
	const std::deque<cBlob>::iterator itEnd = blob.history.get()->end();
	for ( ; it != itEnd ; ++it){
		x0 = 1-2.0* (*it).location.x/screenWidth;
		y0 = 2.0* (*it).location.y/screenHeight-1;
		*p++ = x0; 	*p++ = y0; 	
		*c++ = 1.0; *c++ = 0.0; *c++ = 1.0; *c++ = 0.7;

		++pointIndex;
	}
		   
	DrawColouredLines(&points[0], &colors[0], pointIndex, target);

}
#endif



#endif
