#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>
#include "bcm_host.h"
#include "graphics.h"
#include "util.h"
#include "helper.h"

#define check() assert(glGetError() == 0)

uint32_t GScreenWidth;
uint32_t GScreenHeight;
EGLDisplay GDisplay;
EGLSurface GSurface;
EGLContext GContext;

GfxShader GSimpleVS;
GfxShader GSimpleFS;
GfxProgram GSimpleProg;
GfxShader GTriVS;
GfxShader GTriFS;
GfxProgram GTriProg;
GfxShader GFillVS;
GfxShader GFillFS;
GfxProgram GFillProg;
GfxShader GYuvVS;
GfxShader GYuvFS;
GfxProgram GYuvProg;
GLuint GQuadVertexBuffer;
GLuint GQuadVertexBufferB;

static const int WIN_SIZE = 1400;

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

	//load the test shaders
	GSimpleVS.loadVertexShader("simplevertshader.glsl");
	GSimpleFS.loadFragmentShader("simplefragshader.glsl");
	GSimpleProg.create(&GSimpleVS,&GSimpleFS);
	check();

	GTriVS.loadVertexShader("trivertshader.glsl");
	GTriFS.loadFragmentShader("trifragshader.glsl");
	GTriProg.create(&GTriVS,&GTriFS);
	check();

	GFillVS.loadVertexShader("red.vert.glsl");
	GFillFS.loadFragmentShader("red.frag.glsl");
	GFillProg.create(&GFillVS,&GFillFS);
	check();

	GYuvVS.loadVertexShader("red.vert.glsl");
	GYuvFS.loadFragmentShader("red.frag.glsl");
	GYuvProg.create(&GYuvVS,&GYuvFS);
	check();

	//glUseProgram(GSimpleProg.getId());
	glUseProgram(GTriProg.getId());
	check();

	//create an ickle vertex buffer
	/*
	static const GLfloat quad_vertex_positions[] = {
		0.0f, 0.0f,	1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};
	*/
	static const GLfloat quad_vertex_positions[] = {
		-1.0f, -1.0f,	1.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};
	static const GLint indizies[] = {
		0, 1, 2,
		1, 2, 3
	};

	static const GLfloat quad_vertex_positionsB[] = {
/* Frist triangle */
		0.0f, 0.0f,	1.0f, 0.0f, //a
						1.0f, 0.0f, 1.0f, 1.0f, //b
						0.0f, 1.0f, 1.0f, 1.0f, //c
		1.0f, 0.0f, 1.0f, 1.0f, //b
						0.0f, 1.0f, 1.0f, 1.0f, //c
						0.0f, 0.0f,	1.0f, 1.0f, //a
		0.0f, 1.0f, 1.0f, 2.0f, //c
						0.0f, 0.0f,	1.0f, 1.0f, //a
						1.0f, 0.0f, 1.0f, 1.0f, //b

/* Second triangle */
		1.0f, 0.0f, 1.0f, 1.0f, //b
						0.0f, 1.0f, 1.0f, 1.0f, //c
						1.0f, 1.0f, 1.0f, 1.0f, //d
		0.0f, 1.0f, 1.0f, 2.0f, //c
						1.0f, 1.0f, 1.0f, 1.0f, //d
						1.0f, 0.0f, 1.0f, 1.0f, //b
		1.0f, 1.0f, 1.0f, 0.0f, //d
						1.0f, 0.0f, 1.0f, 1.0f, //b
						0.0f, 1.0f, 1.0f, 1.0f, //c
	};



	glGenBuffers(1, &GQuadVertexBuffer);
	check();
	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_positions), quad_vertex_positions, GL_STATIC_DRAW);
	check();

	static GLfloat *quad_vertex_positionsC = NULL;
	static int quad_vertex_positionsC_len;

	quad_vertex_positionsC_len = expandVertexData(
			quad_vertex_positions, indizies,
			sizeof(indizies) , 4, &quad_vertex_positionsC);

	glGenBuffers(1, &GQuadVertexBufferB);
	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBufferB);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_positionsB), quad_vertex_positionsB, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, quad_vertex_positionsC_len, quad_vertex_positionsC, GL_STATIC_DRAW);
	free( quad_vertex_positionsC ); 
	check();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	check();


	//Test 
	glEnable( GL_DEPTH_TEST );
}

