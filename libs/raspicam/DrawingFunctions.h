#ifndef DRAWING_FUNCTIONS_H
#define DRAWING_FUNCTIONS_H

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "GfxProgram.h"

void DrawTextureRect(GfxTexture* texture, float alpha, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawBlobRect(float r, float g, float b, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawBlobRects(GLfloat *vertices, GLfloat *colors, GLfloat numRects, GfxTexture* render_target);
void DrawColouredLines(GLfloat *vertices, GLfloat *colors, GLfloat numPoints, GfxTexture* render_target);


void DrawPongRect(GfxTexture* texture, float r, float g, float b,
		float x0, float y0, float x1, float y1, GfxTexture* render_target);


#endif
