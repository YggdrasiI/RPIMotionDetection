#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdexcept>
#include <assert.h>
#include <unistd.h>
#include <iostream>

#include "lodepng.h"
#include "GfxProgram.h"

extern uint32_t GScreenWidth;
extern uint32_t GScreenHeight;

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

int _check_gl_error(const char *file, int line) {
	GLenum err (glGetError());
	int ret = (err == 0);
	while(err!=GL_NO_ERROR) {
		std::string error;

		switch(err) {
			case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
		}

		std::cerr << "GL_" << error.c_str() <<" - "<<file<<":"<<line<<std::endl;
		err=glGetError();
	}
	return ret;
}

bool GfxShader::loadVertexShader(const char* filename)
{
	//cheeky bit of code to read the whole file into memory
	assert(!src);
	FILE* f = fopen(filename, "rb");
	assert(f);
	fseek(f,0,SEEK_END);
	int sz = ftell(f);
	fseek(f,0,SEEK_SET);
	src = new GLchar[sz+1];
	fread(src,1,sz,f);
	src[sz] = 0; //null terminate it!
	fclose(f);

	//now create and compile the shader
	glShaderType = GL_VERTEX_SHADER;
	id = glCreateShader(glShaderType);
	glShaderSource(id, 1, (const GLchar**)&src, 0);
	glCompileShader(id);
	check();

	//compilation check
	GLint compiled;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
	if(compiled==0)
	{
		printf("Failed to compile vertex shader %s:\n%s\n", filename, src);
		printShaderInfoLog(id);
		glDeleteShader(id);
		return false;
	}
	else
	{
		//printf("Compiled vertex shader %s:\n%s\n", filename, src);
		printf("Compiled vertex shader %s.\n", filename);
	}

	return true;
}

bool GfxShader::loadFragmentShader(const char* filename)
{
	//cheeky bit of code to read the whole file into memory
	assert(!src);
	FILE* f = fopen(filename, "rb");
	assert(f);
	fseek(f,0,SEEK_END);
	int sz = ftell(f);
	fseek(f,0,SEEK_SET);
	src = new GLchar[sz+1];
	fread(src,1,sz,f);
	src[sz] = 0; //null terminate it!
	fclose(f);

	//now create and compile the shader
	glShaderType = GL_FRAGMENT_SHADER;
	id = glCreateShader(glShaderType);
	glShaderSource(id, 1, (const GLchar**)&src, 0);
	glCompileShader(id);
	check();

	//compilation check
	GLint compiled;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
	if(compiled==0)
	{
		printf("Failed to compile fragment shader %s:\n%s\n", filename, src);
		printShaderInfoLog(id);
		glDeleteShader(id);
		return false;
	}
	else
	{
		//printf("Compiled fragment shader %s:\n%s\n", filename, src);
		printf("Compiled fragment shader %s.\n", filename);
	}

	return true;
}

bool GfxProgram::create(GfxShader* vertex_shader, GfxShader* fragment_shader)
{
	vertexShader = vertex_shader;
	fragmentShader = fragment_shader;
	id = glCreateProgram();
	glAttachShader(id, vertexShader->getId());
	glAttachShader(id, fragmentShader->getId());
	glLinkProgram(id);
	check();
	printf("Created program id %d from vs %d and fs %d\n", getId(), vertexShader->getId(), fragmentShader->getId());

	// Prints the information log for a program object
	char log[1024];
	glGetProgramInfoLog(id,sizeof log,NULL,log);
	printf("%d:program:\n%s\n", id, log);

	return true;	
}

bool GfxTexture::createFromFile(const char *filename)
{
  unsigned error;
  unsigned char* image = NULL;
  size_t width, height;

  error = lodepng_decode32_file(&image, &width, &height, filename);

  if(error){
		printf("decoder error %u: %s\n", error, lodepng_error_text(error));
		return false;
	}
	
	bool ret = createRGBA(width,height,image);
  free(image);
	return ret;
}

bool GfxTexture::createRGBA(int width, int height, const void* data)
{
	Width = width;
	Height = height;
	glGenTextures(1, &id);
	check();
	glBindTexture(GL_TEXTURE_2D, id);
	check();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	check();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
	check();
	glBindTexture(GL_TEXTURE_2D, 0);
	IsRGBA = true;
	return true;
}

bool GfxTexture::createGreyScale(int width, int height, const void* data)
{
	Width = width;
	Height = height;
	glGenTextures(1, &id);
	check();
	glBindTexture(GL_TEXTURE_2D, id);
	check();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, Width, Height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	check();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
	check();
	glBindTexture(GL_TEXTURE_2D, 0);
	IsRGBA = false;
	return true;
}

