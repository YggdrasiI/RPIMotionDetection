#include <stdio.h>
#include <unistd.h>
#include "camera.h"
#include "graphics.h"
#include "../../Fps.h"

#include <opencv2/opencv.hpp>
#include "threshtree.h"

//#define MAIN_TEXTURE_WIDTH 512
//#define MAIN_TEXTURE_HEIGHT 512
//#define MAIN_TEXTURE_WIDTH 960
//#define MAIN_TEXTURE_HEIGHT 540
//#define MAIN_TEXTURE_WIDTH 640
//#define MAIN_TEXTURE_HEIGHT 480
#define MAIN_TEXTURE_WIDTH 512
#define MAIN_TEXTURE_HEIGHT 512

char tmpbuff[MAIN_TEXTURE_WIDTH*MAIN_TEXTURE_HEIGHT*4];

//entry point
int main(int argc, const char **argv)
{
	//should the camera convert frame data from yuv to argb automatically?
	bool do_argb_conversion = false;

	//how many detail levels (1 = just the capture res, > 1 goes down by half each level, 4 max)
	int num_levels = 4;

	Fps fps(50);


	unsigned char thresh = 200;
	//filter
	int of_area_min = 1000;
	int of_area_max = 400000;
	int of_tree_depth_min = 1;
	int of_tree_depth_max = 100;

	BlobtreeRect roi = {0,0,MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT };
	Blobtree *blob = NULL;
	blobtree_create(&blob);

	ThreshtreeWorkspace *tworkspace = NULL;
	threshtree_create_workspace( MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT, &tworkspace );
	if( tworkspace == NULL ){
		printf("Unable to create workspace.\n");
		return -1;
	}


	blobtree_set_grid(blob, 4, 4);
	//set filter
	blobtree_set_filter(blob, F_AREA_MIN, of_area_min );
	blobtree_set_filter(blob, F_AREA_MAX, of_area_max );
	blobtree_set_filter(blob, F_TREE_DEPTH_MIN, of_tree_depth_min );
	blobtree_set_filter(blob, F_TREE_DEPTH_MAX, of_tree_depth_max );

	//init graphics and the camera
	InitGraphics();
	CCamera* cam = StartCamera(MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT,30,num_levels,do_argb_conversion);
	//CCamera* cam = StartCamera(MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT,60,num_levels,do_argb_conversion);
	

	//create 4 textures of decreasing size
	GfxTexture textures[4];
	for(int texidx = 0; texidx < num_levels; texidx++)
		textures[texidx].create(MAIN_TEXTURE_WIDTH >> texidx, MAIN_TEXTURE_HEIGHT >> texidx);

	IplImage* cvout = NULL;
	cvout = cvCreateImage(cvSize(roi.width,roi.height), IPL_DEPTH_8U, 1);
	IplImage* cvin = NULL;
	cvin = cvLoadImage("../../../images/tex.png",1);//0-gray, 1-color

	printf("Running frame loop\n");
	for(int i = 0; i < 30000; i++)
	{
		//pick a level to read based on current frame (flicking through them every 30 frames)
		//int texidx = (i / 30)%num_levels;
		int texidx = 0;

		//lock the chosen frame buffer, and copy it directly into the corresponding open gl texture
		const void* frame_data; int frame_sz;
		if(cam->BeginReadFrame(texidx,frame_data,frame_sz))
		{
			if(do_argb_conversion)
			{
				//if doing argb conversion the frame data will be exactly the right size so just set directly
				textures[texidx].setPixels(frame_data);
			}
			else
			{
				//if not converting argb the data will be the wrong size and look weird, put copy it in
				//via a temporary buffer just so we can observe something happening!
				memcpy(tmpbuff,frame_data,frame_sz);
				textures[texidx].setPixels(tmpbuff);
			}
			cam->EndReadFrame(texidx);
		}

		//begin frame, draw the texture then end frame (the bit of maths just fits the image to the screen while maintaining aspect ratio)
		BeginFrame();
		float aspect_ratio = float(MAIN_TEXTURE_WIDTH)/float(MAIN_TEXTURE_HEIGHT);
		//float screen_aspect_ratio = 1280.f/720.f;
		//float screen_aspect_ratio = 1920.f/1080.f;
		float screen_aspect_ratio = 800.0f/600.0f;
		float r = aspect_ratio/screen_aspect_ratio;
		DrawTextureRectTri(&textures[texidx], -1.0, -1.0, 1.0, 1.0); 
		
		//Blob detection 
		int needed_sz = (roi.x+roi.width)*(roi.y+roi.height);
		//printf("(W,H) = (%i,%i)   ", roi.width, roi.height);
		//printf("Frame size: %i, needed: %i\n", frame_sz, needed_sz);
		if( frame_sz>= needed_sz  )
		{
			threshtree_find_blobs(blob, (unsigned char*) frame_data, roi.width, roi.height, roi, thresh, tworkspace);
			//threshtree_find_blobs(blob, (unsigned char*) cvin->imageData, roi.width, roi.height, roi, thresh, tworkspace);
			handle_blobs(blob, &roi ,(textures+texidx) );
			//print_tree(blob->tree->root,0);
		
			char* tmp =  cvout->imageData;
			cvout->imageData = (char*)frame_data;
			char fn[256];
			sprintf(fn, "/dev/shm/picam_%d.jpg", i);
			//cvSaveImage(fn, cvout, 0);
			cvout->imageData = tmp;
			
		}

		EndFrame();
		//printf("Frameende\n");

		fps.next(stdout);
	}

	StopCamera();

	threshtree_destroy_workspace( &tworkspace );
	blobtree_destroy(&blob);

	cvReleaseImage(&cvout);

}



