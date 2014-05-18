/*
 Um wireframes darzustellen wird ein Shader verwendet, der für jeden Knoten eines
 Dreiecks die Koordinaten der beiden anderen Knoten des Dreiecks benötigt.
 Daher werden den Vertex-Koordinaten die Koordinaten der benachbarten Knoten
 nachgestellt und dann im Vertexshader geladen. 
 Achtung, dafür ist es auch notwendig, Knoten, die in mehreren Dreiecken vorkommen,
 zu duplizieren. D.h. Die Datenmenge steigt.
 Achtung, der Speicher muss dann manuell freigegeben werden.

vertex_in : Koordinaten der Knoten
indizies_in : Angabe der Knotenindizies eines Dreiecks. Länge muss Vielfaches von 3 sein.
indizies_len: Länge des indizes-Arrays in Bytes(!)
vertex_dim : Dimension der Eingabeknoten (3 oder 4). Ausgabedimension ist immer 4.

vertex_out: Expandiertes Array.

Beispiel:  ([v0, v1, v2], [0,1,2])  wird zu
[v0,
	v1,
	v2,
 v1,
	v2,
	v0,
 v2,
	v0,
	v1]
, wobei die Gleichheit nur die ersten drei Dimensionen gilt. Die vierte wird intern
	vom Shader genutzt.

@return: size of vertex_out.
*/

#include <cstdlib>
#include <cstring>
#include "helper.h"

int expandVertexData( 
		const GLfloat *vertex_in,
		const GLint *indizies_in,
		const int indizies_len,
		const GLint vertex_dim,
		GLfloat **vertex_out
		){
	const int L1 = (vertex_dim<0)?0:(vertex_dim<5?vertex_dim:4);
	const int L2 = 4-L1;

	int tri_len = indizies_len/3/sizeof(GLfloat);
	if( *vertex_out != NULL ){
		free( *vertex_out );
	}

	const int vertex_len = tri_len*9*4*sizeof(GLfloat);
	*vertex_out = (GLfloat*) malloc( vertex_len );

	const GLint *tri_end = indizies_in+3*tri_len;
	const GLint *tri = indizies_in;

	GLfloat *v_out = *vertex_out;
	for( ;  tri<tri_end ; ){
		const GLfloat *a = vertex_in+(L1 * *tri++);
		const GLfloat *b = vertex_in+(L1 * *tri++);
		const GLfloat *c = vertex_in+(L1 * *tri++);
		// a,[b,c]
		fillVec4( &v_out, a, L1, L2, 0);
		fillVec4( &v_out, b, L1, L2, 1);//die Vierte Komponente der benachbarten Knoten immer auf 1 setzen.
		fillVec4( &v_out, c, L1, L2, 1);//die Vierte Komponente der benachbarten Knoten immer auf 1 setzen.
		// b,[c,a]
		fillVec4( &v_out, b, L1, L2, 1);
		fillVec4( &v_out, c, L1, L2, 1);
		fillVec4( &v_out, a, L1, L2, 1);

		// c,[a,b]
		fillVec4( &v_out, c, L1, L2, 2);
		fillVec4( &v_out, a, L1, L2, 1);
		fillVec4( &v_out, b, L1, L2, 1);
	}

	return vertex_len;
}

void fillVec4( GLfloat **dest, const GLfloat *src, const int L1, const int L2, const int vNr){
		memcpy( *dest, src, L1 * sizeof(GLfloat) ); 
		*dest += L1;
		memset( *dest, 0, L2 * sizeof(GLfloat) );
		*dest += L2;
		*((*dest)-1) = (float)vNr;
}

void printArray(const GLfloat *v, int num, int vertex_dim ){
	num = num/sizeof(GLfloat); 
printf("Array with %i Nodes:\n", num/vertex_dim);
	for(int i=0;i<num; ++i){
		if( i%vertex_dim  ==  1 ){
			printf("  ");
		}
		printf("%f, ", v[i]);
		if( i%vertex_dim  == vertex_dim - 1 ){
			printf("\n");
		}
	}
}