void BeginFrame()
{
	// Prepare viewport
	//glViewport ( 0, 0, GScreenWidth, GScreenHeight );
	//glViewport ( (GScreenWidth-GScreenHeight)/2 , 0, GScreenHeight, GScreenHeight );
	glViewport ( (GScreenWidth-512)/2 ,(GScreenHeight-512)/2 , 512, 512 );
	check();

	// Clear the background
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	check();
}

void EndFrame()
{
	eglSwapBuffers(GDisplay,GSurface);
	check();
}

void ReleaseGraphics()
{
}

// printShaderInfoLog
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
void printShaderInfoLog(GLint shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen];
		// error check for fail to allocate memory omitted
		glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
		std::cout << "InfoLog : " << std::endl << infoLog << std::endl;
		delete [] infoLog;
	}
}

bool GfxShader::loadVertexShader(const char* filename)
{
	//cheeky bit of code to read the whole file into memory
	assert(!Src);
	FILE* f = fopen(filename, "rb");
	assert(f);
	fseek(f,0,SEEK_END);
	int sz = ftell(f);
	fseek(f,0,SEEK_SET);
	Src = new GLchar[sz+1];
	fread(Src,1,sz,f);
	Src[sz] = 0; //null terminate it!
	fclose(f);

	//now create and compile the shader
	GlShaderType = GL_VERTEX_SHADER;
	Id = glCreateShader(GlShaderType);
	glShaderSource(Id, 1, (const GLchar**)&Src, 0);
	glCompileShader(Id);
	check();

	//compilation check
	GLint compiled;
	glGetShaderiv(Id, GL_COMPILE_STATUS, &compiled);
	if(compiled==0)
	{
		printf("Failed to compile vertex shader %s:\n%s\n", filename, Src);
		printShaderInfoLog(Id);
		glDeleteShader(Id);
		return false;
	}
	else
	{
		printf("Compiled vertex shader %s:\n%s\n", filename, Src);
	}

	return true;
}

bool GfxShader::loadFragmentShader(const char* filename)
{
	//cheeky bit of code to read the whole file into memory
	assert(!Src);
	FILE* f = fopen(filename, "rb");
	assert(f);
	fseek(f,0,SEEK_END);
	int sz = ftell(f);
	fseek(f,0,SEEK_SET);
	Src = new GLchar[sz+1];
	fread(Src,1,sz,f);
	Src[sz] = 0; //null terminate it!
	fclose(f);

	//now create and compile the shader
	GlShaderType = GL_FRAGMENT_SHADER;
	Id = glCreateShader(GlShaderType);
	glShaderSource(Id, 1, (const GLchar**)&Src, 0);
	glCompileShader(Id);
	check();

	//compilation check
	GLint compiled;
	glGetShaderiv(Id, GL_COMPILE_STATUS, &compiled);
	if(compiled==0)
	{
		printf("Failed to compile fragment shader %s:\n%s\n", filename, Src);
		printShaderInfoLog(Id);
		glDeleteShader(Id);
		return false;
	}
	else
	{
		printf("Compiled fragment shader %s:\n%s\n", filename, Src);
	}

	return true;
}

bool GfxProgram::create(GfxShader* vertex_shader, GfxShader* fragment_shader)
{
	VertexShader = vertex_shader;
	FragmentShader = fragment_shader;
	Id = glCreateProgram();
	glAttachShader(Id, VertexShader->getId());
	glAttachShader(Id, FragmentShader->getId());
	glLinkProgram(Id);
	check();
	printf("Created program id %d from vs %d and fs %d\n", getId(), VertexShader->getId(), FragmentShader->getId());

	// Prints the information log for a program object
	char log[1024];
	glGetProgramInfoLog(Id,sizeof log,NULL,log);
	printf("%d:program:\n%s\n", Id, log);

	return true;	
}

