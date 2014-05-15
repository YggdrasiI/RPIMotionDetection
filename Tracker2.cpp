/*
 * Modification of Tracker.cpp which use an other
 * blob detection lib.
 */

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
	double max_radius_2 = m_max_radius * m_max_radius;
	double x, y, min_x, min_y, max_x, max_y;

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
	blobs.clear();

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

		blobs.push_back(temp);
		curNode = blobtree_next(frameblobs);
	}

	float d1,d2;
	// main tracking loop -- O(n^2) -- simply looks for a blob in the previous frame within a specified radius
	for (int i = 0; i < blobs.size(); i++) {
		cBlob &blobi = blobs[i];
		new_hand = true;
		for (int j = 0; j < blobs_previous.size(); j++) {
			if (blobs_previous[j].tracked) continue;

			d1=blobs[i].location.x - blobs_previous[j].location.x;
			d2=blobs[i].location.y - blobs_previous[j].location.y;
			if ( (d1*d1 + d2*d2) < max_radius_2) {
				blobs_previous[j].tracked = true;
				blobs[i].event = BLOB_MOVE;
				blobs[i].origin.x = history ? blobs_previous[j].origin.x : blobs_previous[j].location.x;
				blobs[i].origin.y = history ? blobs_previous[j].origin.y : blobs_previous[j].location.y;
				blobs[i].origin.z = history ? blobs_previous[j].origin.z : blobs_previous[j].location.z;

				blobs[i].handid = blobs_previous[j].handid;
				blobs[i].duration = blobs_previous[j].duration;
				blobs[i].missing_duration = 0;
				//blobs[i].cursor = blobs_previous[j].cursor;
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
			blobs[i].handid = next_handid;
			last_handid = next_handid;

			blobs[i].event = BLOB_DOWN;
			blobs[i].duration = 1;
			blobs[i].missing_duration = 0;
			//blobs[i].cursor = NULL;
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
			blobs.push_back(b);
		}
	}

}

