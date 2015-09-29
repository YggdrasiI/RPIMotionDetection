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
#include "GraphicsPong.h"
#include "DrawingFunctions.h"
#include "Tracker2.h"
extern Tracker2 tracker;

#include "Pong.h"
extern Pong pong;

#include <FontManager.h>
extern FontManager fontManager;

// Header for drawing function. Definition in libs/tracker/DrawingOpenGL.cpp
void tracker_drawBlobsGL(Tracker &tracker, int screenWidth, int screenHeight, bool drawHistoryLines = false, std::vector<cBlob> *toDraw = NULL, GfxTexture *target = NULL);
void tracker_drawHistory( Tracker &tracker, int screenWidth, int screenHeight, cBlob &blob, GfxTexture *target);

//statics from RaspiTex.c
//extern "C" long long time_diff;
extern "C" long long update_fps();

//List of Gfx*Objects which will be used in this app.
uint32_t GScreenWidth;
uint32_t GScreenHeight;
EGLDisplay GDisplay;
EGLSurface GSurface;
EGLContext GContext;

GfxShader GSimpleVS;
GfxShader GSimpleFS;
GfxShader GBlobFS;
GfxShader GBlobsVS;
GfxShader GBlobsFS;
GfxShader GPongFS;
GfxShader GColouredLinesFS;
GfxShader GGuiVS;
GfxShader GGuiFS;
GfxShader GFontVS;
GfxShader GFontFS;

GfxProgram GSimpleProg;
GfxProgram GBlobProg;
GfxProgram GPongProg;
GfxProgram GBlobsProg;
GfxProgram GColouredLinesProg;
GfxProgram GGuiProg;
GfxProgram GFontProg;

GLuint GQuadVertexBuffer;

GfxTexture imvTexture;
GfxTexture numeralsTexture;
GfxTexture raspiTexture;//Texture of Logo
GfxTexture guiTexture; //only redrawed on gui changes
GfxTexture blobsTexture; //lower resolution as video frame (similar to guiTexture)
extern "C" RASPITEXUTIL_TEXTURE_T  guiBuffer;
extern "C" RASPITEXUTIL_TEXTURE_T  blobsBuffer;

bool guiNeedRedraw = true;
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
	//gl window size could differ...
	//graphics_get_display_size(0 /* LCD */, &GScreenWidth, &GScreenHeight);
	GScreenWidth = glWinWidth;
	GScreenHeight = glWinHeight;

	/* Begin of row values is NOT word-aligned. Set alignment to 1 */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
	imvTexture.CreateGreyScale(121,68);
	//imvTexture.GenerateFrameBuffer();
	
	//restore default
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); 
	
	numeralsTexture.CreateFromFile("../../images/numerals.png");
	numeralsTexture.SetInterpolation(true);

	raspiTexture.CreateFromFile("../../images/Raspi_Logo_128.png");
	raspiTexture.SetInterpolation(true);

	//guiTexture.CreateRGBA(GScreenWidth,GScreenHeight, NULL);
	guiTexture.CreateRGBA(800,600, NULL);
	guiTexture.SetInterpolation(false);
	guiTexture.GenerateFrameBuffer();
	guiTexture.toRaspiTexture(&guiBuffer);

	blobsTexture.CreateRGBA(800,600, NULL);
	blobsTexture.SetInterpolation(false);
	blobsTexture.GenerateFrameBuffer();
	blobsTexture.toRaspiTexture(&blobsBuffer);
}

/* Raspivid uses gl_scenes/pong.c to draw scenes.
 * pong_redraw() in the above file calls RedrawGui().
 */
void RedrawGui()
{
	blobCache.clear();
	tracker.getFilteredBlobs(TRACK_ALL_ACTIVE, blobCache);
	tracker_drawBlobsGL(tracker, motion_data.width, motion_data.height, false, &blobCache, &blobsTexture);

	if( !guiNeedRedraw ) return;
	//old approach
	//DrawGui(&numeralsTexture,&pong,0.05f,	-1.0f,1.0f,1.0f,-1.0f, &guiTexture);
	//new approach
	fontManager.Render(-1.0f,1.0f,1.0f,-1.0f, &guiTexture);

	guiNeedRedraw = false;
}

void RedrawTextures()
{

	//imvTexture.SetPixels(motion_data.imv_norm);
	//DrawTextureRect(&imvTexture,-1.0,1.0f,-1.0f,-1.0f,1.0f,NULL);
	
	//DrawGui(&numeralsTexture,&pong,0.05f,
	//		-1.0f,1.0f,1.0f,-1.0f, NULL);

	/*
	blobCache.clear();
	tracker.getFilteredBlobs(ALL_ACTIVE, blobCache);
	tracker_drawBlobsGL(tracker, motion_data.width, motion_data.height, &blobCache, NULL);
	*/

	/* Call update_fps from RaspiTex.c, which returns  time_diff
	 * as difference to previous measurement. Uses static variables -> Don't call update_fps twice...
	 * dt=1 means redrawing with 30fps.
	 *
	 */
	float dt = update_fps()/33.3f; // /1000.0*30.0;

	//Event handling (with old position) and position update
	pong.checkCollision(motion_data.width, motion_data.height, blobCache );
	pong.updatePosition(dt);


	// Draw pong ball
	pong.drawBall();

}

