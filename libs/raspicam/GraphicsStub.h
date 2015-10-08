#ifndef GRAPHICS_STUB_H
#define GRAPHICS_STUB_H

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"


#ifdef __cplusplus
extern "C" 
{
#endif

	//For Pong app
enum SHADER_TYPE {
ShaderNormal,
ShaderSobel,
ShaderHSV,
ShaderMirror,
SHADER_TYPE_NUM,
};

int /*SHADER_TYPE*/ GetShader(int option);

void InitGraphics();
void InitShaders();
void ReleaseGraphics();

void InitTextures(uint32_t glWinWidth, uint32_t glWinHeight);
void RedrawGui();
void RedrawTextures();

#ifdef __cplusplus
}
#endif

#endif
