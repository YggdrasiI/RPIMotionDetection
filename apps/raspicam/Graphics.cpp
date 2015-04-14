#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>

#ifdef WITH_OCV
#include <opencv2/opencv.hpp>
#endif

#include "bcm_host.h"
#include "RaspiImv.h"

#include "Graphics.h"
#include "DrawingFunctions.h"

#include "Tracker2.h"
extern Tracker2 tracker;

#define check() assert(glGetError() == 0)

// Header for drawing function. Definition in libs/tracker/DrawingOpenGL.cpp
void tracker_drawBlobsGL(Tracker &tracker, int screenWidth, int screenHeight, bool drawHistoryLines = false, std::vector<cBlob> *toDraw = NULL, GfxTexture *target = NULL);
void tracker_drawHistory( Tracker &tracker, int screenWidth, int screenHeight, cBlob &blob, GfxTexture *target);

//List of Gfx*Objects which will be used in this app.
extern uint32_t GScreenWidth;
extern uint32_t GScreenHeight;
extern EGLDisplay GDisplay;
extern EGLSurface GSurface;
extern EGLContext GContext;

extern GfxShader GSimpleVS;
extern GfxShader GSimpleFS;
extern GfxShader GBlobFS;
extern GfxShader GBlobsVS;
extern GfxShader GBlobsFS;
extern GfxShader GColouredLinesFS;

extern GfxProgram GSimpleProg;
extern GfxProgram GBlobProg;
extern GfxProgram GBlobsProg;
extern GfxProgram GColouredLinesProg;

extern GLuint GQuadVertexBuffer;

GfxTexture imvTexture;
static std::vector<cBlob> blobCache;



void InitGraphics()
{
	bcm_host_init();
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	static const EGLint context_attributes[] = 
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLConfig config;

	// get an EGL display connection
	GDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(GDisplay!=EGL_NO_DISPLAY);
	check();

	// initialize the EGL display connection
	result = eglInitialize(GDisplay, NULL, NULL);
	assert(EGL_FALSE != result);
	check();

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(GDisplay, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	check();

	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	check();

	// create an EGL rendering context
	GContext = eglCreateContext(GDisplay, config, EGL_NO_CONTEXT, context_attributes);
	assert(GContext!=EGL_NO_CONTEXT);
	check();

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &GScreenWidth, &GScreenHeight);
	assert( success >= 0 );

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = GScreenWidth;
	dst_rect.height = GScreenHeight;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = GScreenWidth << 16;
	src_rect.height = GScreenHeight << 16;        

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		0/*layer*/, &dst_rect, 0/*src*/,
		&src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

	nativewindow.element = dispman_element;
	nativewindow.width = GScreenWidth;
	nativewindow.height = GScreenHeight;
	vc_dispmanx_update_submit_sync( dispman_update );

	check();

	GSurface = eglCreateWindowSurface( GDisplay, config, &nativewindow, NULL );
	assert(GSurface != EGL_NO_SURFACE);
	check();

	// connect the context to the surface
	result = eglMakeCurrent(GDisplay, GSurface, GSurface, GContext);
	assert(EGL_FALSE != result);
	check();

	// Set background color and clear buffers
	glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
	glClear( GL_COLOR_BUFFER_BIT );

	InitShaders();
}

void InitTextures(uint32_t glWinWidth, uint32_t glWinHeight)
{
	/* Begin of row values is NOT word-aligned. Set alignment to 1 */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
	imvTexture.CreateGreyScale(121,68);
	//imvTexture.GenerateFrameBuffer();
}

void RedrawGui()
{
	// no gui in this app
}


void RedrawTextures()
{

	imvTexture.SetPixels(motion_data.imv_norm);
	//DrawTextureRect(&imvTexture,-1.0, -1.f,-1.f,1.f,1.f,NULL);
	//Use Scaling to flip horizontal
	//DrawTextureRect(&imvTexture,-1.0, .5f,-.5f,-.5f,.5f,NULL);
	DrawTextureRect(&imvTexture,0.4, 1.0f,-1.0f,-1.0f,1.0f,NULL);

	blobCache.clear();
	tracker.getFilteredBlobs(TRACK_ALL_ACTIVE|LIMIT_ON_N_OLDEST, blobCache);
	//tracker.drawBlobsGL(motion_data.width, motion_data.height);
	tracker_drawBlobsGL(tracker, motion_data.width, motion_data.height, true, &blobCache);

#if 0
	static int savecounter=0;
	if( savecounter == 400 )
		SaveFrameBuffer("/dev/shm/fb1.png");
	++savecounter;
#endif
}

void InitShaders()
{
	//load shaders
	GSimpleVS.LoadVertexShader("shader/simplevertshader.glsl");
	//GSimpleFS.LoadFragmentShader("shader/simplefragshader.glsl");
	GSimpleFS.LoadFragmentShader("shader/blobids_fragshader.glsl");
	GSimpleProg.Create(&GSimpleVS,&GSimpleFS);
	check();
	GBlobFS.LoadFragmentShader("shader/blobfragshader.glsl");
	GBlobProg.Create(&GSimpleVS,&GBlobFS);
	GBlobsVS.LoadVertexShader("shader/blobsvertshader.glsl");
	GBlobsFS.LoadFragmentShader("shader/blobsfragshader.glsl");
	GBlobsProg.Create(&GBlobsVS,&GBlobsFS);

	GColouredLinesFS.LoadFragmentShader("shader/colouredlinesfragshader.glsl");
	GColouredLinesProg.Create(&GBlobsVS,&GColouredLinesFS);

	check();

	//create an ickle vertex buffer
	static const GLfloat quad_vertex_positions[] = {
		0.0f, 0.0f,	1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};
	glGenBuffers(1, &GQuadVertexBuffer);
	check();
	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_positions), quad_vertex_positions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	check();
}

void ReleaseGraphics()
{

}

int GetShader(){
	return ShaderNormal;
}

