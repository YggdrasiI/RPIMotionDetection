#ifndef GRAPHICS_PONG_H
#define GRAPHICS_PONG_H

//#include "RaspiTexUtil.h"
#include "GfxProgram.h"
//#include "GraphicsStub.h"
//#include "DrawingFunctions.h"
class Pong;

void DrawGui(GfxTexture* scoreTexture, Pong *pong, float border,
        float x0, float y0, float x1, float y1, GfxTexture* render_target);

void DrawPongRect(GfxTexture* texture, float r, float g, float b,
        float x0, float y0, float x1, float y1, GfxTexture* render_target);

#endif
