#include <pthread.h>

#include "RaspiVid.h"
#include "RaspiImv.h"

#include "depthtree.h"
#include "Tracker2.h"
#include "Graphics.h"

static DepthtreeWorkspace *dworkspace = NULL;
static Blobtree *frameblobs = NULL;
static Tracker2 tracker;
unsigned char depth_map[256];

pthread_t blob_tid;
void* blob_detection(void *argn){

      while (1){
				if( motion_data.available ){
					//swap pointers if possible
					while(motion_data.mutex){}
					motion_data.mutex = 1;
					char *tmp = motion_data.imv_array_in;
					motion_data.imv_array_in = motion_data.imv_array_buffer;
					motion_data.imv_array_buffer = tmp;
					motion_data.available = 0;
					motion_data.mutex = 0;

					//1. Convert imv vector to 1-norm.
					//Note: Some uness. operations if gridwidth>1.
					imv_eval_norm(&motion_data);

					//1.5 (optional) OpenGl Output
					if( true ){
						imvTexture.SetPixels(motion_data.imv_norm);
					}

					printf("Eval blobs...\n");
					//2. Blob detection
					BlobtreeRect input_roi = {0,0, motion_data.width, motion_data.height -4 };//shrink height because lowest rows contains noise.
					depthtree_find_blobs(frameblobs, motion_data.imv_norm,
							motion_data.width, motion_data.height,
							input_roi, depth_map, dworkspace);

					//3. Tracker
					tracker.trackBlobs( frameblobs, true );
					
					//4. Opengl Output

				}else{
					printf("No data\n");
				}
         vcos_sleep(100);
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

	blobtree_set_filter(frameblobs, F_AREA_MIN, 100 );
	blobtree_set_filter(frameblobs, F_AREA_MAX, 2000 );
	blobtree_set_extra_filter(frameblobs, hand_filter);
	/* Only show leafs with above filtering effects */
	blobtree_set_filter(frameblobs, F_ONLY_LEAFS, 1);

	//init depth map, [0,1,1,1,1,2,3,3,4,4,5, 100,100,…]
	for( int i=0; i<256; i++){
		switch(i){
			case 0:
				depth_map[i] = 0;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				depth_map[i] = 1;
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				depth_map[i] = (i/2)*2;
				break;
			default:
				depth_map[i] = 100;
		}
	}
	//Create thread for blob detection.
	int err = pthread_create(&blob_tid, NULL, &blob_detection, NULL);
	if (err != 0){
		printf("\nFailed to create blob thread :[%s]", strerror(err));
		return -1;
	}

	//Add arguments to force opengl output of 
	//raspivid

	//start raspivid application.
	raspivid(argc, argv);


	depthtree_destroy_workspace( &dworkspace );
	blobtree_destroy(&frameblobs);
}
