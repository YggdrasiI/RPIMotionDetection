#include <pthread.h>

#include "RaspiVid.h"
#include "RaspiImv.h"

#include "depthtree.h"
#include "Tracker2.h"
#include "Graphics.h"

static DepthtreeWorkspace *dworkspace = NULL;
static Blobtree *frameblobs = NULL;
Tracker2 tracker;
unsigned char depth_map[256];
pthread_t blob_tid;

static void eval_ids(DepthtreeWorkspace *dworkspace, unsigned char *out, int len ){
	unsigned int *ids, *cm, *cs;
	ids = dworkspace->ids;
	cm = dworkspace->comp_same;
	cs = dworkspace->comp_size;
	while(--len){
		if( *(cs + *ids) > 8 ){
			*out = *(cm + *ids);
		}else{
			*out = 1;
		}
		++out;++ids;
	}
}

void* blob_detection(void *argn){

      while (1){
				if( motion_data.available ){

					//swap pointers if possible
					while(motion_data.mutex){
						usleep(1000);
					}
					motion_data.mutex = 1;
					char *tmp = motion_data.imv_array_in;
					motion_data.imv_array_in = motion_data.imv_array_buffer;
					motion_data.imv_array_buffer = tmp;
					motion_data.available = 0;
					motion_data.mutex = 0;

					//1. Convert imv vector to norm.
					//Note: Some uness. operations if gridwidth>1.
					//imv_eval_norm(&motion_data);
					imv_eval_norm2(&motion_data);

					//1.5 (optional) OpenGl Output
					if( true ){
						/* Problem: This texture update is outside
						 * of the gl context thread and will be
						 * ignored. Solution?!"
						 * */
						//imvTexture.setPixels(motion_data.imv_norm);
					}

					//2. Blob detection
					BlobtreeRect input_roi = {0,0, motion_data.width, motion_data.height -0 };//shrink height because lowest rows contains noise.
					depthtree_find_blobs(frameblobs, motion_data.imv_norm,
							motion_data.width, motion_data.height,
							input_roi, depth_map, dworkspace);

					//3. Tracker
					tracker.trackBlobs( frameblobs, true );
					
					//4. Opengl Output
					// see gl_scenes/motion.c

					// Debug: Replace imv_norm with ids, roi has to start in (0,0) and with full width.
					eval_ids(dworkspace, motion_data.imv_norm, input_roi.width* input_roi.height);


				}else{
					//printf("No new imv data\n");
					//vcos_sleep(100);
					usleep(100000);
				}
			}
}


static int hand_filter(Node *n){

	const Blob* const data = (Blob*) n->data;

	/* Remove Blobs which has almost the same size as their (first) child node */
	if( n->child != NULL ){
		const Blob* const cdata = (Blob*) n->child->data;
		if( ((float)(cdata->area)) > (data->area) * 0.99f ){
			return 1;
		}
	}

	//remove big areas
	if( 10 *( data->roi.width * data->roi.height )>120*68 )
		return 1;

	// remove areas which are very unsquared
	if( data->area > 25
			&& (	data->roi.width > 3*data->roi.height 
				|| data->roi.height > 3*data->roi.width ) )
	{
		return 1;
	}
	
	// remove areas with sparse content
	if( data->roi.width + data->roi.height > 4*data->area ){
		return 1;
	}

	return 0;
}

int main(int argc, const char **argv){

	// Create objects for blob detection and tracker
	// 120x68 for 1080p encoding resolution.
	depthtree_create_workspace( 120, 68, &dworkspace );
	if( dworkspace == NULL ){
		printf("Unable to create workspace.\n");
		return -1;
	}
	blobtree_create(&frameblobs);
	blobtree_set_grid(frameblobs, 1, 1);

	blobtree_set_filter(frameblobs, F_AREA_MIN, 75 );
	blobtree_set_filter(frameblobs, F_AREA_MAX, 1000 );
	blobtree_set_extra_filter(frameblobs, hand_filter);
	/* Only show leafs with above filtering effects */
	blobtree_set_filter(frameblobs, F_ONLY_LEAFS, 1);

	//init depth map
	for( int i=0; i<256; i++){
		depth_map[i] = (i<10?0:i/4+1);
	}
	//Create thread for blob detection.
	int err = pthread_create(&blob_tid, NULL, &blob_detection, NULL);
	if (err != 0){
		printf("\nFailed to create blob thread :[%s]", strerror(err));
		return -1;
	}

	/* Setup tracker */
	tracker.setMaxRadius(15);
	//filter out blobs with live time < M frames
	tracker.setMinimalDurationFilter(5);
	//reduce output to N oldest blobs
	tracker.setOldestDurationFilter(2);

	//start raspivid application.
	raspivid(argc, argv);


	depthtree_destroy_workspace( &dworkspace );
	blobtree_destroy(&frameblobs);
}


