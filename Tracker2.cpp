/*
 * Implementation of abstract Tracker class.
 */
#include <unistd.h>

#include "Tracker2.h"

Tracker2::Tracker2()
{
}

Tracker2::~Tracker2()
{
}

void Tracker2::trackBlobs(
		Blobtree * frameblobs,
		bool history )
{

	int max_radius_2 = m_max_radius * m_max_radius;
	int x, y, min_x, min_y, max_x, max_y;

	cBlob temp;
	bool new_hand(true);

	// clear the blobs from two frames ago
	blobs_previous.clear();

	// before we populate the blobs vector with the current frame, we need to store the live blobs in blobs_previous
	for (unsigned int i = 0; i < blobs.size(); i++){
		if (blobs[i].event != BLOB_UP){
			blobs_previous.push_back(blobs[i]);
	
			blobs_previous[i].duration++;
			// init previous blobs as untracked blobs
			blobs_previous[i].tracked = false;
		}
	}

	// populate the blobs vector with the current frame
	blobsTmp.clear();

	BlobtreeRect *roi;
	Node *curNode = blobtree_first(frameblobs);
	while( curNode != NULL ){
		roi = &(((Blob*)(curNode->data))->roi);
		x     = roi->x + roi->width/2;
		y     = roi->y + roi->height/2;

		min_x = roi->x; 
		min_y = roi->y;
		max_x = roi->x + roi->width;
		max_y =	roi->y + roi->height;

		temp.location.x = temp.origin.x = x;
		temp.location.y = temp.origin.y = y;


		temp.min.x = min_x; temp.min.y = min_y;
		temp.max.x = max_x; temp.max.y = max_y;

		blobsTmp.push_back(temp);
		curNode = blobtree_next(frameblobs);
	}

	float d1,d2;
	// main tracking loop -- O(n^2) -- simply looks for a blob in the previous frame within a specified radius
	for (unsigned int i = 0; i < blobsTmp.size(); i++) {
		cBlob &currentBlob = blobsTmp[i];
		new_hand = true;
		for (unsigned int j = 0; j < blobs_previous.size(); j++) {
			cBlob &previousBlob = blobs_previous[j];
			if (previousBlob.tracked) continue;

			d1=currentBlob.location.x - previousBlob.location.x;
			d2=currentBlob.location.y - previousBlob.location.y;
			if ( (d1*d1 + d2*d2) < max_radius_2) {
				previousBlob.tracked = true;
				currentBlob.event = BLOB_MOVE;
				if( history ){
					currentBlob.origin.x = previousBlob.origin.x;
					currentBlob.origin.y = previousBlob.origin.y;
#ifdef WITH_HISTORY
					/* Grab history stack pointer and
					 * add the previous Blob to the history.
					 */
					currentBlob.transfer_history(previousBlob);
					currentBlob.update_history(previousBlob);
#endif
				}else{
					currentBlob.origin.x = previousBlob.location.x;
					currentBlob.origin.y = previousBlob.location.y;
				}

				currentBlob.handid = previousBlob.handid;
				currentBlob.duration = previousBlob.duration;
				currentBlob.missing_duration = 0;
				new_hand = false;
				break;
			}
		}
		/* assing free handid if new blob */
		if( new_hand){

			//search next free id.
			int next_handid = (last_handid+1) % MAXHANDS;
			while( handids[next_handid]==true && next_handid!=last_handid ){
				next_handid = (next_handid+1) % MAXHANDS;
			} //if array full -> next_handid = last_handid

			if( next_handid == last_handid ){
				printf("To many blobs\n");
			}

			handids[next_handid] = true;
			currentBlob.handid = next_handid;
			last_handid = next_handid;

			currentBlob.event = BLOB_DOWN;
			currentBlob.duration = 1;
			currentBlob.missing_duration = 0;
			//currentBlob.cursor = NULL;
		}
	}

	// add any blobs from the previous frame that weren't tracked as having been removed
	for (unsigned int i = 0; i < blobs_previous.size(); i++) {
		cBlob &b = blobs_previous[i];
		if (!b.tracked) {
			if( b.missing_duration < m_max_missing_duration ){
				b.missing_duration++;
				b.event = BLOB_PENDING;
			}else{
				b.event = BLOB_UP;
				//free handid
				handids[b.handid] = false;
			}
			blobsTmp.push_back(b);
		}
	}

	//Now wait until no other thread works with blob vector
	//and swap with blobTmp
	while( m_swap_mutex ){
		usleep(1000);
	}
	m_swap_mutex = 1;
	blobs.swap( blobsTmp );
	m_swap_mutex = 0;
}

