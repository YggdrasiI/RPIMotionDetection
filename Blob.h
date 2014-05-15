#ifndef BLOB_H
#define BLOB_H

//#include "TuioCursor.h"
//#include "TuioCursor25D.h"

// event types
enum { BLOB_NULL, // ?
	BLOB_DOWN, // New blob detected
	BLOB_MOVE, // Blob moved
	BLOB_PENDING, // Blob missing. Waiting N frames to recapture blob.
	BLOB_UP,	// blob was missed for N frames and is now marked for removing.
	NUM_BLOB_TYPES
};

struct point {
	float x, y;//avoid double for ARM?
	float z; //depth
};

class cBlob {
  private:

  protected:

  public:
	point location, origin;	// current location and origin for defining a drag vector
	point min, max;		// to define our axis-aligned bounding box
	int event;		// event type: one of BLOB_NULL, BLOB_DOWN, BLOB_MOVE, BLOB_UP
	bool tracked;		// a flag to indicate this blob has been processed
	int handid; // associate id for tuio processing
	int duration; //blob exists for duration frames.
	int missing_duration; //blob is missed for missing_duration frames.
};

#endif
