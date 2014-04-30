#include <stdio.h>
#include <opencv2/opencv.hpp>

#include "blobtree.h"
#include "depthtree.h"

using namespace cv;

/* Let only nodes with depth level = X passes. */
static int my_filter(Node *n){
	Blob* data = (Blob*) n->data;
	if( data->depth_level > 3 ) return 3;
	if( data->depth_level == 3 ) return 0;
	return 1;
}

int main(int argc, char** argv )
{

		//==========================================================
		//Setup
		
		const int loopMax = 120;
		int loop = 0;
		int thresh = 100;

		//Init depth_map
		unsigned char depth_map[256];
		int i; for( i=0; i<256; i++) depth_map[i] = i/50;

		//Input handling
    if ( argc < 2 )
    {
        printf("usage: DisplayImage.out [Image_Path] [Thresh]\n");
    }else{
			loop = -1;
		}

		if( argc > 2 ){
			thresh = atoi(argv[2]);
		}



		//Loop over list [Images] or [Image_Path, Images]
		while( loop < loopMax ){
			Mat mat;
			if( loop == -1 ){
				mat = imread( argv[1], CV_LOAD_IMAGE_GRAYSCALE );
			}else{
				std::ostringstream filename;
				filename << "png/" << loop << ".png" ;
				mat = imread( filename.str(), CV_LOAD_IMAGE_GRAYSCALE );
			}
			loop++;

			if ( !mat.data )
			{
				if( loop == -1 ) continue;
				printf("No image data \n");
				return -1;
			}

			/*
			//Convert Image to black/white image
			Mat grey(mat.size(),CV_8UC1);
			//mat.convertTo(grey, CV_8UC1);//wrong?!
			cv::cvtColor(mat, grey, CV_RGB2GRAY);
			mat = grey;
			*/

			Mat color(mat.size(),CV_8UC3);
			cv::cvtColor(mat, color, CV_GRAY2BGR);

			printf("Image data continuous: %s\n", mat.isContinuous()?"Yes":"No" );

			//====================================================
			const int W = mat.size().width;
			const int H = mat.size().height;
			const uchar* ptr = mat.data;

			//Init workspace
			DepthtreeWorkspace *workspace = NULL;
			depthtree_create_workspace( W, H, &workspace );
			if( workspace == NULL ){
				printf("Unable to create workspace.\n");
				return -1;
			}

			Blobtree *blob = blobtree_create();

			// Set distance between compared pixels.	
			// Look at blobtree.h for more information.
			blobtree_set_grid(blob, 1,1);

			BlobtreeRect roi0 = {0,0,W, H };//shrink height because lowest rows contains noise.

			//blobtree_find_blobs(blob, ptr, W, H, roi0, thresh);
			depthtree_find_blobs(blob, ptr, W, H, roi0, depth_map, workspace);

			/* Textual output of whole tree of blobs. */
			//print_tree(blob->tree->root,0);

			//Set output filter
			blobtree_set_filter(blob, F_AREA_MIN, 25); //filter out small blobs
			blobtree_set_filter(blob, F_AREA_MAX, 10000000); //filter out big blobs

			blobtree_set_filter(blob, F_TREE_DEPTH_MIN, 1);//filter out top level blob for whole image
			blobtree_set_filter(blob, F_TREE_DEPTH_MAX, 3);//filter out blobs included into bigger blobs

			//blobtree_set_filter(blob, F_ONLY_LEAFS, 1);
			
			//Add own filter over function pointer
			blobtree_set_extra_filter(blob, my_filter);

			//fill color image with id map of filtered list of blobs
			//1. Create mapping for filtered list
			depthree_filter_blob_ids(blob,workspace);

			int fid;
			int* ids = workspace->ids;
			int* cm = workspace->comp_same;

			int* ris = workspace->real_ids;
			int* riv = workspace->real_ids_inv;
			int* bif = workspace->blob_id_filtered;//map id on id of unfiltered node

			for( int y=0; y<H; ++y){
				for( int x=0; x<W; ++x) {
					//fid = *(cm + fid); //fid = *(cm + fid);
					fid = *(bif+ *ids);

					const cv::Vec3b col( (fid*5*5+100)%256, (fid*7*7+10)%256, (fid*29*29+1)%256 );
					color.at<Vec3b>(y, x) = col;
					++ids;
				}
			}

			BlobtreeRect *roi;
			Node *curNode = blobtree_first(blob);

			int num=0;

			while( curNode != NULL ){
				num++;
				Blob *data = (Blob*)curNode->data;
				roi = &data->roi;
				int x     = roi->x + roi->width/2;
				int y     = roi->y + roi->height/2;

				printf("Blob with id %i: x=%i y=%i w=%i h=%i area=%i, depthlevel=%i\n",data->id,
						roi->x, roi->y,
						roi->width, roi->height,
						data->area,
						data->depth_level
						);

				cv::Rect cvRect(
						max(roi->x-1,0),
						max(roi->y-1,0),
						roi->width+2,
						roi->height+2
						);
				const cv::Scalar col1(205.0f);
				cv::rectangle( mat, cvRect, col1, 1);//, int thickness=1, int lineType=8, int shift=0 )¶
				//const cv::Scalar col2(255.0f,255.0f- (num*29)%256,(num*5)%256);
				const cv::Scalar col2(255,255, 255);
				cv::rectangle( color, cvRect, col2, 1);//, int thickness=1, int lineType=8, int shift=0 )¶

				curNode = blobtree_next(blob);
			}

			depthtree_destroy_workspace( &workspace );
			blobtree_destroy(blob);
			blob = NULL;


			//====================================================================
			//Graphical Output

			
			namedWindow("Display Image", CV_WINDOW_AUTOSIZE );

			cv::Mat out;
			cv::resize(color, out, color.size()*2, 0, 0, INTER_NEAREST);
			imshow("Display Image", out);


			//Keyboard interaction
			int key = waitKey(0);
			printf("Key: %i\n", key);
			switch(key){
				case 27:{ //ESC-Key
									loop=loopMax;
									break;
								}
				case 65361:{ //Left
										 loop-=2;
										 if(loop<-1)loop=0;
										 break;
									 }
			}
			
			//loop=loopMax;
		}

	 return 0;
}
