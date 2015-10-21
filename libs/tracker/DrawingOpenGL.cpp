/* 
 * Static drawing function for OpenGL.
 */
#include <unistd.h>
#include <algorithm> //for std::sort

#include "Tracker.h"

#ifdef WITH_OPENGL
#include "TrackerDrawingOpenGL.h"
#include "DrawingFunctions.h"

#ifdef WITH_HISTORY
/* Connect midpoints of the saved history of a blob */
void tracker_drawHistory( Tracker &tracker, int screenWidth, int screenHeight, cBlob &blob, GfxTexture *target){
	if( blob.history.get() == nullptr ) return;

	unsigned int numPoints = 1+blob.history.get()->size();
	float scaleW = 2.0/screenWidth;
	float scaleH = 2.0/screenHeight;
	GLfloat points[numPoints*2];
	GLfloat *p = &points[0];

	/* Do not generate colors for each line, but use member variable. */
	//GLfloat colors[numPoints*4];
	//GLfloat *c = &colors[0];

	float x0,y0;
	//Use current blob as first point. Flip x coordinates
	x0 = 1 - blob.location.x*scaleW;
	y0 = blob.location.y*scaleH - 1;
	*p++ = x0; 	*p++ = y0; 	
	//*c++ = 1.0; *c++ = 0.0; *c++ = 1.0; *c++ = 0.7;

	int pointIndex = 1;
	std::deque<cBlob>::iterator it = blob.history.get()->begin();
	const std::deque<cBlob>::iterator itEnd = blob.history.get()->end();
	for ( ; it != itEnd ; ++it){
		x0 = 1 - (*it).location.x*scaleW;
		y0 = (*it).location.y*scaleH - 1;
		*p++ = x0; 	*p++ = y0; 	
		//*c++ = 1.0-pointIndex/30.0; *c++ = 0.0; *c++ = 1.0; *c++ = 0.7;

		++pointIndex;
	}
		   
	DrawColouredLines(&points[0], tracker.m_phistory_line_colors /*&colors[0]*/, pointIndex, target);

}
#endif

void tracker_drawBlobsGL(Tracker &tracker, int screenWidth, int screenHeight, bool drawHistoryLines, std::vector<cBlob> *toDraw, GfxTexture *target){
	if( toDraw == NULL ){
		//toDraw = &tracker.blobs;
		toDraw = &tracker.getBlobs();
	}

	/* Clear target. It's not neccessary to unbind the
	 * framebuffer because subfunctions will do this already. */
	if( target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,target->getFramebufferId());
		glViewport ( 0, 0, target->getWidth(), target->getHeight() );
		glClear(GL_COLOR_BUFFER_BIT);
	}

	//Wait and look mutex.
	while( tracker.m_swap_mutex ){
		usleep(900);
	}
	tracker.m_swap_mutex = 1;

	GLfloat points[toDraw->size()*12];
	GLfloat colors[toDraw->size()*24];
	int quadIndex = 0;
	float scaleW = 2.0/screenWidth;
	float scaleH = 2.0/screenHeight;
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
		x0 = 1-b.min.x*scaleW;
		y0 = b.min.y*scaleH-1;
		x1 = 1-b.max.x*scaleW;
		y1 = b.max.y*scaleH-1;

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

		if( drawHistoryLines ){
#ifdef WITH_HISTORY
			tracker_drawHistory(tracker, screenWidth, screenHeight, b, target);
#endif
		}

	}
	tracker.m_swap_mutex = 0;

	DrawBlobRects(&points[0], &colors[0], quadIndex, target);

}

#endif
