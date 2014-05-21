#ifndef GRAPHICS_H
#define GRAPHICS_H

/* Source raspicam_gpu */

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "lodepng.h"

#include "GraphicsStub.h"
#include "Pong.h"

class GfxShader
{
	GLchar* Src;
	GLuint Id;
	GLuint GlShaderType;

public:

	GfxShader() : Src(NULL), Id(0), GlShaderType(0) {}
	~GfxShader() { if(Src) delete[] Src; }

	bool LoadVertexShader(const char* filename);
	bool LoadFragmentShader(const char* filename);
	GLuint GetId() { return Id; }
};

class GfxProgram
{
	GfxShader* VertexShader;
	GfxShader* FragmentShader;
	GLuint Id;

public:

	GfxProgram() {}
	~GfxProgram() {}

	bool Create(GfxShader* vertex_shader, GfxShader* fragment_shader);
	GLuint GetId() { return Id; }
};

class GfxTexture
{
	int Width;
	int Height;
	GLuint Id;
	bool IsRGBA;

	GLuint FramebufferId;
public:

	GfxTexture() : Width(0), Height(0), Id(0), FramebufferId(0) {}
	~GfxTexture() {}

	bool CreateFromFile(const char *filename);
	bool CreateRGBA(int width, int height, const void* data = NULL);
	bool CreateGreyScale(int width, int height, const void* data = NULL);
	bool GenerateFrameBuffer();
	void SetPixels(const void* data);
	void SetInterpolation(bool interpol);
	void Save(const char* fname);
	GLuint GetId() { return Id; }
	GLuint GetFramebufferId() { return FramebufferId; }
	int GetWidth() {return Width;}
	int GetHeight() {return Height;}
};


void SaveFrameBuffer(const char* fname);
void DrawTextureRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawBlobRect(float r, float g, float b, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawGUI(GfxTexture* scoreTexture, Pong *pong, float border, float x0, float y0, float x1, float y1, GfxTexture* render_target);


extern GfxTexture imvTexture;

#endif
