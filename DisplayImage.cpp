#include <stdio.h>
#include <opencv2/opencv.hpp>

#include "blobtree.h"
#include "depthtree.h"

using namespace cv;

int main(int argc, char** argv )
{

		//==========================================================
		//Setup
		
		const int loopMax = 3;//120;
		int loop = 0;
		int thresh = 100;

		//Init depth_map
		unsigned char depth_map[256];
		int i; for( i=0; i<256; i++) depth_map[i] = i/10;

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

			BlobtreeRect roi0 = {0,0,W, H-8 };//shrink height because lowest rows contains noise.

			blobtree_find_blobs(blob, ptr, W, H, roi0, thresh);
			//depthtree_find_blobs(blob, ptr, W, H, roi0, depth_map, workspace);


			blobtree_set_filter(blob, F_AREA_MIN, 25); //filter out small blobs
			blobtree_set_filter(blob, F_AREA_MAX, 10000000); //filter out big blobs

			blobtree_set_filter(blob, F_DEPTH_MIN, 1);//filter out top level blob for whole image
			blobtree_set_filter(blob, F_DEPTH_MAX, 1);//filter out blobs included into bigger blobs

			BlobtreeRect *roi;
			Node *curNode = blobtree_first(blob);
			while( curNode != NULL ){
				Blob *data = (Blob*)curNode->data;
				roi = &data->roi;
				int x     = roi->x + roi->width/2;
				int y     = roi->y + roi->height/2;

				/*
				printf("Blob with id %i: x=%i y=%i w=%i h=%i area=%i\n",data->id,
						roi->x, roi->y,
						roi->width, roi->height,
						data->area
						);
						*/

				cv::Rect cvRect(
						max(roi->x-1,0),
						max(roi->y-1,0),
						roi->width+2,
						roi->height+2
						);
				//const cv::Scalar color(255.0f,255.0f,255.0f);
				const cv::Scalar color(255.0f);
				cv::rectangle( mat, cvRect, color, 1);//, int thickness=1, int lineType=8, int shift=0 )¶

				curNode = blobtree_next(blob);
			}

			depthtree_destroy_workspace( &workspace );
			blobtree_destroy(blob);
			blob = NULL;


			//====================================================================
			//Graphical Output

			
			namedWindow("Display Image", CV_WINDOW_AUTOSIZE );

			cv::Mat out;
			cv::resize(mat, out, mat.size()*2, 0, 0, INTER_NEAREST);
			//imshow("Display Image", out);

			/* //Hm, IplImage stellt keine Option zur Verfügung die Schritt-
			   //weite von Pixeln zu ändern.
			IplImage test1 = out;
			//IplImage test2 = cvCreateImage(mat.size(), IPL_DEPTH_8U, 1);
			printf("widthStep: %i, width: %i \n align: %i, dataOrder: %i\n",
					test1.widthStep, test1.width,
					test1.align, test1.dataOrder
					);
			cvShowImage("Display Image", &test1);
			*/
	
			printf("Steps: %i, %i\n", out.step[0], out.step[1]);
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
			
		}

	 return 0;
}
