/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */

#ifndef TRACKER_H
#define TRACKER_H

#include <stdlib.h>
#include <vector>

#ifdef WITH_OCV
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv2/opencv.hpp>
#endif

#ifdef WITH_OPENGL
class GfxTexture; 
#endif

#include "Blob.h"

#include "threshtree.h"
#include "depthtree.h"


#define MAXHANDS 40

enum Trackfilter{
	TRACK_DOWN = BLOB_DOWN,
	TRACK_MOVE = BLOB_MOVE,
	TRACK_PENDING = BLOB_PENDING,
	TRACK_UP = BLOB_UP,
	TRACK_ALL = BLOB_DOWN|BLOB_MOVE|BLOB_PENDING|BLOB_UP,
	TRACK_ALL_ACTIVE = 16,
	LIMIT_ON_N_OLDEST = 32 ,
};

class Tracker {
	protected:
		int m_frameId; //Increase id for each frame.
		int m_max_radius;
		int m_max_missing_duration;
		int m_use_N_oldest_blobs;
		int m_minimal_frames_till_active;

		std::vector<cBlob> blobs, blobs_previous, blobsTmp /*for swapping */;

		// storage of used handids
		bool handids[MAXHANDS];
		int last_handid;

	public:
		int m_swap_mutex;

#ifdef WITH_HISTORY
		float *m_phistory_line_colors;
#endif

	public:
		Tracker();
		virtual ~Tracker() = 0;
		std::vector<cBlob>& getBlobs();
		void getFilteredBlobs(int /*Trackfilter*/ filter, std::vector<cBlob> &output);

		virtual void trackBlobs(
				Blobtree *frameblobs,
				bool history ) = 0;
		
		/* Set maximal differcene between midpoints of blobs between
		 * two frames F_i and F_{i+k}, k<= m_max_missing_duration */
		void setMaxRadius(int max_radius);

		/* Set number of frames which a blobs waits for
		 * an new nearby detection */
		void setMaxMissingDuration(int max_missing_duration);

		/* Set the number of frames after which a blob
		 * gets active. */
		void setMinimalDurationFilter(int M_frames_minimal);

		/* Just returns the N oldest blobs. This reduce the
		 * number of blobs during choppy frames. Internally,
		 * the blobs will be still saved. Thus, this setting
		 * requires a high minimalDurationFilter() value.
		 */
		void setOldestDurationFilter(int N_oldest_blobs);


#ifdef WITH_OCV
		/* Helper function to draw blobs for debugging */
		void tracker_drawBlobs(Tracker &tracker, cv::Mat &out, bool drawHistoryLines, std::vector<cBlob> *toDraw = NULL );
#endif
#ifdef WITH_OPENGL
		/* Helper function to draw blobs for debugging */
		void tracker_drawBlobsGL(Tracker &tracker, int screenWidth, int screenHeight, bool drawHistoryLines =     false, std::vector<cBlob> *toDraw = NULL, GfxTexture *target = NULL);
		void tracker_drawHistory( Tracker &tracker, int screenWidth, int screenHeight, cBlob &blob, GfxTexture     *target);

#ifdef WITH_HISTORY
		//void drawHistory( int screenWidth, int screenHeight, cBlob &blob, GfxTexture *target);
#endif

#endif
};



#endif
