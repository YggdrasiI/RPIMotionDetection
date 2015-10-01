#ifndef GFXPROGRAM_H
#define GFXPROGRAM_H

#include <string>
#include <map>

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "RaspiTexUtil.h"

#define check() assert(glGetError() == 0)

int _check_gl_error(const char *file, int line);
#define check_gl_error() assert(_check_gl_error(__FILE__,__LINE__))

class GfxShader
{
	GLchar* src;
	GLuint id;
	GLuint glShaderType;

public:

	GfxShader() : src(NULL), id(0), glShaderType(0) {}
	~GfxShader() { if(src) delete[] src; }

	bool loadVertexShader(const char* filename);
	bool loadFragmentShader(const char* filename);
	GLuint getId() { return id; }
};

//string hash function for handles map.
static unsigned int hash( const char *name, size_t len = 0);

class GfxProgram
{
	GfxShader* vertexShader;
	GfxShader* fragmentShader;
	GLuint id;

	//The following map uses string hashes as key.
	//This allows access easy by str:string, const char, preprocessor hash, ...),
	//see overloaded getAttribLocation(), getUniformLocation functions.
	std::map<unsigned int, GLuint> handles; // cache glGet*Location values

public:

	GfxProgram() {}
	~GfxProgram() {}

	bool create(GfxShader* vertex_shader, GfxShader* fragment_shader);
	GLuint getId() { return id; }
	GLuint getAttribLocation( const char *name);
	GLuint getAttribLocation( std::string name);
	GLuint getAttribLocation( unsigned int name_hash);
	GLuint getUniformLocation( const char *name);
	GLuint getUniformLocation( std::string name);
	GLuint getUniformLocation( unsigned int name_hash);
};

class GfxTexture
{
	int Width;
	int Height;
	GLuint id;
	GLuint FramebufferId;
	bool IsRGBA;

public:

	GfxTexture() : Width(0), Height(0), id(0), FramebufferId(0) {}
	~GfxTexture() {}

	bool createFromFile(const char *filename);
	bool createRGBA(int width, int height, const void* data = NULL);
	bool createGreyScale(int width, int height, const void* data = NULL);
	bool generateFramebuffer();
	void setPixels(const void* data);
	void setInterpolation(bool interpol);
	void save(const char* fname);
	GLuint getId() { return id; }
	GLuint getFramebufferId() { return FramebufferId; }
	int getWidth() {return Width;}
	int getHeight() {return Height;}
	void toRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex);
	void fromRaspiTexture(RASPITEXUTIL_TEXTURE_T *tex);
};

void printShaderInfoLog(GLint shader);
void saveFramebuffer(const char* fname);



#endif
