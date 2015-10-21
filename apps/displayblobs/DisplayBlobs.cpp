#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <opencv2/opencv.hpp>

//get ENV variables from BlobDetection lib
#include "settings.h"

#include "threshtree.h"
#include "depthtree.h"
#include "Tracker2.h"
#include "TrackerDrawingOpenCV.h"

#ifdef WITH_GSL
#include "../../Gestures.h"
#endif

#include "Fps.h"

using namespace cv;

static std::vector<cBlob> blobCache;
// Header for drawing function. Definition in libs/tracker/DrawingOpenCV.cpp
//void tracker_drawBlobs(Tracker &tracker, cv::Mat &out, bool drawHistoryLines, std::vector<cBlob> *toDraw );

/* Define own filters. Node will pass filter
 * if 0 will be returned. For more info
 * see comment in blob.h for
 * typedef int (FilterNodeHandler)(Node *node)
 * */

/* Let only nodes with depth level = X passes. */
static int my_filter(Node *n){
	const Blob* const data = (Blob*) n->data;
	if( data->depth_level > 3 ) return 3;
	if( data->depth_level == 3 ) return 0;
	return 1;
}

/* Remove Blobs which has almost the same size as their (first) child node */
static int my_filter2(Node *n){
	if( n->child != NULL ){
		const Blob* const data = (Blob*) n->data;
		const Blob* const cdata = (Blob*) n->child->data;
		if( ((float)(cdata->area)) > (data->area) * 0.99f ){
			return 1;
		}
	}
	return 0;
}

