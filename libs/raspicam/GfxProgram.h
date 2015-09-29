#ifndef GFXPROGRAM_H
#define GFXPROGRAM_H

#include <map>

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "RaspiTexUtil.h"

#define check() assert(glGetError() == 0)

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

//string hash function for handles map.
static unsigned int hash( const char *name, size_t len = 0);

class GfxProgram
{
	GfxShader* VertexShader;
	GfxShader* FragmentShader;
	GLuint Id;

	//The following map uses string hashes as key.
	//This allows access easy by str:string, const char, preprocessor hash, ...),
	//see overloaded GetHandle() function.
	std::map<unsigned int, GLint> handles; // cache GetAttributeLocation values

public:

	GfxProgram() {}
	~GfxProgram() {}

	bool Create(GfxShader* vertex_shader, GfxShader* fragment_shader);
	GLuint GetId() { return Id; }
	GLint GetHandle( const char *name);
	GLint GetHandle( std::string name);
	GLint GetHandle( unsigned int name_hash);
};

class GfxTexture
{
	int Width;
	int Height;
	GLuint Id;
	GLuint FramebufferId;
	bool IsRGBA;

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
	void toRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex);
	void fromRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex);
};

void printShaderInfoLog(GLint shader);
void SaveFrameBuffer(const char* fname);



#endif
