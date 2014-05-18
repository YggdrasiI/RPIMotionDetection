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
	for (int i = 0; i < blobs.size(); i++){
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
	for (int i = 0; i < blobsTmp.size(); i++) {
		cBlob &blobi = blobsTmp[i];
		new_hand = true;
		for (int j = 0; j < blobs_previous.size(); j++) {
			if (blobs_previous[j].tracked) continue;

			d1=blobsTmp[i].location.x - blobs_previous[j].location.x;
			d2=blobsTmp[i].location.y - blobs_previous[j].location.y;
			if ( (d1*d1 + d2*d2) < max_radius_2) {
				blobs_previous[j].tracked = true;
				blobsTmp[i].event = BLOB_MOVE;
				blobsTmp[i].origin.x = history ? blobs_previous[j].origin.x : blobs_previous[j].location.x;
				blobsTmp[i].origin.y = history ? blobs_previous[j].origin.y : blobs_previous[j].location.y;

				blobsTmp[i].handid = blobs_previous[j].handid;
				blobsTmp[i].duration = blobs_previous[j].duration;
				blobsTmp[i].missing_duration = 0;
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
			blobsTmp[i].handid = next_handid;
			last_handid = next_handid;

			blobsTmp[i].event = BLOB_DOWN;
			blobsTmp[i].duration = 1;
			blobsTmp[i].missing_duration = 0;
			//blobsTmp[i].cursor = NULL;
		}
	}

	// add any blobs from the previous frame that weren't tracked as having been removed
	for (int i = 0; i < blobs_previous.size(); i++) {
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
