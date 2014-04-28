#include <stdio.h>
#include <opencv2/opencv.hpp>

#include "blobtree.h"

using namespace cv;

int main(int argc, char** argv )
{

		const int loopMax = 120;
		int loop = 0;

    if ( argc < 2 )
    {
        printf("usage: DisplayImage.out <Image_Path> [Thresh]\n");
    }else{
			loop = -1;
		}

		int thresh = 100;
		if( argc > 2 ){
			thresh = atoi(argv[2]);
		}
		printf("Thresh: %i\n", thresh);

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

			Blobtree *m_blob = blobtree_create();

			BlobtreeRect roi0 = {0,0,W, H-8 };//shrink height because lowest rows contains noise.

			blobtree_find_blobs(m_blob, ptr, W, H, roi0, thresh);
			blobtree_set_filter(m_blob, F_AREA_MIN, 25); //filter out small blobs
			blobtree_set_filter(m_blob, F_AREA_MAX, 10000000); //filter out big blobs

			blobtree_set_filter(m_blob, F_DEPTH_MIN, 1);//filter out top level blob for whole image
			blobtree_set_filter(m_blob, F_DEPTH_MAX, 1);//filter out blobs included into bigger blobs

			BlobtreeRect *roi;
			Node *curNode = blobtree_first(m_blob);
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

				curNode = blobtree_next(m_blob);
			}
			blobtree_destroy(m_blob);
			m_blob = NULL;


			//====================================================================

			namedWindow("Display Image", CV_WINDOW_AUTOSIZE );

			cv::Mat out;
			cv::resize(mat, out, mat.size()*2, 0, 0, INTER_NEAREST);

			imshow("Display Image", out);

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
