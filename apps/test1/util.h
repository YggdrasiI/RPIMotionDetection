/* Utilfunctions copied from https://github.com/blackberry/OpenGLES-Samples/blob/master/OpenGLES2-ProgrammingGuide/Common/src/ */
#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <math.h>

#define PI 3.1415926535897932384626433832795f


typedef struct
{
    GLfloat m[4][4];
} ESMatrix;

void 
esScale(ESMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz);

void
esTranslate(ESMatrix *result, GLfloat tx, GLfloat ty, GLfloat tz);

void
esRotate(ESMatrix *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);

void
esFrustum(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);

void
esPerspective(ESMatrix *result, float fovy, float aspect, float nearZ, float farZ);

void
esOrtho(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);

void
esMatrixMultiply(ESMatrix *result, ESMatrix *srcA, ESMatrix *srcB);

void
esMatrixLoadIdentity(ESMatrix *result);

#endif
