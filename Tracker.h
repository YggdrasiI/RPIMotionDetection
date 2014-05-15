/* Abstract Tracker class. 
 * Implementations: TrackerCvBlobsLib, Tracker2.
 */

#ifndef TRACKER_H
#define TRACKER_H

#include <stdlib.h>
#include <cv.h>
#include <cxcore.h>
#include <opencv2/opencv.hpp>
#include <vector>

#include "Blob.h"

#include "threshtree.h"
#include "depthtree.h"


#define MAXHANDS 10


class Tracker {
	protected:

		int m_max_radius;
		int m_max_missing_duration;

		std::vector<cBlob> blobs, blobs_previous;

		// storage of used handids
		bool handids[MAXHANDS];
		int last_handid;

	public:
		Tracker();
		virtual ~Tracker() = 0;
		std::vector<cBlob>& getBlobs();

		virtual void trackBlobs(
				Blobtree *frameblobs,
				bool history ) = 0;

		void drawBlobs(cv::Mat &out);
};



#endif
