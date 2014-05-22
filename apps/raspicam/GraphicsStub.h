#ifndef GRAPHICS_STUB_H
#define GRAPHICS_STUB_H

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#ifdef __cplusplus
extern "C" 
{
#endif

void InitGraphics();
void InitShaders();
void ReleaseGraphics();
void BeginFrame();
void EndFrame();

void InitTextures();
void RedrawGui();
void RedrawTextures();
void FooBar();

#ifdef __cplusplus
}
#endif

#endif
