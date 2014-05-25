/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */

#ifndef TRACKER_H
#define TRACKER_H

#include <stdlib.h>
#include <vector>

#ifdef WITH_OCV
#include <cv.h>
#include <cxcore.h>
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
	ALL_ACTIVE,
	NUM_TRACK_FILTER
};

class Tracker {
	protected:

		int m_max_radius;
		int m_max_missing_duration;

		std::vector<cBlob> blobs, blobs_previous, blobsTmp /*for swapping */;

		// storage of used handids
		bool handids[MAXHANDS];
		int last_handid;

		int m_swap_mutex;

	public:
		Tracker();
		virtual ~Tracker() = 0;
		std::vector<cBlob>& getBlobs();
		void getFilteredBlobs(Trackfilter filter, std::vector<cBlob> &output);

		virtual void trackBlobs(
				Blobtree *frameblobs,
				bool history ) = 0;

#ifdef WITH_OCV
		/* Helper function to draw blobs for debugging */
		void drawBlobs(cv::Mat &out);
#endif
#ifdef WITH_OPENGL
		/* Helper function to draw blobs for debugging */
		void drawBlobsGL(int screenWidth, int screenHeight, std::vector<cBlob> *toDraw = NULL, GfxTexture *target = NULL );
#endif
};



#endif
