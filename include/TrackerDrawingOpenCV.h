/* Helper functions to draw tracker objects
 * on OpenCV target.
 */
#ifndef TRACKER_DRAWING_OPENCV_H
#define TRACKER_DRAWING_OPENCV_H

#include <stdlib.h>
#include <vector>
#include "Blob.h"

#ifdef WITH_OCV
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv2/opencv.hpp>
/* Helper function to draw blobs for debugging */
void tracker_drawBlobs(Tracker &tracker, cv::Mat &out, bool drawHistoryLines, std::vector<cBlob> *toDraw = NULL );
#endif

#endif
