#ifndef HELPER_H
#define HELPER_H

#include <cstdio>
#include "GLES2/gl2.h"

int expandVertexData ( 
		const GLfloat *vertex_in,
		const GLint *indizies_in,
		const int indizies_len,
		const GLint vertex_dim,
		GLfloat **vertex_out
		);
void fillVec4( GLfloat **dest, const GLfloat *src, const int L1, const int L2, const int vNr);

void printArray(const GLfloat *v, int num, int vertex_dim );




#endif
