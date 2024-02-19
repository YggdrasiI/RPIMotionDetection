#include <pthread.h>
#include <vector>

#include "RaspiVid.h"
#include "RaspiImv.h"

#include "depthtree.h"
#include "Tracker2.h"
#include "Graphics.h"

#include "FontManager.h"
#include "Gestures.h"


static DepthtreeWorkspace *dworkspace = NULL;
static Blobtree *frameblobs = NULL;
Tracker2 tracker;
unsigned char depth_map[256];
pthread_t blob_tid;

FontManager fontManager;
static std::vector<cBlob> blobCache; //for gestures

static GestureStore gestureStore;
std::vector<Gesture*> gestures = std::vector<Gesture*>();
static const vec2 gest_pen_headline = {0,160};
static vec2 gest_pen = {0,0};
static vec4 gest_color = {.37254, .69411, .17647, 1.0};
static int gest_pos = 0;

//Setup of font manager for GUI textes. Called after OpenGL initialisation.
void setup_fonts(FontManager *fontManager){

	//fontManager->add_font("./shader/fontrendering/fonts/Vera.ttf", 70 );
	fontManager->add_font("./shader/fontrendering/fonts/amiri-regular.ttf", 70 );
	//fontManager->add_font("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 70 );

	texture_font_t *font1;
	font1 = fontManager->getFonts()->at(0);
	vec2 pen = {gest_pen_headline.x, gest_pen_headline.y};
	fontManager->add_text( font1, L"Last Gestures: α", &gest_color, &pen );
}

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
					if( false ){
						/* Problem: This texture update is outside
						 * of the gl context thread and will be
						 * ignored. Solution?!"
						 * */
						imvTexture.setPixels(motion_data.imv_norm);
					}

					//2. Blob detection
					BlobtreeRect input_roi = {0,0, motion_data.width, motion_data.height - 0 }; // Noise in lowest row removed by raspivid update. Shrinking of height not ness anymore 
					depthtree_find_blobs(frameblobs, motion_data.imv_norm,
							motion_data.width, motion_data.height,
							input_roi, depth_map, dworkspace);

					//3. Tracker
					tracker.trackBlobs( frameblobs, true );

					//3.5 Gestures
					blobCache.clear();
					tracker.getFilteredBlobs(TRACK_UP|LIMIT_ON_N_OLDEST, blobCache);
					//tracker.getFilteredBlobs(TRACK_UP, blobCache);

					// Remove old detections from drawing
					while( gestures.size() > 8 ){
						delete gestures.front();
						gestures.erase(gestures.begin());
					}

					std::vector<cBlob>::iterator it = blobCache.begin();
					const std::vector<cBlob>::iterator itEnd = blobCache.end();
					int tmpI(0);
					bool gestureHandledTwice(false);
					for( ; it != itEnd ; ++it ){
						// Skip short/flickering movements
						if( (*it).duration < 20 ) continue; 

						size_t id = (size_t) (*it).id;//handid;
						printf("Gest (%u,%i)\t", id, (*it).event );
						for( const auto& g: gestures){
							if( id == g->getGestureId() ){
								printf("WRN, handle same blob chain twice! id=%u el=%i\n", id, tmpI);
								gestureHandledTwice = true;
								continue;
							}
						}

						tmpI++;
						//if( gestureHandledTwice ) continue;
						printf("===\n");

						// Convert list of coordinates into spline approximation
						Gesture *gest = new Gesture( (*it), 0, 0 );

						// This objects stores some metadata/results.
						GesturePatternCompareResult res;
						gestureStore.compateWithPatterns(gest, res);
						if( res.minGest != NULL && res.minDist < 0.08 ){
							printf("Gesture similar to %s\n", res.minGest->getGestureName() );
							gestures.emplace_back(gest);

							// Draw gesture name
							gest_pen.x = 0;
							if( ++gest_pos > 3 ){
								gest_pos = 0;	
								fontManager.clear_text();
								vec2 pen = {gest_pen_headline.x, gest_pen_headline.y};
								fontManager.add_text( fontManager.getFonts()->at(0),
										L"Last Gestures:", &gest_color, &pen );
							}
							gest_pen.y = gest_pen_headline.y - 80*(1+gest_pos);
							char tmpText[100];
							snprintf(tmpText, 100, "%s   %i", res.minGest->getGestureName(), gest->getGestureId());
							fontManager.add_text( fontManager.getFonts()->at(0),
									tmpText /*res.minGest->getGestureName()*/,
									&gest_color, &gest_pen );

						}else{
							//delete gest;
							gest->setGestureName("Unknown");
							gestures.emplace_back(gest);
							gest_pos++;
							gest_pen.x = 0;
							gest_pen.y = gest_pen_headline.y - 80*(1+gest_pos);
							fontManager.add_text( fontManager.getFonts()->at(0),
									gest->getGestureName(), &gest_color, &gest_pen );
						}
					}

					//4. Opengl Output
					// see gl_scenes/motion.c

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
	frameblobs = blobtree_create();
	blobtree_set_grid(frameblobs, 1, 1);

	blobtree_set_filter(frameblobs, F_AREA_MIN, 75 );
	blobtree_set_filter(frameblobs, F_AREA_MAX, 1000 );
	blobtree_set_extra_filter(frameblobs, hand_filter);
	/* Only show leafs with above filtering effects */
	blobtree_set_filter(frameblobs, F_ONLY_LEAFS, 1);

	//init depth map
	//Mapping low values to zero reduces the noise of 
	//the motion vector values.
	for( int i=0; i<256; i++){
		depth_map[i] = (i<7?0:i/4+1);
	}
	//Create thread for blob detection.
	int err = pthread_create(&blob_tid, NULL, &blob_detection, NULL);
	if (err != 0){
		printf("\nFailed to create blob thread :[%s]", strerror(err));
		return -1;
	}

	/* Setup tracker */
	tracker.setMaxRadius(15);
	tracker.setMaxMissingDuration(10);
	//filter out blobs with live time < M frames
	tracker.setMinimalDurationFilter(5);
	//reduce output to N oldest blobs
	tracker.setOldestDurationFilter(2);

	//Setup font manager
	fontManager.setInitFunc(setup_fonts);

	addGestureTestPattern(gestureStore);

	//start raspivid application.
	raspivid(argc, argv);


	depthtree_destroy_workspace( &dworkspace );
	blobtree_destroy(&frameblobs);
}


