#include <pthread.h>

#include "RaspiVid.h"
#include "RaspiImv.h"

#include "depthtree.h"
#include "Tracker2.h"
#include "Graphics.h"

#include "FontManager.h"

#include "Pong.h"

static DepthtreeWorkspace *dworkspace = NULL;
static Blobtree *frameblobs = NULL;
Tracker2 tracker;
FontManager fontManager;
//Pong pong(0.06, 800.0/600.0);
Pong pong(0.06, 16.0/9);
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
					imv_eval_norm2(&motion_data);

					//1.5 (optional) OpenGl Output
					if( true ){
						/* Problem: This texture update is outside
						 * of the gl context thread and will be
						 * ignored. Solution?!"
						 * */
						//imvTexture.SetPixels(motion_data.imv_norm);
					}

					//2. Blob detection
					BlobtreeRect input_roi = {0,0, motion_data.width, motion_data.height -0 };//shrink height because lowest rows contains noise.
					depthtree_find_blobs(frameblobs, motion_data.imv_norm,
							motion_data.width, motion_data.height,
							input_roi, depth_map, dworkspace);

					//3. Tracker, track without history generation
					tracker.trackBlobs( frameblobs, false );
					
					//4. Opengl Output

					// Debug: Replace imv_norm with ids, roi has to start in (0,0) and with full width.
					//eval_ids(dworkspace, motion_data.imv_norm, input_roi.width* input_roi.height);


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
	if( (data->roi.width * data->roi.height) > (120*68/8) )
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

	blobtree_set_filter(frameblobs, F_AREA_MIN, 50 );
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

	/* Parse input arguments (similar to RaspiVid, but less smart )*/
   int valid = 1;
   int i;

   for (i = 1; i < argc-1 && valid; ++i)
	 {
		 int command_id, num_parameters;

		 if (!argv[i])
			 continue;

		 // Player setup for pong
		 if ( strcmp(argv[i],"--player") == 0 || strcmp(argv[i],"-pl") == 0 ){
			 if( strcmp( argv[i+1], "left" ) == 0 ){
				 pong.setActivePlayers(true, false);
				 continue;
			 }
			 if( strcmp( argv[i+1], "right" ) == 0 ){
				 pong.setActivePlayers(false, true);
				 continue;
			 }
			 if( strcmp( argv[i+1], "both" ) == 0 ){
				 pong.setActivePlayers(true, true);
				 continue;
			 }
		 }
	 }

	//Setup tracker
	tracker.setMaxRadius(10);
	tracker.setOldestDurationFilter(4);

	//Setup font manager for GUI textes
	fontManager.add_font("./shader/fontrendering/fonts/custom.ttf", 50 );
	fontManager.add_font("./shader/fontrendering/fonts/ObelixPro.ttf", 70 );

	texture_font_t *font1, *font2;
	font1 = fontManager.getFonts()->at(0);
	font2 = fontManager.getFonts()->at(1);

  vec2 pen = {-400,150};
  vec4 color = {.2,0.2,0.2,1};
	vec4 transColor = {1,0.3,0.3,0.6};
	fontManager.add_text( font1, L"freetypeGlesRpi", &color, &pen );
	
	pen.x = -390;
	pen.y = 140;
	fontManager.add_text( font1, L"freetypeGlesRpi", &transColor, &pen );

	pen.x = 600;
	pen.y = 400;
	fontManager.add_text( font2, L"Special chars: ηαβ∅", &color, &pen );

	//start raspivid application.
	raspivid(argc, argv);


	depthtree_destroy_workspace( &dworkspace );
	blobtree_destroy(&frameblobs);
}


