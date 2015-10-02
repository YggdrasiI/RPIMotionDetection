/* Helper functions to draw tracker objects
 * on OpenGL target.
 */
#ifndef TRACKER_DRAWING_OPENGL_H
#define TRACKER_DRAWING_OPENGL_H

#include <stdlib.h>
#include <vector>
#include "Blob.h"

#ifdef WITH_OPENGL
class GfxTexture; 

/* Helper function to draw blobs for debugging */
void tracker_drawBlobsGL(Tracker &tracker, int screenWidth, int screenHeight, bool drawHistoryLines =     false, std::vector<cBlob> *toDraw = NULL, GfxTexture *target = NULL);

#ifdef WITH_HISTORY
void tracker_drawHistory( Tracker &tracker, int screenWidth, int screenHeight, cBlob &blob, GfxTexture     *target);
#endif

#endif

#endif