void DrawTextureRect(GfxTexture* texture, float x0, float y0, float x1, float y1)
{

	const GLuint Prog = GSimpleProg.getId();

	glUseProgram(Prog);
	check();

	glUniform2f(glGetUniformLocation(Prog,"offset"),x0,y0);
	glUniform2f(glGetUniformLocation(Prog,"scale"),x1-x0,y1-y0);
	glUniform1i(glGetUniformLocation(Prog,"tex"), 0);
	check();


	glBindTexture(GL_TEXTURE_2D,texture->getId());
	check();

	GLuint loc = glGetAttribLocation(Prog,"vertex");
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);
	check();
	glEnableVertexAttribArray(loc);
	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	/*
	//Ansatz mit zwei Dreiecken ohne gemeinsame Knoten.
	const GLsizei vertexSize = (4+8)*sizeof(GLfloat); //vertex + both other triangle nodes
	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBufferB);
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, vertexSize, 0);
	check();

	glEnableVertexAttribArray(loc);
	check();

	glDrawArrays(GL_TRIANGLES, 0, 6);
	*/


	check();

	glFinish();
	check();

	glFlush();
	check();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

}


void DrawTextureRectTri(GfxTexture* texture, float x0, float y0, float x1, float y1)
{

	const GLuint Prog = GTriProg.getId();

	glUseProgram(Prog);
	check();

	//Achtung, hier wird auf [0,1] gemappt und daher werden
	//die Argumente der inversen Methode genommen. T^-1 = (y-b)/a., T=ax+b
	glUniform2f(glGetUniformLocation(Prog,"offset"),-x0,-y0);
	glUniform2f(glGetUniformLocation(Prog,"scale"),1.0/(x1-x0),1.0/(y1-y0));
	glUniform1i(glGetUniformLocation(Prog,"tex"), 0);

  const GLuint p1_attrib = glGetAttribLocation(Prog, "p1_3d");
  const GLuint p2_attrib = glGetAttribLocation(Prog, "p2_3d");
  glUniform2f(glGetUniformLocation(Prog,"WIN_SCALE"), WIN_SIZE/2.0, WIN_SIZE/2.0);
  glUniform3f(glGetUniformLocation(Prog,"WIRE_COL"), 0.8,0.3,0.1);
  glUniform3f(glGetUniformLocation(Prog,"FILL_COL"), 0.7,0.8,0.9);
	check();

	//Da unter Opengles einige Inputargumente nicht mehr automatisch erzeugt werden,
	//muss dies per Hand nachgeholt werden.
	//Hier wird  gl_ModelViewProjectionMatrix erzeugt und dem Vertexshader übergeben.
	/* Create and load a rotation matrix uniform. */
	static ESMatrix rotationMatrix;
	static float rotationAngle = 0.0f;
	//rotationAngle += 0.3f;
	esMatrixLoadIdentity(&rotationMatrix);
	esRotate(&rotationMatrix, rotationAngle, 0.0f, 1.0f, 0.0f);//da ist noch was verbuggt
	//esRotate(&rotationMatrix, rotationAngle, 0.0f, 0.0f, 1.0f);
	//esTranslate(&rotationMatrix, -0.5f, -0.5f, 0.0f);//für [0,1] Quadrat
	glUniformMatrix4fv( glGetUniformLocation(Prog,"MyModelViewProjectionMatrix"),
			1, GL_FALSE, (GLfloat*) &rotationMatrix.m[0]);

	check();

	//glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);

	glBindTexture(GL_TEXTURE_2D,texture->getId());
	check();

	GLuint loc = glGetAttribLocation(Prog,"vertex");
	check();

	//glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);
	//check();

	//glEnableVertexAttribArray(loc);
	//check();

	//glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

//Ansatz mit zwei Dreiecken ohne gemeinsame Knoten.
	const GLsizei vertexSize = (4+8)*sizeof(GLfloat) /*vertex + both other triangle nodes*/;
	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBufferB);
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, vertexSize, 0);
	glVertexAttribPointer(p1_attrib, 4, GL_FLOAT, 0, vertexSize, (GLvoid*) (4*sizeof(GLfloat)) );
	glVertexAttribPointer(p2_attrib, 4, GL_FLOAT, 0, vertexSize, (GLvoid*) (8*sizeof(GLfloat)) );
	check();

	glEnableVertexAttribArray(loc);
	glEnableVertexAttribArray(p1_attrib);
	glEnableVertexAttribArray(p2_attrib);
	check();

	glDrawArrays(GL_TRIANGLES, 0, 6);
	check();

	glFinish();
	check();

	glFlush();
	check();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

