#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "time.h"

#include "tree.h"
#include "blob.h"

static char * c( char i){
	switch(i){
		case 0: return " ";
		case 1: return "□";
		case 2: return "■";
		case 3: return "⬛";
		case 4: return "█";
		case 5: return "░";
		case 6: return "✘";
		default: return " ";
	}
}

void print_matrix_with_roi( unsigned int* data, unsigned int w, unsigned int h, BlobtreeRect roi){
	unsigned int i,j,d;
	for(i=roi.y;i<roi.height;i++){
		for(j=roi.x;j<roi.width;j++){
			d = *(data+i*w+j);
			printf("%s%u ",d<10?" ":"",d);
		}
		printf("\n");
	}
	printf("\n");
}

void print_matrix( unsigned int* data, unsigned int w, unsigned int h){
	BlobtreeRect roi = {0,0,w,h};
	print_matrix_with_roi(data,w,h,roi);
}

void print_matrix_char_with_roi( char* data, unsigned int w, unsigned int h, BlobtreeRect roi, unsigned int gridw, unsigned int gridh){
	unsigned int i,j, wr, hr, w2, h2;
	unsigned int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			d = *(data+i*w+j);
			//printf("%u ",d);
			//printf("%s", d==0?"■⬛":"□");
			//printf("%s", d==0?"✘":" ");
			printf("%s", d!=0?"█":"░");
			//printf("%s", c(d));
		}
		j-=gridw-wr;

		if( wr>0 ){
			d = *(data+i*w+j);
			printf("%s", c(d));
		}
		printf("\n");
	}

	i-=gridh-hr;
	if( hr>0 ){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			d = *(data+i*w+j);
			printf("%s", c(d));
		}
		j-=gridw-wr;

		if( wr>0 ){
			d = *(data+i*w+j);
			printf("%s", c(d));
		}
		printf("\n");
	}
}

void print_matrix_char( char* data, unsigned int w, unsigned int h){
	BlobtreeRect roi = {0,0,w,h};
	print_matrix_char_with_roi(data,w,h,roi,1,1);
}


void test(Node* root, unsigned int tree_size){
	/* Allocate space for 'name' of tree */
	unsigned int* tree_name = malloc( tree_size*sizeof(unsigned int) );

	gen_tree_id(root->child, tree_name, tree_size);

	unsigned int l;
	for( l=0;l<tree_size;l++)
		printf("%u ", *(tree_name+l) );
	printf("\n");

	free(tree_name);
}


void gen_image_data(unsigned char* sw, unsigned int w, unsigned int h){

	srand( time(NULL) );
	unsigned int wh=w*h,i;
	for( i=0;i<w+1;i++){
		*(sw+i) = 0;
		*(sw+wh-1-i) = 0;
	}

	for( i=w+1;i<wh-w-1;i++)
		if( i%w==0 || i%w==w-1)
			*(sw+i) = 0;
		else	
			*(sw+i) = random()%2;
}

void gen_image_data2(unsigned char* sw, unsigned int w, unsigned int h, unsigned int depth){
	//srand( time(NULL) );
	unsigned int wh=w*h,i,j,k,l,d,col;
	unsigned int tx,ty;
	//B == Block
	unsigned int Bbreite = 2*depth+1;
	unsigned int Bw = w/Bbreite;
	unsigned int Bh = h/Bbreite;

	//durchlaufe alle Blöcke
	for( i=0;i<Bh;i++ ){
		for( j=0;j<Bw;j++ ){
			col = 1;
			for( d=0;d<depth+1;d++ ){
				col = 200*(random()%2);
				//col = 1-col;
				//von außen nach innen einen Block mit col füllen
				for(k=d;k<Bbreite-d;k++){
					for(l=d;l<Bbreite-d;l++){
						tx = i*Bbreite + k;
						ty = j*Bbreite + l;
						*(sw+ tx*h +ty) = col;
					}
				}
			}	
		}
	}

	//Rand auf Null
	for( i=0;i<w+1;i++){
		*(sw+i) = 0;
		*(sw+wh-1-i) = 0;
	}
	for( i=w+1;i<wh-w-1;i++) if( i%w==0 || i%w==w-1) *(sw+i) = 0;
}

void gen_image_data3(unsigned char* sw, unsigned int w, unsigned int h, unsigned int depth){
	//srand( time(NULL) );
	unsigned int wh=w*h,i,j,k,l,d,col;
	unsigned int tx,ty;
	//B == Block
	unsigned int Bbreite = 2*depth+1;
	unsigned int Bw = w/Bbreite;
	unsigned int Bh = h/Bbreite;

	//durchlaufe alle Blöcke
	for( i=0;i<Bh;i++ ){
		for( j=0;j<Bw;j++ ){
			col = 1;
			for( d=0;d<depth+1;d++ ){
				col = (random()%5);
				//col = 1-col;
				//von außen nach innen einen Block mit col füllen
				for(k=d;k<Bbreite-d;k++){
					for(l=d;l<Bbreite-d;l++){
						tx = i*Bbreite + k;
						ty = j*Bbreite + l;
						*(sw+ tx*h +ty) = col;
					}
				}
			}	
		}
	}

	//Rand auf Null
	for( i=0;i<w+1;i++){
		*(sw+i) = 0;
		*(sw+wh-1-i) = 0;
	}
	for( i=w+1;i<wh-w-1;i++) if( i%w==0 || i%w==w-1) *(sw+i) = 0;
}


#endif
