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

void Tracker::getFilteredBlobs(Trackfilter filter, std::vector<cBlob> &output)
{
	/* I-Frames are without motions. Allow one missing frame. 
	 * A general skipping of I-Frames would be a better solution.
	 */
	for (int i = 0; i < blobs.size(); i++) {
		cBlob &b = blobs[i];
		if( filter == ALL_ACTIVE && b.missing_duration < 2 && b.duration > 5){
			output.push_back(b);
		}
	}
}

#ifdef WITH_OCV
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

	for (int i = 0; i < toDraw->size(); i++) {
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

		if( 1 )
		{
			//printf("Draw blob! [%i-%i] x [%i-%i]\n", b.min.x, b.max.x, b.min.y, b.max.y);
			float x0,y0,x1,y1;
			x0 = 2.0*b.min.x/screenWidth-1;
			y0 = 2.0*b.min.y/screenHeight-1;
			x1 = 2.0*b.max.x/screenWidth-1;
			y1 = 2.0*b.max.y/screenHeight-1;
			/* Flip x0 with x1 to get same flipping as video frame */
			x0 = -x0; x1 = -x1;
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
		}
	}
	m_swap_mutex = 0;

	DrawBlobRects(&points[0], &colors[0], quadIndex, target);

}
#endif
