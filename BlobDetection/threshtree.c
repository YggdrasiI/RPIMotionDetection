#ifndef THRESHTREE_C
#define THRESHTREE_C

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h> //for memset

#include "threshtree.h"

#include "threshtree_macros.h"


bool threshtree_create_workspace(
        const int w, const int h,
        ThreshtreeWorkspace **pworkspace
        ){

    if( *pworkspace != NULL ){
        //destroy old struct.
        threshtree_destroy_workspace( pworkspace );
    }
    //Now, *pworkspace is NULL

    if( w*h == 0 ) return false;

    ThreshtreeWorkspace *r = malloc( sizeof(ThreshtreeWorkspace) );

    const int max_comp = (w+h)*100;
    r->max_comp = max_comp;
		r->used_comp = 0;
    if( 
            ( r->ids = (int*) malloc( w*h*sizeof(int) ) ) == NULL ||
            ( r->comp_same = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
            ( r->prob_parent = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
#ifdef BLOB_COUNT_PIXEL
            ( r->comp_size = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
#endif
#ifdef BLOB_DIMENSION
            ( r->top_index = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
            ( r->left_index = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
            ( r->right_index = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
            ( r->bottom_index = (int*) malloc( max_comp*sizeof(int) ) ) == NULL || 
						
#endif
						0 ){
        // alloc failed
        threshtree_destroy_workspace( &r );
        return false;
    }

#ifdef BLOB_SUBGRID_CHECK
		r->triangle = NULL;
		r->triangle_len = 0;
#endif

		r->real_ids = NULL;
		r->real_ids_inv = NULL;

    r->blob_id_filtered = NULL;

    *pworkspace=r;
    return true;
}

bool threshtree_realloc_workspace(
        const int max_comp,
        ThreshtreeWorkspace **pworkspace
        ){

    ThreshtreeWorkspace *r = *pworkspace;
    r->max_comp = max_comp;
    if( 
            ( r->comp_same = (int*) realloc(r->comp_same, max_comp*sizeof(int) ) ) == NULL ||
            ( r->prob_parent = (int*) realloc(r->prob_parent, max_comp*sizeof(int) ) ) == NULL ||
#ifdef BLOB_COUNT_PIXEL
            ( r->comp_size = (int*) realloc(r->comp_size, max_comp*sizeof(int) ) ) == NULL ||
#endif
#ifdef BLOB_DIMENSION
            ( r->top_index = (int*) realloc(r->top_index, max_comp*sizeof(int) ) ) == NULL ||
            ( r->left_index = (int*) realloc(r->left_index, max_comp*sizeof(int) ) ) == NULL ||
            ( r->right_index = (int*) realloc(r->right_index, max_comp*sizeof(int) ) ) == NULL ||
            ( r->bottom_index = (int*) realloc(r->bottom_index, max_comp*sizeof(int) ) ) == NULL ||
#endif
						0 ){
			// realloc failed
			VPRINTF("Critical error: Reallocation of workspace failed!\n");
			threshtree_destroy_workspace( pworkspace );
			return false;
		}

    free(r->blob_id_filtered);//omit unnessecary reallocation and omit wrong/low size
		r->blob_id_filtered = NULL;//should be allocated later if needed.

    return true;
}

void threshtree_destroy_workspace(
        ThreshtreeWorkspace **pworkspace 
        ){
		if( *pworkspace == NULL ) return;

    ThreshtreeWorkspace *r = *pworkspace ;
    free(r->ids);
    free(r->comp_same);
    free(r->prob_parent);
#ifdef BLOB_COUNT_PIXEL
    free(r->comp_size);
#endif
#ifdef BLOB_DIMENSION
    free(r->top_index);
    free(r->left_index);
    free(r->right_index);
    free(r->bottom_index);
#endif

#ifdef BLOB_SUBGRID_CHECK
		free(r->triangle);
		r->triangle = NULL;
		r->triangle_len = 0;
#endif

    free(r->real_ids);
    free(r->real_ids_inv);

    free(r->blob_id_filtered);

    free(r);
    *pworkspace = NULL;
}

Tree* find_connection_components(
		const unsigned char *data,
		const int w, const int h,
		const unsigned char thresh,
		Blob** tree_data,
		ThreshtreeWorkspace *workspace )
{
	const BlobtreeRect roi = {0,0,w,h};
	return find_connection_components_coarse(data,w,h,roi,thresh,1,1,tree_data, workspace);
}

 
Tree* find_connection_components_roi(
		const unsigned char *data,
		const int w, const int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		Blob** tree_data,
		ThreshtreeWorkspace *workspace )
{
	return find_connection_components_coarse(data,w,h,roi,thresh,1,1,tree_data, workspace);
}

/* now include the old algo without subchecks ( find_connection_components_coarse ) */
#include "threshtree_old.c"


#ifdef BLOB_SUBGRID_CHECK 
#define STEPHEIGHT stepwidth
Tree* find_connection_components_subcheck(
		const unsigned char *data,
		const int w, const int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		const int stepwidth,
		Blob** tree_data,
		ThreshtreeWorkspace *workspace )
{
/* With region of interrest (roi)
 * and fixed stepwidth.
 * A - stepwidth - B
 * |               |
 * |           stepwidth
 * |               |
 * C ------------- D
 * */
/* Marks of ι Cases:
 *  x - stepwidth, y - stepwidth, swr - (w-1)%x, shr - (h-1)%y
 * <----------------------- w --------------------------------->
 * |        <-- roi.width ------------------------>
 * |        <- roi.width-stepwidth-swr -->
 * | 
 * | | |    A ←x→ B ←x→ … B ←x→ C ←swr+stepwidth→ C
 * | | roi  ↑                   ↑                 ↑
 * h | hei  y                   y                 y
 * | r ght  ↓                   ↓                 ↓
 * | o -    E ←x→ F ←x→ … F ←x→ G ←swr+stepwidth→ H
 * | i ste  ↑                   ↑                 ↑
 * | . pwi  y                   y                 y
 * | h dth  ↓                   ↓                 ↓
 * | e -    E ←x→ F ←x→ … F ←x→ G ←swr+stepwidth→ H
 * | i shr  …                   …                 …
 * | g |    E ←x→ F ←x→ … F ←x→ G ←swr+stepwidth→ H
 * | h      ↑                   ↑                 ↑
 * | t     	1                   1                 1
 * | |      ↓                   ↓                 ↓
 * h |      L ←x→ M ←x→ … M ←x→ N ←swr+stepwidth→ P
 * | |      ↑                   ↑                 ↑
 * | |     	1                   1                 1
 * | |      ↓                   ↓                 ↓
 * h |      L ←x→ M ←x→ … M ←x→ N ←swr+stepwidth→ P
 * |  
 * |  
 * | 
 *
 */
	if( stepwidth < 2){
		return find_connection_components_coarse(data,w,h,roi,thresh,1,1,tree_data, workspace);
	}
//    return find_connection_components_coarse(data,w,h,roi,thresh,stepwidth,stepwidth,tree_data);

	//init
	int r=w-roi.x-roi.width; //right border
  int b=h-roi.y-roi.height; //bottom border
	if( r<0 || b<0 ){
		fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
		*tree_data = NULL;
		return NULL;
	}

	int swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
	int shr = (roi.height-1)%STEPHEIGHT; // remainder of height/stepheight;
	int sh = STEPHEIGHT*w;
	int sh1 = (STEPHEIGHT-1)*w;
//	int sh2 = shr*w;

	int id=-1;//id for next component
	int a1,a2; // for comparation of g(f(x))=a1,a2=g(f(y)) 
	int k; //loop variable

	/* Create pointer to workspace arrays */
	int max_comp = workspace->max_comp; 

	int* ids = workspace->ids; 

	int* comp_same = workspace->comp_same; 
	int* prob_parent = workspace->prob_parent; 
#ifdef BLOB_COUNT_PIXEL
	int* comp_size = workspace->comp_size; 
#endif
#ifdef BLOB_DIMENSION
	int* top_index = workspace->top_index; 
	int* left_index = workspace->left_index;
	int* right_index = workspace->right_index;
	int* bottom_index = workspace->bottom_index;
	int s=roi.x,z=roi.y; //s-spalte, z-zeile
#else
	const int s=-1,z=-1; //Should not be used.
#endif

	/* triangle array store information about the 
	 * evaluation of ids between the grid pixels.
	 * This info is needed to avoid double evaluation
	 * of ids for some pixels, which would be a critical problem.
	 * Let sw = stepwidth, x%sw=0, y%sw=0.
	 *
	 * The image will divided into quads [x,x+sw) x (y-sw,y+sw]
	 * Quads on right and bottom border will expand to image border.
	 * Quads on the top border will notional shrinked to [x,x+sw) x (-1,0].
	 *
	 * 0 - Only the ids of the corners of a quad was evaluated
	 * 1 - Ids for all Pixels over the anti-diagonal and the corners was evaluated.
	 * 2 - All pixels of the quad was examined.
	 *
	 * */
	const int triwidth = (roi.width-1)/stepwidth + 1;
	const size_t triangle_len = (triwidth+1)* ( (roi.height-1)/STEPHEIGHT + 1);
	if( triangle_len  > workspace->triangle_len ){
		free(workspace->triangle);
		workspace->triangle = malloc( triangle_len	* sizeof(unsigned char) );
		if( workspace->triangle == NULL ){
			printf("(threshtree) Critical error: Mem allocation for triangle failed\n");
		}
		workspace->triangle_len = triangle_len;
	}
			
	unsigned char* const triangle = workspace->triangle;
	unsigned char* tri = triangle;
#if VERBOSE > 0
	printf("triwidth: %i\n", triwidth);
#endif

	const unsigned char* const dS = data+w*roi.y+roi.x;
	const unsigned char* dR = dS+roi.width; //Pointer to right border. Update on every line
	//unsigned char* dR2 = swr>0?dR-swr-stepwidth-stepwidth:dR; //cut last indizies.
	const unsigned char* dR2 = dR-swr-stepwidth; //cut last indizies.

	const unsigned char* const dE = dR + (roi.height-1)*w;
  const unsigned char* const dE2 = dE - shr*w;//remove last lines.

	//int i = w*roi.y+roi.x;
	const unsigned char* dPi = dS; // Pointer to data+i 
	int* iPi = ids+(dS-data); // Poiner to ids+i

#if VERBOSE > 0
	//debug: prefill array
	printf("Note: Prefill ids array with -1. This will be removed for VERBOSE=0\n");
	memset(ids,-1, w*h*sizeof(int));
#endif

	/**** A,A'-CASE *****/
	//top, left corner of BlobtreeRect get first id.
	NEW_COMPONENT(-1);
	iPi += stepwidth;
	dPi += stepwidth;

#ifdef BLOB_DIMENSION
	s += stepwidth;
#endif
	++tri;

	/* Split all logic to two subcases:
	 * *(dPi)<=thresh (marked as B,C,…),
	 * *(dPi)>thresh (marked as B',C',…)
	 * to avoid many 'x&&y || x|y==0' checks.
	 * */

/* *tri beschreibt, was in der Zelle rechts davon/darüber passiert ist.*/

	//top border
	for( ;dPi<dR2; ){
		if( *(dPi) > thresh ){
			/**** B-CASE *****/
			if(*(dPi-stepwidth) > thresh ){//same component as left neighbour
				LEFT_CHECK(stepwidth);
				*(tri-1)=0;
			}else{//new component
				SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,0);
				*(tri-1)=2;
			}
		}else{
			/**** B'-CASE *****/
			if(*(dPi-stepwidth) <= thresh ){//same component as left neighbour
				LEFT_CHECK(stepwidth)
				*(tri-1)=0;
			}else{//new component
				SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,0);
				*(tri-1)=2;
			}
		}
		iPi += stepwidth;
		dPi += stepwidth;
#ifdef BLOB_DIMENSION
		s += stepwidth;
#endif
		++tri;
	}

	//now process last, bigger, cell. stepwidth+swr indizies left.
	//SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,swr);
	SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,swr);
	*(tri-1)=2;
	*(tri) = 9;//Dummy, unused value.

	/* Pointer is stepwidth+swr behind currrent element.
	 * Move pointer to 'next' row.*/
	dPi += r+roi.x+sh1+1;
	iPi += r+roi.x+sh1+1;

	dR += sh; // Move right border to next row.
	dR2 += sh;
#ifdef BLOB_DIMENSION
	s=roi.x;
	z += STEPHEIGHT;
#endif	
	++tri;

	//2nd,...,(h-shr)-row
	for( ; dPi<dE2 ; ){

		//left border
		/* 8 Cases for >thresh-Test of 3 positions
		 *    a — b
		 *    |
		 *    x
		 *
		 *    The positions in '—' are already evaluated if a!=b (<=> *(tri-triwidth)==2 )
		 * */
		{
			const unsigned char casenbr = ( *(dPi) > thresh )? \
				( (( *(dPi-sh+stepwidth) <= thresh ) << 1)
				| (( *(dPi-sh) <= thresh ) << 0)): \
				(	(( *(dPi-sh+stepwidth) > thresh ) << 1)
				| (( *(dPi-sh) > thresh ) << 0));

			printf("tri=%i, %i, %i\n", *(tri-triwidth), tri-triangle, triwidth);
			switch( casenbr ){
				case 0:{ /* no differences */
								 TOP_CHECK(STEPHEIGHT, sh);
								 *(tri) = 0;
								 break;
							 }
				case 1:
				case 2:{
								 SUBCHECK_PART2b(dPi,iPi,stepwidth,w,sh,s,z);
								 *(tri) = 1;
								 break;
							 }
				case 3:{ /* a and b at same side of thresh, evaluate pixels between them*/
									 const BlobtreeRect roiVorne = {s,z-16,17,17};
									 printf("Case 3: (%i, %i)\n", z,s);
		debug_print_matrix( ids, w, h, roiVorne, 1, 1);
								 if( *(tri-triwidth)<2 ){
									 SUBCHECK_PART1c(dPi,iPi,stepwidth,w,sh,s,z);
								 }
		debug_print_matrix( ids, w, h, roiVorne, 1, 1);
								 SUBCHECK_PART2b(dPi,iPi,stepwidth,w,sh,s,z);
								 *(tri) = 1;
		debug_print_matrix( ids, w, h, roiVorne, 1, 1);
								 break;
							 }
			}
		}

		iPi += stepwidth;
		dPi += stepwidth;
#ifdef BLOB_DIMENSION
		s += stepwidth;
#endif
		++tri;

		/*inner elements till last colum before dR2 reached.
		 * => Lefthand tests with -stepwidth
		 * 		Righthand tests with +stepwidth
		 */
		for( ; dPi<dR2; ){

			/* Bit order: 
			 * 0 2 3
			 * 1 
			 */
			/*const*/ unsigned char casenbr = ( *(dPi) > thresh )? \
				( (( *(dPi-sh+stepwidth) <= thresh ) << 3)
				| (( *(dPi-sh) <= thresh ) << 2)
				| (( *(dPi-stepwidth) <= thresh ) << 1)
				| (( *(dPi-sh-stepwidth) <= thresh ) << 0)): \
				(	(( *(dPi-sh+stepwidth) > thresh ) << 3)
				| (( *(dPi-sh) > thresh ) << 2)
				| (( *(dPi-stepwidth) > thresh ) << 1)
				| (( *(dPi-sh-stepwidth) > thresh ) << 0));

#if VERBOSE > 1
		debug_print_matrix( ids, w, h, roi, 1, 1);
		printf("F, casenbr: %i, *tri: %i %i %i\n",casenbr, *(tri-1), *(tri-triwidth), *(tri-triwidth+1));
		debug_getline();
#endif
			if( dPi+stepwidth>=dR2 ){
				//set bit do avoid PART1c/PART1d calls for column of last loop step.
				//Could be unrolled...
				casenbr = casenbr | 0x08;
			}
			if( dPi+sh>=dE2 ){
				//set bit do force PART4 calls for this row.
				casenbr = casenbr | 0x02;
			}

			if( *(tri-1) > 1 ){
				switch( casenbr) {
					case 0:
					case 8: {
										TOP_CHECK(STEPHEIGHT, sh);
										TOP_LEFT_COMP(stepwidth);
										*(tri) = 1; //tri filled, quad not.
										break;
									}
					case 1:{
									 SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 2:{
									 SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 3:{
									 SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 4:{
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 5:{
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 6:{
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 7:{
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 9:{
									 SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri) = 3;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 10:{
										SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 11:{
										SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 12:{
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
										SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 13:{
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
										SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 14:{
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 15:{
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri) = 3;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
				}
			}else{
				switch( casenbr ){
					case 0:
					case 8: {
										TOP_CHECK(STEPHEIGHT, sh);
										TOP_LEFT_COMP(stepwidth);
										*(tri) = 0;
										break;
									}
					case 1:{
									 SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 2:{
									 SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 3:{
									 SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 4:{
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 5:{
									 SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 6:{
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 7:{
									 SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 9:{
									 SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
									 *(tri-1) = 3;
									 *(tri) = 1;
									 SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
									 break;
								 }
					case 10:{
										SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri-1) = 3;
										*(tri) = 1;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 11:{
										SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri-1) = 3;
										*(tri) = 1;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 12:{
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri-1) = 3;
										*(tri) = 1;
										SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 13:{
										SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri-1) = 3;
										*(tri) = 1;
										SUBCHECK_PART4b(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 14:{
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri-1) = 3;
										*(tri) = 1;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
					case 15:{
										SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
										SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
										*(tri-1) = 3;
										*(tri) = 1;
										SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
										break;
									}
				}

			}


			dPi += stepwidth;
			iPi += stepwidth;
#ifdef BLOB_DIMENSION
			s += stepwidth;
#endif
			++tri;
		}

#if VERBOSE > 2 
		debug_print_matrix( ids, w, h, roi, 1, 1);
		printf("G *tri: %i %i %i\n", *(tri-1), *(tri-triwidth), *(tri-triwidth+1));
		debug_getline();
#endif

		/* now process last, bigger, cell.
		 * There exists only two cases:
		 * triangle=0 or triangle=1. Top border was always evaluated
		 * and bottom border should evaluated, too.
		 */

		if( *(tri-1) > 1 ){
			SUBCHECK_PART6b(dPi,iPi,stepwidth,w,sh,s,z,swr); 
		}else{
			SUBCHECK_PART6a(dPi,iPi,stepwidth,w,sh,s,z,swr); 
		}
		*(tri) = 2;

		/* Pointer is still on currrent element.
		 * Move pointer to 'next' row.*/
		dPi += r+roi.x+sh1+swr+1; 
		iPi += r+roi.x+sh1+swr+1;
		dR += sh; // Move border indizes to next row.
		dR2 += sh;
#ifdef BLOB_DIMENSION
		s=roi.x;
		z += STEPHEIGHT;
#endif	
		++tri;

#if VERBOSE > 2 
		debug_print_matrix( ids, w, h, roi, 1, 1);
		printf("Z:%i, S:%i, I:%i %i\n",z,s,dPi-dS, iPi-ids-(dS-data) );
		debug_getline();
#endif

	} //row loop

	//correct pointer of last for loop step
	iPi -= sh1;
	dPi -= sh1;
	dR -= sh1;
	dR2 -= sh1;
#ifdef BLOB_DIMENSION
	z -= STEPHEIGHT-1;
#endif

#if VERBOSE > 1
		debug_print_matrix( ids, w, h, roi, 1, 1);
		printf("Z:%i, S:%i, I:%i %i\n",z,s,dPi-dS, iPi-ids-(dS-data) );
		debug_getline();
#endif

	if( dE2 != dE ){
		//Process elementwise till end of ROI reached.
		for( ; dPi<dE ; ){
			SUBCHECK_TOPDIAG(dPi,iPi,stepwidth,w,sh,s,z);
			++dPi; ++iPi;
#ifdef BLOB_DIMENSION
			++s;
#endif
			for( ; dPi<dR-1 ; ){
				SUBCHECK_ALLDIR(dPi,iPi,stepwidth,w,sh,s,z);
				++dPi; ++iPi;
#ifdef BLOB_DIMENSION
				++s;
#endif
			}
			//right border
			SUBCHECK_TOPLEFT(dPi,iPi,stepwidth,w,sh,s,z);
			//move pointer to 'next' row
			dPi += r+roi.x+1; 
			iPi += r+roi.x+1;
#ifdef BLOB_DIMENSION
			s = roi.x;
			++z;
#endif
			dR += w;
			//dR2 += w; //not ness.
			
#if VERBOSE > 1
			debug_print_matrix( ids, w, h, roi, 1, 1);
			printf("Z:%i, S:%i, I:%i %i\n",z,s,dPi-dS, iPi-ids-(dS-data) );
			debug_getline();
#endif
		}
	}//end of if(dE2<dE)



/* end of main algo */

#if VERBOSE > 0 
//printf("Matrix of ids:\n");
//print_matrix(ids,w,h);

//printf("comp_same:\n");
//print_matrix(comp_same, id+1, 1);
debug_print_matrix( ids, w, h, roi, 1, 1);
debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, 0);
if( stepwidth*STEPHEIGHT >1 ){
	debug_print_matrix( ids, w, h, roi, stepwidth, STEPHEIGHT);
	//printf("\n\n");
	debug_print_matrix2( ids, comp_same, w, h, roi, stepwidth, STEPHEIGHT, 0);
}
#endif

/* Postprocessing.
 * Sum up all areas with connecteted ids.
 * Then create nodes and connect them. 
 * If BLOB_DIMENSION is set, detectet
 * maximal limits in [left|right|bottom]_index(*(real_ids+X)).
 * */
int nids = id+1; //number of ids
int tmp_id,tmp_id2, real_ids_size=0,l;
int found;
	free(workspace->real_ids);
	workspace->real_ids = calloc( nids, sizeof(int) ); //store join of ids.
	int* const real_ids = workspace->real_ids;

	free(workspace->real_ids_inv);
	workspace->real_ids_inv = calloc( nids, sizeof(int) ); //store for every id with position in real_id link to it's position.
	int* const real_ids_inv = workspace->real_ids_inv;

#if 0
	for(k=1;k<nids;k++){ // k=1 skip the dummy component id=0

		/* Sei F=comp_same. Wegen F(x)<=x folgt (F wird innerhalb dieser Schleife angepasst!)
		 * F^2 = F^3 = ... = F^*
		 * D.h. um die endgültige id zu finden muss comp_same maximal zweimal aufgerufen werden.
		 * */
		tmp_id = *(comp_same+k); 

#if VERBOSE > 0
		printf("%i: (%i->%i ",k,k,tmp_id);
#endif
		if( tmp_id != k ){
			tmp_id = *(comp_same+tmp_id); 
			*(comp_same+k) = tmp_id; 
#if VERBOSE > 0
			printf("->%i ",tmp_id);
#endif
		}
#if VERBOSE > 0
		printf(")\n");
#endif

		if( tmp_id != k ){

#ifdef BLOB_COUNT_PIXEL
			//move area size to other id.
			*(comp_size+tmp_id) += *(comp_size+k); 
			*(comp_size+k) = 0;
#endif

#ifdef BLOB_DIMENSION
			//update dimension
			if( *( top_index+tmp_id ) > *( top_index+k ) )
				*( top_index+tmp_id ) = *( top_index+k );
			if( *( left_index+tmp_id ) > *( left_index+k ) )
				*( left_index+tmp_id ) = *( left_index+k );
			if( *( right_index+tmp_id ) < *( right_index+k ) )
				*( right_index+tmp_id ) = *( right_index+k );
			if( *( bottom_index+tmp_id ) < *( bottom_index+k ) )
				*( bottom_index+tmp_id ) = *( bottom_index+k );
#endif

		}else{
			//Its a component id of a new area
			*(real_ids+real_ids_size) = tmp_id;
			*(real_ids_inv+tmp_id) = real_ids_size;//inverse function
			real_ids_size++;
		}

	}
#endif

#if 1
	for(k=0;k<nids;k++){
		tmp_id = k;
		tmp_id2 = *(comp_same+tmp_id); 
#if VERBOSE > 0
        printf("%i: (%i->%i) ",k,tmp_id,tmp_id2);
#endif
		while( tmp_id2 != tmp_id ){
			tmp_id = tmp_id2; 
			tmp_id2 = *(comp_same+tmp_id); 
#if VERBOSE > 0
            printf("(%i->%i) ",tmp_id,tmp_id2);
#endif
		}
#if VERBOSE > 0 
printf("\n");
#endif

#ifdef BLOB_COUNT_PIXEL
			//move area size to other id.
			*(comp_size+tmp_id2) += *(comp_size+k); 
			*(comp_size+k) = 0;
#endif


#ifdef BLOB_DIMENSION
			//update dimension
			if( *( top_index+tmp_id2 ) > *( top_index+k ) )
				*( top_index+tmp_id2 ) = *( top_index+k );
			if( *( left_index+tmp_id2 ) > *( left_index+k ) )
				*( left_index+tmp_id2 ) = *( left_index+k );
			if( *( right_index+tmp_id2 ) < *( right_index+k ) )
				*( right_index+tmp_id2 ) = *( right_index+k );
			if( *( bottom_index+tmp_id2 ) < *( bottom_index+k ) )
				*( bottom_index+tmp_id2 ) = *( bottom_index+k );
#endif

		//check if area id already identified as real id
		found = 0;
		for(l=0;l<real_ids_size;l++){
			if( *(real_ids+l) == tmp_id ){
				found = 1;
				break;
			}
		}
		if( !found ){
			*(real_ids+real_ids_size) = tmp_id;
			*(real_ids_inv+tmp_id) = real_ids_size;//inverse function
			real_ids_size++;
		}
	}
#endif


/*
 * Generate tree structure
 */

	/* store for real_ids the index of the node in the tree array */
  int *tree_id_relation = malloc( (real_ids_size+1)*sizeof(int) );

  Node *nodes = malloc( (real_ids_size+1)*sizeof(Node) );
  Blob *blobs = malloc( (real_ids_size+1)*sizeof(Blob) );
  Tree *tree = malloc( sizeof(Tree) );
  tree->root = nodes;
  tree->size = real_ids_size + 1;

	//init all node as leafs
	for(l=0;l<real_ids_size+1;l++) *(nodes+l)=Leaf;

	Node * const root = nodes;
	Node *cur  = nodes;
	Blob *curdata  = blobs;

	curdata->id = -1;
	memcpy( &curdata->roi, &roi, sizeof(BlobtreeRect) );
	curdata->area = roi.width * roi.height;
#ifdef SAVE_DEPTH_MAP_VALUE
	curdata->depth_level = 0; 
#endif
	cur->data = curdata; // link to the data array.

	BlobtreeRect *rect;

	for(l=0;l<real_ids_size;l++){
		cur++;
		curdata++;
		cur->data = curdata; // link to the data array.

		rect = &curdata->roi;
		const int rid = *(real_ids+l);
		curdata->id = rid;	//Set id of this blob.
#ifdef BLOB_DIMENSION
		rect->y = *(top_index + rid);
		rect->height = *(bottom_index + rid) - rect->y + 1;
		rect->x = *(left_index + rid);
		rect->width = *(right_index + rid) - rect->x + 1;
#endif
#ifdef SAVE_DEPTH_MAP_VALUE
		curdata->depth_level = 0; /* ??? without anchor not trivial.*/
#endif

		tmp_id = *(prob_parent+*(real_ids+l)); //get id of parent (or child) area. 
		if( tmp_id < 0 ){
			/* Use root as parent node. */
			//cur->parent = root;
			add_child(root, cur );
		}else{
			//find real id of parent id.
			tmp_id2 = *(comp_same+tmp_id); 
			while( tmp_id != tmp_id2 ){
				tmp_id = tmp_id2; 
				tmp_id2 = *(comp_same+tmp_id); 
			} 
			/*Now, tmp_id is in real_id array. And real_ids_inv is defined. */
			//cur->parent = root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id );
			add_child( root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id ),
					cur );
		}

	}
#ifdef BLOB_COUNT_PIXEL
	//sum up node areas
	if(stepwidth == 1){
		sum_areas(root->child, comp_size);
	}
#endif
#ifdef BLOB_DIMENSION
	if(stepwidth > 1){
		/* comp_size values are not correct/not useable.
		 * Use bounding box as approximation. */
		set_area_prop(root->child);
	}
#endif

#ifdef BLOB_SORT_TREE
	//sort_tree(root->child);
	sort_tree(root);
#endif

	//current id indicates maximal used id in ids-array
	workspace->used_comp=id;

	//clean up
	free(tree_id_relation);
	//free(triangle);
//	free(anchors);

	//set output parameter
    *tree_data = blobs;
	return tree;
}

#undef STEPHEIGHT
#endif



void threshtree_find_blobs( Blobtree *blob, 
		const unsigned char *data, 
		const int w, const int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		ThreshtreeWorkspace *workspace )
{
	//clear old tree
	if( blob->tree != NULL){
		tree_destroy(&blob->tree);
		blob->tree = NULL;
	}
	if( blob->tree_data != NULL){
		free(blob->tree_data);
		blob->tree_data = NULL;
	}
	//get new blob tree structure.
#ifdef BLOB_SUBGRID_CHECK
	blob->tree = find_connection_components_subcheck(
			data, w, h, roi, thresh,
			blob->grid.width,
			&blob->tree_data,
			workspace );
#else
	blob->tree = find_connection_components_coarse(
			data, w, h, roi, thresh,
			blob->grid.width, blob->grid.height,
			&blob->tree_data,
			workspace );
#endif
}


void threshtree_filter_blob_ids(
		Blobtree* blob,
		ThreshtreeWorkspace *pworkspace
		){

	int numNodes = blob->tree->size;
	VPRINTF("Num nodes: %i\n", numNodes);

	if(pworkspace->blob_id_filtered==NULL){
		//Attention, correct size of blob_id_filtered is assumed if != NULL.
		//See workspace reallocation
		pworkspace->blob_id_filtered= (int*) malloc( pworkspace->max_comp*sizeof(int) );
	}
	int * const nodeToFilteredNode = (int*) calloc( numNodes,sizeof(int) );
	int * const blob_id_filtered = pworkspace->blob_id_filtered;
	const int * const comp_same = pworkspace->comp_same;
	const int * const real_ids_inv = pworkspace->real_ids_inv;

	if( nodeToFilteredNode != NULL && blob_id_filtered != NULL ){
		nodeToFilteredNode[0]=0;
		nodeToFilteredNode[1]=1;

		/* 1. Map is identity on filtered nodes.
		 * After this loop all other nodes will be still mapped to 0.
		 * The ±1-shifts are caused by the dummy node on first position.
		 * */
		const Node * const root = blob->tree->root;
		const Node *cur = blobtree_first(blob);
		while( cur != NULL ){
			//const int id = ((Blob*)cur->data)->id;
			//const int node_id = *(pworkspace->real_ids_inv+id) + 1;
			const int node_id = cur-root;
			//note: Both definitions of node_id are equivalent.
			//*(nodeToFilteredNode + node_id) = id;
			*(nodeToFilteredNode + node_id) = node_id;
			cur = blobtree_next(blob);
		}

		// 2. Take all nodes which are mapped to 0 and 
		// search parent node with nonzero mapping.
		// Start for index=i=2 because first node is dummy and second is root.
		int pn, ri; //parent real id, read id of parent node
		for( ri=2; ri<numNodes; ri++){
			if( nodeToFilteredNode[ri] == 0 ){
				//find parent node of 'ri' which was not filtered out
				Node *pi = (blob->tree->root +ri)->parent;
				while( pi != NULL ){
					pn = nodeToFilteredNode[pi-root];
					if( pn != 0 ){ 
						nodeToFilteredNode[ri] = pn;
						break;
					}
					pi = pi->parent;
				}//if no matching element is found, i is mapped to root id (=0).
			}
		}

		/*3. Expand nodeToFilteredNode map information on all ids
		 * 3a)	Use projection (yes, its project now) comp_same to map id
		 * 			on preimage of real_ids_inv. (=> id2)
		 * 3b) Get node for id2. The dummy node produce +1 shift.
		 * 3c) Finally, use nodeToFilteredNode map.
		 */
		int id=pworkspace->used_comp;//dec till 0
		while( id ){
			*(blob_id_filtered+id) = *(nodeToFilteredNode +	*(real_ids_inv + *(comp_same+id)) + 1 );
			id--;
		}
		*(blob_id_filtered+id) = *(nodeToFilteredNode +	*(real_ids_inv + *(comp_same+id)) + 1 );

#if VERBOSE > 1
		printf("nodeToFilteredNode[realid] = realid\n");
		for( ri=0; ri<numNodes; ri++){
			int id = ((Blob*)((blob->tree->root +ri)->data))->id;
			printf("id=%i, nodeToFilteredNode[%i] = %i\n",id, ri, nodeToFilteredNode[ri]);
		}
#endif

		free(nodeToFilteredNode);

	}else{
		printf("(threshtree_filter_blob_ids) Critical error: Mem allocation failed\n");
	}


}



#endif
