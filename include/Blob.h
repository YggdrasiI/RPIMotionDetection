#ifndef BLOB_H
#define BLOB_H

#define WITH_HISTORY
#define MAX_HISTORY_LEN 100


#ifdef WITH_HISTORY
#include <deque>
#include <cassert>
#include <memory>
#endif
 

// event types
enum { BLOB_NULL = 0, // ?
	BLOB_DOWN = 1, // New blob detected
	BLOB_MOVE = 2, // Blob moved
	BLOB_PENDING = 4, // Blob missing. Waiting N frames to recapture blob.
	BLOB_UP = 8,	// blob was missed for N frames and is now marked for removing.
	//NUM_BLOB_TYPES
};

/* Use int to omit floating point operations */
struct point {
 	int  x, y;
};

class cBlob {
	private:
	public:
		/* Test if some copy occurs */
		/*
		cBlob(const cBlob& src){
			printf("Copy Blob %i\n",src.handid);
			memcpy((void*)this,&src,sizeof(cBlob));
			history = NULL;
		};*/
		//cBlob& operator = (const cBlob&);

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
		//std::deque<cBlob> *history;
		std::shared_ptr< std::deque<cBlob> > history;//;
		void transfer_history(cBlob &previousBlob){
			//assert( history == NULL );
			history = previousBlob.history;
			//previousBlob.history = NULL;
		}

		/*Note: Only add (old) blob b to history container
		 * with the property b.history==NULL;
		 * Otherwise, the destruction of b will delete the
		 * container.
		 */
		void update_history(cBlob &b){
			if( history.get() == nullptr ){
				//history = std::shared_ptr<std::deque<cBlob>>(new std::deque<cBlob>(10)); 
				history = std::shared_ptr<std::deque<cBlob>>(new std::deque<cBlob>(0)); 
			}else if( history.get()->size() >= MAX_HISTORY_LEN ){
				history.get()->pop_back();
			}
			history.get()->push_front(b);//copy b, thus increases the occuring of the pointer.
		}
#endif

		cBlob()
#ifdef WITH_HISTORY
			:history(nullptr)
#endif
		{
		};
		~cBlob()
		{
#ifdef WITH_HISTORY
			//delete history;
#endif
		};

};

#endif
