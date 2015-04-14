#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

bool GfxShader::LoadVertexShader(const char* filename)
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
		//printf("Compiled vertex shader %s:\n%s\n", filename, Src);
		printf("Compiled vertex shader %s.\n", filename);
	}

	return true;
}

bool GfxShader::LoadFragmentShader(const char* filename)
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
		//printf("Compiled fragment shader %s:\n%s\n", filename, Src);
		printf("Compiled fragment shader %s.\n", filename);
	}

	return true;
}

bool GfxProgram::Create(GfxShader* vertex_shader, GfxShader* fragment_shader)
{
	VertexShader = vertex_shader;
	FragmentShader = fragment_shader;
	Id = glCreateProgram();
	glAttachShader(Id, VertexShader->GetId());
	glAttachShader(Id, FragmentShader->GetId());
	glLinkProgram(Id);
	check();
	printf("Created program id %d from vs %d and fs %d\n", GetId(), VertexShader->GetId(), FragmentShader->GetId());

	// Prints the information log for a program object
	char log[1024];
	glGetProgramInfoLog(Id,sizeof log,NULL,log);
	printf("%d:program:\n%s\n", Id, log);

	return true;	
}

bool GfxTexture::CreateFromFile(const char *filename)
{
  unsigned error;
  unsigned char* image = NULL;
  size_t width, height;

  error = lodepng_decode32_file(&image, &width, &height, filename);

  if(error){
		printf("decoder error %u: %s\n", error, lodepng_error_text(error));
		return false;
	}
	
	bool ret = CreateRGBA(width,height,image);
  free(image);
	return ret;
}

bool GfxTexture::CreateRGBA(int width, int height, const void* data)
{
	Width = width;
	Height = height;
	glGenTextures(1, &Id);
	check();
	glBindTexture(GL_TEXTURE_2D, Id);
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

bool GfxTexture::CreateGreyScale(int width, int height, const void* data)
{
	Width = width;
	Height = height;
	glGenTextures(1, &Id);
	check();
	glBindTexture(GL_TEXTURE_2D, Id);
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

bool GfxTexture::GenerateFrameBuffer()
{
	//Create a frame buffer that points to this texture
	glGenFramebuffers(1,&FramebufferId);
	check();
	glBindFramebuffer(GL_FRAMEBUFFER,FramebufferId);
	check();
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,Id,0);
	check();
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	check();
	return true;
}

void GfxTexture::SetPixels(const void* data)
{
	glBindTexture(GL_TEXTURE_2D, Id);
	check();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, IsRGBA ? GL_RGBA : GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	check();
	glBindTexture(GL_TEXTURE_2D, 0);
	check();
}

void GfxTexture::SetInterpolation(bool interpol)
{
	glBindTexture(GL_TEXTURE_2D, Id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)(interpol?GL_LINEAR:GL_NEAREST) );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)(interpol?GL_LINEAR:GL_NEAREST) );
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GfxTexture::Save(const char* fname)
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
	tex->id = Id;
	tex->framebufferId = FramebufferId;
	tex->isRGBA = IsRGBA?1:0;
}

//copy metadata from c ctruct to GfxTexture obj.
void GfxTexture::fromRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex){
	Width = tex->width;
	Height = tex->height;
	Id = tex->id;
	FramebufferId = tex->framebufferId;
	IsRGBA = tex->isRGBA?1:0;
}

void SaveFrameBuffer(const char* fname)
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
