#ifndef GRAPHICS_H
#define GRAPHICS_H

/* Source raspicam_gpu */

#include "RaspiTexUtil.h"

#include "GfxProgram.h"
#include "GraphicsStub.h"
#include "DrawingFunctions.h"
#include "Pong.h"

void DrawGui(GfxTexture* scoreTexture, Pong *pong, float border, float x0, float y0, float x1, float y1, GfxTexture* render_target);

extern GfxTexture imvTexture;
extern GfxTexture raspiTexture;

#endif