bool GfxTexture::generateFramebuffer()
{
	//Create a frame buffer that points to this texture
	glGenFramebuffers(1,&FramebufferId);
	check();
	glBindFramebuffer(GL_FRAMEBUFFER,FramebufferId);
	check();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,id,0);
	check();
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	check();
	return true;
}

void GfxTexture::setPixels(const void* data)
{
	glBindTexture(GL_TEXTURE_2D, id);
	check();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, IsRGBA ? GL_RGBA : GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	check();
	glBindTexture(GL_TEXTURE_2D, 0);
	check();
}

void GfxTexture::setInterpolation(bool interpol)
{
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)(interpol?GL_LINEAR:GL_NEAREST) );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)(interpol?GL_LINEAR:GL_NEAREST) );
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GfxTexture::save(const char* fname)
{
	void* image = malloc(Width*Height*4);
	glBindFramebuffer(GL_FRAMEBUFFER,FramebufferId);
	check();
	glReadPixels(0,0,Width,Height,IsRGBA ? GL_RGBA : GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
	check();
	glBindFramebuffer(GL_FRAMEBUFFER,0);

	unsigned error = lodepng::encode(fname, (const unsigned char*)image, Width, Height, IsRGBA ? LCT_RGBA : LCT_GREY);
	if(error) 
		printf("error: %d\n",error);

	free(image);
}

//copy metadata from GfxTexture obj to c struct
void GfxTexture::toRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex){
	tex->width = Width;
	tex->height = Height;
	tex->id = id;
	tex->framebufferId = FramebufferId;
	tex->isRGBA = IsRGBA?1:0;
}

//copy metadata from c ctruct to GfxTexture obj.
void GfxTexture::fromRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex){
	Width = tex->width;
	Height = tex->height;
	id = tex->id;
	FramebufferId = tex->framebufferId;
	IsRGBA = tex->isRGBA?1:0;
}

void saveFramebuffer(const char* fname)
{
	//uint32_t GScreenWidth;
	//uint32_t GScreenHeight;
	//graphics_get_display_size(0 /* LCD */, &GScreenWidth, &GScreenHeight);
	void* image = malloc(GScreenWidth*GScreenHeight*4);
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	check();
	glReadPixels(0,0,GScreenWidth,GScreenHeight, GL_RGBA, GL_UNSIGNED_BYTE, image);

	unsigned error = lodepng::encode(fname, (const unsigned char*)image, GScreenWidth, GScreenHeight, LCT_RGBA);
	if(error) 
		printf("error: %d\n",error);

	free(image);

}

GLuint GfxProgram::getAttribLocation( const char *name){
	unsigned int name_hash = hash(name);
	try {
		return handles.at(name_hash);
	}
	catch (const std::out_of_range& oor) {
		GLuint id = glGetAttribLocation(getId(), name);
		handles[name_hash] = id;
		return id;
	}
}

GLuint GfxProgram::getAttribLocation( std::string name){
	unsigned int name_hash = hash(name.c_str(), name.length());
	try {
		return handles.at(name_hash);
	}
	catch (const std::out_of_range& oor) {
		GLuint id = glGetAttribLocation(getId(), name.c_str());
		handles[name_hash] = id;
		return id;
	}
}

GLuint GfxProgram::getAttribLocation( unsigned int name_hash){
	try {
		return handles.at(name_hash);
	}
	catch (const std::out_of_range& oor) {
		std::cerr << "Handle not defined. Out of Range error: " << oor.what() << '\n';
		return -1;
	}
}
GLuint GfxProgram::getUniformLocation( const char *name){
	unsigned int name_hash = hash(name);
	try {
		return handles.at(name_hash+1);
	}
	catch (const std::out_of_range& oor) {
		GLuint id = glGetUniformLocation(getId(), name);
		handles[name_hash+1] = id;
		return id;
	}
}

GLuint GfxProgram::getUniformLocation( std::string name){
	unsigned int name_hash = hash(name.c_str(), name.length());
	try {
		return handles.at(name_hash+1);
	}
	catch (const std::out_of_range& oor) {
		GLuint id = glGetUniformLocation(getId(), name.c_str());
		handles[name_hash+1] = id;
		return id;
	}
}

GLuint GfxProgram::getUniformLocation( unsigned int name_hash){
	try {
		return handles.at(name_hash+1);
	}
	catch (const std::out_of_range& oor) {
		std::cerr << "Handle not defined. Out of Range error: " << oor.what() << '\n';
		return -1;
	}
}

static unsigned int hash( const char *name, size_t len){
	if(len == 0){
		len = strlen(name);
	}
	uint32_t hash, i;
	for(hash = i = 0; i < len; ++i)
	{
		hash += name[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;

}