void InitShaders()
{
	//load the test shaders
	GSimpleVS.LoadVertexShader("shader/simplevertshader.glsl");
	GSimpleFS.LoadFragmentShader("shader/simplefragshader.glsl");
	GSimpleProg.Create(&GSimpleVS,&GSimpleFS);
	check();
	GBlobFS.LoadFragmentShader("shader/blobfragshader.glsl");
	GBlobProg.Create(&GSimpleVS,&GBlobFS);

	GBlobsVS.LoadVertexShader("shader/blobsvertshader.glsl");
	GBlobsFS.LoadFragmentShader("shader/blobsfragshader.glsl");
	GBlobsProg.Create(&GBlobsVS,&GBlobsFS);

	GGuiVS.LoadVertexShader("shader/guivertshader.glsl");
	GGuiFS.LoadFragmentShader("shader/guifragshader.glsl");
	GGuiProg.Create(&GGuiVS,&GGuiFS);

	GPongFS.LoadFragmentShader("shader/pongfragshader.glsl");
	GPongProg.Create(&GSimpleVS,&GPongFS);

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
	const int* score = pong.getScore();
	return  ((score[0]+score[1])/3) %SHADER_TYPE_NUM;

	return ShaderNormal;
}



// Extra drawing functions for pong app.

void DrawGui(GfxTexture *scoreTexture, Pong *pong, float border, float x0, float y0, float x1, float y1, GfxTexture* render_target)
{
	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->GetFramebufferId());
		glViewport ( 0, 0, render_target->GetWidth(), render_target->GetHeight() );
		check();
	}

	glUseProgram(GGuiProg.GetId());	check();

	glUniform2f(glGetUniformLocation(GGuiProg.GetId(),"offset"),x0,y0);
	glUniform2f(glGetUniformLocation(GGuiProg.GetId(),"scale"),x1-x0,y1-y0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scoreTexture->GetId());
	glUniform1i(glGetUniformLocation(GGuiProg.GetId(),"numerals"), 0);

	glUniform2f(glGetUniformLocation(GGuiProg.GetId(),"border"),
			pong->isActivePlayer(0)?border:0.0,
			pong->isActivePlayer(1)?1.0-border:1.0 );//border
	glUniform2f(glGetUniformLocation(GGuiProg.GetId(),"score"),
			(float) pong->getScore()[0],
			(float) pong->getScore()[1]);//score
	if( pong->isActivePlayer(1) ) {
		glUniform4f(glGetUniformLocation(GGuiProg.GetId(),"scorePosLeft"), 0.0, 0.25, 0.5, 0.75); //scorePosLeft
	}else{
		glUniform4f(glGetUniformLocation(GGuiProg.GetId(),"scorePosLeft"), 0.0, 0.1, 0.1, -0.2); //scorePosLeft
	}

	if( pong->isActivePlayer(0) ){
		glUniform4f(glGetUniformLocation(GGuiProg.GetId(),"scorePosRight"), 0.5, 0.25, 1.0, 0.75); //scorePosRight
	}else{
		glUniform4f(glGetUniformLocation(GGuiProg.GetId(),"scorePosRight"), 0.9, 0.1, 1.0, -0.2); //scorePosRight
	}

	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);	check();

	GLuint loc = glGetAttribLocation(GBlobProg.GetId(),"vertex");
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);	check();
	glEnableVertexAttribArray(loc);	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 ); check();

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}


// Like DrawTextureRect with extra color information
void DrawPongRect(GfxTexture* texture, float r, float g, float b,
		float x0, float y0, float x1, float y1, GfxTexture* render_target)
{
	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,render_target->GetFramebufferId());
		glViewport ( 0, 0, render_target->GetWidth(), render_target->GetHeight() );
		check();
	}

	glUseProgram(GPongProg.GetId());	check();

	glUniform2f(GPongProg.GetHandle("offset"),x0,y0);
	glUniform2f(GPongProg.GetHandle("scale"),x1-x0,y1-y0);
	glUniform1i(GPongProg.GetHandle("tex"), 0);
	glUniform3f(GPongProg.GetHandle("colorMod"),r,g,b);
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);	check();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,texture->GetId());	check();

	GLuint loc = glGetAttribLocation(GSimpleProg.GetId(),"vertex");
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);	check();
	glEnableVertexAttribArray(loc);	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 ); check();

	//glDisableVertexAttribArray(loc);	check();//neu
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if(render_target )
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	}
}