static int hand_filter(Node *n){
	if( my_filter2(n) ) return 1;

	const Blob* const data = (Blob*) n->data;

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

static const char* window_name = "Blob Map";
static const char* window_options = window_name; //"Options";
static std::string image_filename;

static Mat input_image;
static BlobtreeRect input_roi; // Region of interest of input image
static bool redraw_pending = false;
static bool display_areas = true;
static bool display_tracker = true;
static bool display_filtered_areas = true;
static bool display_bounding_boxes = true;
static int algorithm = 1;
static int gridwidth = 1;
static int gridheight = gridwidth;
static int of_area_min = 40;
static int of_area_max = 40;
static int of_tree_depth_min = 1;
static int of_tree_depth_max = 100;
static int of_area_depth_min = 0;
static int of_area_depth_max = 255;
static bool of_only_leafs = true;
static bool of_use_own_filter = false;
static int output_scalefactor = 1;
static bool invert_depth_map = false;


// Thresh 
static int thresh = 25;//60;
// Depth map (for algorithm 1)
unsigned char depth_map[256];

static ThreshtreeWorkspace *tworkspace = NULL;
static DepthtreeWorkspace *dworkspace = NULL;
static Blobtree *frameblobs = NULL;

static Tracker2 tracker;

#ifdef WITH_GSL
static GestureStore gestureStore;
static std::vector<Gesture*> gestures = std::vector<Gesture*>();
#endif

/* Init id array:
 * If this flag is set the ids array in the workspace
 * will be filled manually with 0. This allows a better
 * visual representation of the ids. The array is not
 * reset automaticaly due performance increase.
 * */
static bool reset_ids = false;
static const unsigned int IDINITVAL = 0;



void update_filter( ){

	//filter out small blobs
	blobtree_set_filter(frameblobs, F_AREA_MIN,
			//(of_area_min * of_area_min)/16 );
			of_area_min * 10 );

	//filter out big blobs
	blobtree_set_filter(frameblobs, F_AREA_MAX,
			//(of_area_max * of_area_max)/16 );
			of_area_max * 500 );

	//filter out top level frameblobs for whole image
	blobtree_set_filter(frameblobs, F_TREE_DEPTH_MIN,
			of_tree_depth_min );

	//filter out blobs included into bigger blobs
	blobtree_set_filter(frameblobs, F_TREE_DEPTH_MAX,
			of_tree_depth_max );

	/* Note: Changeing depth_map could be the more efficient approach to
	 * get a similar filtering effect.*/
	//filter out blobs with lower depth values.
	blobtree_set_filter(frameblobs, F_AREA_DEPTH_MIN,
			of_area_depth_min );

	//filter out blobs with higher depth values.
	blobtree_set_filter(frameblobs, F_AREA_DEPTH_MAX,
			of_area_depth_max );

	/* Add own filter over function pointer */
	if( of_use_own_filter ){
		blobtree_set_extra_filter(frameblobs, hand_filter);
	}else{
		blobtree_set_extra_filter(frameblobs, NULL);
	}

	/* Only show leafs with above filtering effects */
	blobtree_set_filter(frameblobs, F_ONLY_LEAFS, of_only_leafs?1:0 );


}

void drawGestureSpline( Gesture *gesture, cv::Mat &out, cv::Scalar line_color = cv::Scalar(255,200,200)){

		double *x = NULL , *y = NULL; 
		size_t xy_len = 0;
		gesture->evalSpline(&x,&y,&xy_len);

		if( xy_len > 0 ){
			// 1. Draw orientation triangle
			cv::Point t0(gesture->m_orientationTriangle[0].x,gesture->m_orientationTriangle[0].y);
			cv::Point t1(gesture->m_orientationTriangle[1].x,gesture->m_orientationTriangle[1].y);
			cv::Point t2(gesture->m_orientationTriangle[2].x,gesture->m_orientationTriangle[2].y);
			cv::Scalar triColor(255,50,0);
			cv::line(out,t0,t1,triColor,2);
			cv::line(out,t1,t2,triColor,2);
			cv::line(out,t2,t0,triColor,2);

			// 2. Draw spline
			cv::Point p1((int)x[0],(int)y[0]);
			cv::Point p2;
			for( size_t i=1; i<xy_len; ++i){
				p2.x = x[i]; p2.y = y[i];
				cv::line(out,p1,p2,line_color,2);
				p1 = p2;
			}
		}
		//delete x; delete y;//variables points to member variables of gesture object, now. -> no explicit delete required!
}

int detection_loop(std::string filename ){

	input_image = imread( filename, CV_LOAD_IMAGE_GRAYSCALE );
	if ( !input_image.data )
	{
		printf("No image data \n");
		return -1;
	}

	image_filename = filename;

	// Setup depth level map
	/* pixel value:   0 1 2 3 4 5 … thresh …                   … 255
	 *   depth map:   0 0 0 0 0 0 …    1     1 … 1 2 … 2 3 … 3 …
	 */
	unsigned int div=30;
	for( unsigned int i=0; i<256; i++){
#if 1
		depth_map[i] = (i<thresh?0:(i-thresh)/30+1);
#else
		depth_map[i] = 0;
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
#endif

		if( invert_depth_map ){
			depth_map[i] = 255 - depth_map[i];
		}
	}

	//====================================================
	const int W = input_image.size().width;
	const int H = input_image.size().height;
	const uchar* ptr = input_image.data;


	/* The workspaces depends on W and H
	 * Destroy old instances and reallocate memory
	 * This should be avoided in real applications.
	 * */
	//threshtree_destroy_workspace( &tworkspace );
	//depthtree_destroy_workspace( &dworkspace );
	//blobtree_destroy(&frameblobs);

	//Init workspaces
	threshtree_create_workspace( W, H, &tworkspace );
	if( tworkspace == NULL ){
		printf("Unable to create workspace.\n");
		return -1;
	}
	depthtree_create_workspace( W, H, &dworkspace );
	if( dworkspace == NULL ){
		printf("Unable to create workspace.\n");
		return -1;
	}

	blobtree_create(&frameblobs);

	if( reset_ids ){
		unsigned int* ids = (algorithm==0)?tworkspace->ids:dworkspace->ids;
		for( unsigned int* iEnd=ids+W*H; ids<iEnd;++ids){
			*ids = IDINITVAL; 
		}
	}


	// Set distance between compared pixels.	
	// Look at blobtree.h for more information.
	blobtree_set_grid(frameblobs, gridwidth,gridwidth);

	input_roi = {0,0,W, H };

	if( algorithm == 0 ){
		threshtree_find_blobs(frameblobs, ptr, W, H, input_roi, thresh, tworkspace);
	}else{
		depthtree_find_blobs(frameblobs, ptr, W, H, input_roi, depth_map, dworkspace);
	}

	//Update Tracker
	update_filter();
	//tracker.setMaxRadius(50);
	tracker.setMaxRadius( max((H+W)/10,7) );
	tracker.trackBlobs( frameblobs, true );

#ifdef WITH_GSL
	/* Gesture analyser for tracking output */
	blobCache.clear();

	//Get blobs which are marked as 'finished'.
	//Minimize computatation but increase detection delay.
	tracker.getFilteredBlobs(TRACK_UP, blobCache);
	//Alternative: Fetch all active blobs
	//tracker.getFilteredBlobs(TRACK_ALL_ACTIVE, blobCache);

	// Clear shown gesture splines of previous frame
	gestures.clear();

	std::vector<cBlob>::iterator it = blobCache.begin();
	const std::vector<cBlob>::iterator itEnd = blobCache.end();
	for( ; it != itEnd ; ++it ){
		// Skip short/flickering movements
		if( (*it).duration < 10 ) continue; 

		// Convert list of coordinates into spline approximation
		Gesture *gest = new Gesture( (*it) );
		//gest->evalSplineCoefficients();
		
		// This objects stores some metadata/results.
		GesturePatternCompareResult res;
		gestureStore.compateWithPatterns(gest, res);
		if( res.minGest != NULL && res.minDist < 0.1 ){
			printf("Gesture similar to %s\n", res.minGest->getGestureName() );
			//gestureStore.addPattern(gest);
			gestures.emplace_back(gest);
			printf("Num tmp gestures: %lu\n", gestures.size());
		}else{
			delete gest;
		}
	}
#endif

	/* Textual output of whole tree of blobs. */
	//print_tree(frameblobs->tree->root,0);

	return 0;
}


int fpsTest(std::string filename ){

	input_image = imread( filename, CV_LOAD_IMAGE_GRAYSCALE );
	if ( !input_image.data )
	{
		printf("No image data \n");
		return -1;
	}

	image_filename = filename;
	unsigned int div=30;
	for( unsigned int i=0; i<256; i++){
		depth_map[i] = (i<thresh?0:(i-thresh)/30+1); 
		if( invert_depth_map ){
			depth_map[i] = 255 - depth_map[i];
		}
	}

	//====================================================
	const unsigned int W = input_image.size().width;
	const unsigned int H = input_image.size().height;
	const uchar* ptr = input_image.data;

	blobtree_destroy(&frameblobs);

	//Init workspaces
	threshtree_create_workspace( W, H, &tworkspace );
	depthtree_create_workspace( W, H, &dworkspace );
	blobtree_create(&frameblobs);
	blobtree_set_grid(frameblobs, gridwidth,gridwidth);
	input_roi = {0,0, (int)W, (int)H };//shrink height because lowest rows contains noise.

	Fps fps;
	unsigned int N = 1;
	while( ++N<100000 ){
		//printf("N = %i\n", N);
		if( algorithm == 0 ){
			threshtree_find_blobs(frameblobs, ptr, W, H, input_roi, thresh, tworkspace);
		}else{
			depthtree_find_blobs(frameblobs, ptr, W, H, input_roi, depth_map, dworkspace);
		}

		//Update Tracker
		//update_filter();
		//tracker.trackBlobs( frameblobs, true );

		fps.next(stdout);
	}

	return 0;
}



static void redraw(){

	if( frameblobs == NULL ) return;

	redraw_pending = true;

	Mat color(input_image.size(),CV_8UC3);
	cv::cvtColor(input_image, color, CV_GRAY2BGR);
	if( !color.data ) return;

	/* Set output filters */
	update_filter();

	/* Create data for graphical output */
	//fill color image with id map of filtered list of blobs
	//1. Create mapping for filtered list
	if( display_areas ){

		unsigned int seed, id, s2, s3, s4;
		unsigned int *ids, *riv, *bif, *cm;

		if( algorithm == 1 ){
			ids = dworkspace->ids;
			cm = dworkspace->comp_same;
			riv = dworkspace->real_ids_inv;

			if( display_filtered_areas ){
				depthtree_filter_blob_ids(frameblobs,dworkspace);
			}
			bif = dworkspace->blob_id_filtered;//maps  'unfiltered id' on 'parent filtered id'

		}else{
			ids = tworkspace->ids;
			cm = tworkspace->comp_same;
			riv = tworkspace->real_ids_inv;

			if( display_filtered_areas ){
				threshtree_filter_blob_ids(frameblobs,tworkspace);
			}
			bif = tworkspace->blob_id_filtered;//maps  'unfiltered id' on 'parent filtered id'
		}

		printf("Roi: (%i %i %i %i)\n", input_roi.x, input_roi.y, input_roi.width, input_roi.height);
		fflush(stdout);

		for( unsigned int y=0, H=input_roi.height; y<H; ++y){

			if( !reset_ids){
				//restrict on grid pixels.
				if( y % frameblobs->grid.height != 0 && y!= H-1 ) continue;
			}

			for( unsigned int x=0, W=input_roi.width; x<W; ++x) {
				if( !reset_ids){
					if( x % frameblobs->grid.width != 0 && x!= W-1 ) continue;
				}

				id = ids[ (y+input_roi.y)*input_image.size().width + x + input_roi.x ];
				//id = ids[ (y)*input_image.size().width + x   ];
				if( id == IDINITVAL ) continue;

			  //	seed = *ids;
				seed = id;

				if( display_filtered_areas && bif ){
					s2 = *(bif+ seed);
				}else{

					/* This transformation just adjust the set of if- and else-branch.
					 * to avoid color flickering after the flag changes. */
					s2 = *(riv + *(cm + seed)) + 1 ;
				}

				/* To reduce color flickering for thresh changes
				 * eval variable which depends on some geometric information.
				 * Use seed(=id) to get the associated node of the tree structure. 
				 * Adding 1 compensate the dummy element at position 0.
				*/
				Blob *pixblob = (Blob*) (frameblobs->tree->root + s2 )->data;
				s3 = pixblob->area + pixblob->roi.x + pixblob->roi.y;

				cv::Vec3b col( (s3*5*5+100)%256, (s3*7*7+10)%256, (s3*29*29+1)%256 );

				if( algorithm == 1 ){
					s3 = pixblob->depth_level << 3;
					cv::Vec3b col( (s3)%256, (s3)%256, (255-s3)%256 );
				}

				color.at<Vec3b>(y + input_roi.y, x + input_roi.x ) = col;
				
				
				//++ids;
			}
		}

	}

	if( display_tracker ){
		if( display_bounding_boxes ){
			static std::vector<cBlob> blobCache;
			blobCache.clear();
			//tracker.getFilteredBlobs(TRACK_ALL_ACTIVE/*|TRACK_PENDING*/, blobCache);
			tracker.getFilteredBlobs(TRACK_ALL_ACTIVE, blobCache);
			tracker_drawBlobs( tracker, color, true, &blobCache );
		}
	}else{
		BlobtreeRect *roi;
		Node *curNode = blobtree_first(frameblobs);

		if( display_bounding_boxes ){
			unsigned int num=0;
			while( curNode != NULL ){
				num++;
				Blob *data = (Blob*)curNode->data;
				roi = &data->roi;
				int x     = roi->x + roi->width/2;
				int y     = roi->y + roi->height/2;
				/*
					 printf("Blob with id %i: x=%i y=%i w=%i h=%i area=%i, depthlevel=%i\n",data->id,
					 roi->x, roi->y,
					 roi->width, roi->height,
					 data->area,
					 data->depth_level
					 );
					 */

				cv::Rect cvRect(
						max(roi->x-1,0),
						max(roi->y-1,0),
						roi->width + (roi->x>0?2:1),
						roi->height+ (roi->y>0?2:1)
						);
				cvRect.width = min(cvRect.width,
						color.size().width - cvRect.x );
				cvRect.height = min(cvRect.height,
						color.size().height - cvRect.y );

				//const cv::Scalar col2(255.0f,255.0f- (num*29)%256,(num*5)%256);
				if( display_areas ){
					//int s3 = data->area + data->roi.x + data->roi.y;
					//const cv::Scalar col( (s3*5*5+100)%256, (s3*7*7+10)%256, (s3*29*29+1)%256 );
					cv::rectangle( color, cvRect, /*col*/ cv::Scalar(255,255, 255), 1);
				}else{
					cv::rectangle( color, cvRect, cv::Scalar(100, 100, 255), 1);
				}

				curNode = blobtree_next(frameblobs);
			}
		}

#ifdef BLOB_BARYCENTER
		curNode = blobtree_first(frameblobs);
		while( curNode != NULL ){
			Blob *data = (Blob*)curNode->data;
			cv::circle( color,
					cv::Point(data->barycenter[0], data->barycenter[1]),
					3.0, cv::Scalar(100, 100, 255), 2 );

			curNode = blobtree_next(frameblobs);
		}

#endif


	}

#ifdef WITH_GSL
	std::vector<Gesture*> gestures2 = gestureStore.getPatterns();

	std::vector<Gesture*>::iterator it = gestures2.begin();
	const std::vector<Gesture*>::iterator itEnd = gestures2.end();
	//printf("Number of stored patterns:%lu\n", itEnd-it );
	for( ; it != itEnd ; ++it ){
		//drawGestureSpline( (*it) , color);
	}
	for( auto& gest: gestures){
		drawGestureSpline(gest, color, cv::Scalar(0,200,200) );
	}
#endif

	cv::Mat out;
	cv::resize(color, out, color.size()*output_scalefactor, 0, 0, INTER_NEAREST);

	imshow( window_name, out );

	redraw_pending = false;
}


// ---- BEGIN Some callbacks for opencv gui ----

static void CB_Filter(int, void*){
	if( output_scalefactor < 1 ) output_scalefactor = 1;
	redraw();
}

static void CB_Thresh(int, void*){
	//Change of thresh require rerun of main algo.
	if(gridwidth<1) gridwidth=1;

	detection_loop(image_filename);
	redraw();
}

static void CB_Button1(int state, void* pointer){
	bool* p = (bool*) pointer;
	if( p == &display_bounding_boxes ){ display_bounding_boxes = (state>0); }
	if( p == &display_areas ){ display_areas = (state>0); }
	if( p == &display_filtered_areas ){ display_filtered_areas = (state>0); }
	if( p == &of_only_leafs ){ of_only_leafs = (state>0); }
	if( p == &of_use_own_filter ){ of_use_own_filter = (state>0); }
	if( p == &display_tracker ){ display_tracker = (state>0); }
	if( p == &invert_depth_map ){ 
		invert_depth_map = (state>0);
		for( int i=0; i<256; i++){
			depth_map[i] = 255-depth_map[i];
		}
	}

	redraw();
}

// ---- END Some callbacks for opencv gui ----


void ctrl_c_handler(int s){
	printf("Caught signal %d\n",s);

	depthtree_destroy_workspace( &dworkspace );
	threshtree_destroy_workspace( &tworkspace );
	blobtree_destroy(&frameblobs);

	exit(1); 
}



int main(int argc, char** argv )
{

	//Add signal handler
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = ctrl_c_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);


	//==========================================================
	//Setup

	const int loopMax = 120;
	int loop = 0;

	//Input handling
	if ( argc < 2 )
	{
		printf("usage: DisplayImage.out [Number of algorithm] [image path] [thresh] [grid width]\n"
				"Available Algorithms:\n"
				"0 - Thresh value split image in 'black' and 'white' areas. The areas\n"
				"    can be nested.\n"
				"1 - Depth map conquer pixels into different depth levels. Areas of\n"
				"    level X will be linked together if they are connected by areas\n"
				"    with levels > X. \n\n"
				);
	}else{
		algorithm = atoi(argv[1]);
		algorithm = (algorithm!=0?1:0);
	}

	if ( argc >= 3){
		// Show argv[2] image and then example images
		loop = -1;
	}else{
		// Only use example images in "images/"
	}

	if ( argc >= 4){
		thresh = atoi(argv[3]);
	}

	if ( argc >= 5){
		gridwidth = atoi(argv[4]);
	}

	if ( argc >= 6){
		gridheight = atoi(argv[5]);
	}


//Just test speed
	bool fpsTesting = (argc>=6);
	if( fpsTesting ) {
		std::string filename("");
		if( loop == -1 ){
			filename.append(argv[2]);
		}else{
			std::ostringstream ofilename;
#if 1
			ofilename << "images/" << loop << ".png" ;
#else
			ofilename << "images/home/frame-";
			if( loop<10 ) ofilename << "0";
			if( loop<100 ) ofilename << "0";
			if( loop<1000 ) ofilename << "0";
			ofilename << loop << ".png" ;
#endif
			filename.append(ofilename.str());
		}
		printf("Load image %s\n", filename.c_str());
		fpsTest(filename);

		return 0;
	}




	namedWindow(window_options, CV_WINDOW_AUTOSIZE );
	namedWindow(window_name, CV_WINDOW_AUTOSIZE );

	createTrackbar( "Thresh:", window_options, &thresh, 255, CB_Thresh );
	createTrackbar( "Gridwidth:", window_options, &gridwidth, 5, CB_Thresh );
	createTrackbar( "1/10*(Min Area):", window_options, &of_area_min, 1000, CB_Filter );
	createTrackbar( "1/500*(Max Area):", window_options, &of_area_max, 1000, CB_Filter );
	createTrackbar( "Min Tree Depth:", window_options, &of_tree_depth_min, 100, CB_Filter );
	createTrackbar( "Max Tree Depth:", window_options, &of_tree_depth_max, 100, CB_Filter );
	createTrackbar( "Min Level:", window_options, &of_area_depth_min, 255, CB_Filter );
	createTrackbar( "Max Level:", window_options, &of_area_depth_max, 255, CB_Filter );
	//createTrackbar( "Scale:", window_options, &output_scalefactor, 8, CB_Filter );

#ifdef WITH_QT
	createButton("Bounding boxes",CB_Button1,&display_bounding_boxes,CV_CHECKBOX, display_bounding_boxes );
	createButton("Only leafs",CB_Button1,&of_only_leafs, CV_CHECKBOX, of_only_leafs );
	createButton("Coloured ids",CB_Button1,&display_areas,CV_CHECKBOX, display_areas );
	createButton("Only filtered coloured ids",CB_Button1,&display_filtered_areas,CV_CHECKBOX, display_filtered_areas );
	createButton("Own filter",CB_Button1,&of_use_own_filter, CV_CHECKBOX, of_use_own_filter );
	createButton("Show Tracker",CB_Button1,&display_tracker, CV_CHECKBOX, display_tracker );
	createButton("Invert depth map",CB_Button1,&invert_depth_map, CV_CHECKBOX, invert_depth_map );
#endif

	addGestureTestPattern(gestureStore);

	//Loop over list [Images] or [Image_Path, Images]
	while( loop < loopMax ){

		std::string filename("");
		if( loop == -1 ){
			filename.append(argv[2]);
		}else{
			std::ostringstream ofilename;
#if 1
			ofilename << "images/" << loop << ".png" ;
#else
			ofilename << "images/home/frame-";
			if( loop<10 ) ofilename << "0";
			if( loop<100 ) ofilename << "0";
			if( loop<1000 ) ofilename << "0";
			ofilename << loop << ".png" ;
#endif
			filename.append(ofilename.str());
		}
		printf("Load image %s\n", filename.c_str());
		detection_loop(filename);
		loop++;

		//====================================================================
		//Graphical Output

		redraw();			

		//Keyboard interaction
		int key = waitKey(0);
		printf("Key: %i\n", key);

		switch(key){
			case 27:{ //ESC-Key
								loop=loopMax+1;
								break;
							}
			case 98: // b
			case 65361:{ //Left
									 loop-=2;
									 if(loop<0 && argc<3 ) loop=0;
									 if(loop<-1)loop=0;
									 break;
								 }
		}

		//simple blocking
		while( redraw_pending ) {
			sleep(1);
		}

		//go back to first step
		if( loop==loopMax ){
			loop = 0;
		}
	
	}

	depthtree_destroy_workspace( &dworkspace );
	threshtree_destroy_workspace( &tworkspace );
	blobtree_destroy(&frameblobs);
	uninitStaticGestureVariables();

	return 0;
}
