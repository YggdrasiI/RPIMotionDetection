#ifndef BLOB_H
#define BLOB_H

#define WITH_HISTORY
#define MAX_HISTORY_LEN 100


#ifdef WITH_HISTORY
#include <deque>
#include <cassert>
#endif
 

// event types
enum { BLOB_NULL, // ?
	BLOB_DOWN, // New blob detected
	BLOB_MOVE, // Blob moved
	BLOB_PENDING, // Blob missing. Waiting N frames to recapture blob.
	BLOB_UP,	// blob was missed for N frames and is now marked for removing.
	NUM_BLOB_TYPES
};

/* Use int to omit floating point operations */
struct point {
 	int  x, y;
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

#ifdef WITH_HISTORY
		std::deque<cBlob> *history;
		void transfer_history(cBlob &previousBlob){
			assert( history == NULL );
			history = previousBlob.history;
			previousBlob.history = NULL;
		}

		/*Note: Only add (old) blob b to history container
		 * with the property b.history==NULL;
		 * Otherwise, the destruction of b will delete the
		 * container.
		 */
		void update_history(cBlob &b){
			if( history == NULL ){
				history = new std::deque<cBlob>(10); 
			}else if( history->size() >= MAX_HISTORY_LEN ){
				history->pop_back();
			}
			history->push_front(b);
		}
#endif

		cBlob()
#ifdef WITH_HISTORY
			:history(NULL)
#endif
		{
		};
		~cBlob()
		{
#ifdef WITH_HISTORY
			delete history;
#endif
		};

};

#endif
