#pragma once

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "GLES2/gl2ext.h"

#include "threshtree.h"

void InitGraphics();
void ReleaseGraphics();
void BeginFrame();
void EndFrame();

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

public:

	GfxTexture() : Width(0), Height(0) {}
	~GfxTexture() {}

	bool Create(int width, int height, const void* data = NULL);
	void SetPixels(const void* data);
	GLuint GetId() { return Id; }
	GLint GetWidth() const { return Width; }
	GLint GetHeight() const { return Height; }
};

void DrawTextureRect(GfxTexture* texture, float x0, float y0, float x1, float y1);
void DrawTextureRectTri(GfxTexture* texture, float x0, float y0, float x1, float y1);

void handle_blobs(Blobtree *blob, BlobtreeRect *r, GfxTexture *texture);
