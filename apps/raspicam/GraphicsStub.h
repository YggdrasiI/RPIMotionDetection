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

void InitTextures(uint32_t glWinWidth, uint32_t glWinHeight);
void RedrawGui();
void RedrawTextures();

#ifdef __cplusplus
}
#endif

#endif