bool GfxTexture::create(int width, int height, const void* data)
{
	Width = width;
	Height = height;
	glGenTextures(1, &Id);
	check();
	glBindTexture(GL_TEXTURE_2D, Id);
	check();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	check();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
	check();
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

void GfxTexture::setPixels(const void* data)
{
	glBindTexture(GL_TEXTURE_2D, Id);
	check();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	check();
	glBindTexture(GL_TEXTURE_2D, 0);
	check();
}


void DrawYuvTextureRect(GfxTexture* texY, GfxTexture* texU, GfxTexture* texV,  float x0, float y0, float x1, float y1)
{

	const GLuint Prog = GYuvProg.getId();

	glUseProgram(Prog);
	check();

	glUniform2f(glGetUniformLocation(Prog,"offset"),x0,y0);
	glUniform2f(glGetUniformLocation(Prog,"scale"),x1-x0,y1-y0);
	glUniform1i(glGetUniformLocation(Prog,"tex0"), 0);
	glUniform1i(glGetUniformLocation(Prog,"tex1"), 1);
	glUniform1i(glGetUniformLocation(Prog,"tex2"), 2);
	check();

	glBindTexture(GL_TEXTURE_2D,texY->getId());
	glBindTexture(GL_TEXTURE_2D,texU->getId());
	glBindTexture(GL_TEXTURE_2D,texY->getId());
	check();

	GLuint loc = glGetAttribLocation(Prog,"vertex");
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);
	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);
	check();
	glEnableVertexAttribArray(loc);
	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );
	check();

	glFinish();
	check();

	glFlush();
	check();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

void handle_blobs(Blobtree *blob, BlobtreeRect *r, GfxTexture *texture){

	const GLuint Prog = GFillProg.getId();
	glUseProgram(Prog);
	check();

	static const GLfloat blob_positions[] = {
		-1.0f, -1.0f,	1.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};

	BlobtreeRect *roi;
	Node *curNode = blobtree_first(blob);
	int num=0;
	while( curNode != NULL ){
		num++;
		Blob *data = (Blob*)curNode->data;
		roi = &data->roi;
		//int midX     = roi->x + roi->width/2;
		//int midY    = roi->y + roi->height/2;
	/*	
		printf("Blob with id %i: x=%i y=%i w=%i h=%i area=%i, depthlevel=%i\n",data->id,
				roi->x, roi->y,
				roi->width, roi->height,
				data->area,
				data->depth_level
				);
				*/
				

		//Map rect into [-1,1]²
		float x,y,w,h;
		/*	x1 = (roi->x-r->x)/r->width;
			x1 = 2*x1 - 1;
			x2 = (roi->x+roi->width-r->x)/r->width;
			x2 = 2*x2 - 1;
			x1 = -.5f; x2 = .5f;
			y1 = .0f; y2 = .5f;*/
		x = ((float)(roi->x - r->x))/r->width;
		y = ((float)(roi->y - r->y))/r->height;
		w = ((float)(roi->width)) / r->width;
		h = ((float)(roi->height)) / r->height;
		x = 2*x - 1; y = 2*y - 1;

		//x=.5;y=0.0;w=.5;h=.5;//scale of 0.5 maps on length 1.
	//printf("(x,y) .* (%f, %f) +(%f,%f)\n", w,h, x,y);

	glUniform2f(glGetUniformLocation(Prog,"scale"),w,h);
	glUniform2f(glGetUniformLocation(Prog,"offset"),x,y);
	glUniform4f(glGetUniformLocation(Prog,"color"), 1.0f, 0.0f, 0.0f, .5f);
	check();

	GLuint loc = glGetAttribLocation(Prog,"vertex");
	check();

	glBindBuffer(GL_ARRAY_BUFFER, GQuadVertexBuffer);
	//die können ja konstant bleiben.
	//glBufferData(GL_ARRAY_BUFFER, sizeof(blob_positions), blob_positions, GL_STATIC_DRAW);
	check();

	glVertexAttribPointer(loc, 4, GL_FLOAT, 0, 16, 0);
	check();
	glEnableVertexAttribArray(loc);
	check();
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );


		curNode = blobtree_next(blob);
	}

}


