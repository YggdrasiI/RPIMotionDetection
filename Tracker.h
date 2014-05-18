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
#endif

#include "Blob.h"

#include "threshtree.h"
#include "depthtree.h"


#define MAXHANDS 40


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

		//Maximal difference between blob midpoints between different frames.
		void setMaxRadius(int m);

		virtual void trackBlobs(
				Blobtree *frameblobs,
				bool history ) = 0;

#ifdef WITH_OCV
		/* Helper function to draw blobs for debugging */
		void drawBlobs(cv::Mat &out);
#endif
#ifdef WITH_OPENGL
		/* Helper function to draw blobs for debugging */
		void drawBlobsGL(int screenWidth, int screenHeight);
#endif
};



#endif
