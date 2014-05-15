/*
 * Modification of Tracker from KinectGrid project.
 */

#ifndef TRACKER2_H
#define TRACKER2_H

#include "Tracker.h"


class Tracker2: public Tracker {

	public:
		Tracker2();
		~Tracker2();

		void trackBlobs(
				Blobtree *frameblobs,
				bool history ) ;
};


#endif
