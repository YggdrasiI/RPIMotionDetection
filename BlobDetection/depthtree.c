#ifndef DEPTHTREE_C
#define DEPTHTREE_C

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h> //for memset

#include "blob.h"
#include "depthtree.h"

#include "depthtree_macros.h"

bool depthtree_create_workspace(
        const int w, const int h,
        DepthtreeWorkspace **pworkspace
        ){

    if( *pworkspace != NULL ){
        //destroy old struct.
        depthtree_destroy_workspace( pworkspace );
    }
    //Now, *pworkspace is NULL

    if( w*h == 0 ) return false;

    DepthtreeWorkspace *r = malloc( sizeof(DepthtreeWorkspace) );

    const int max_comp = (w+h)*100;
    r->max_comp = max_comp;
		r->used_comp = 0;

    if( 
            ( r->ids = (int*) malloc( w*h*sizeof(int) ) ) == NULL ||
#ifndef NO_DEPTH_MAP
//            ( r->depths = (int*) malloc( w*h*sizeof(int) ) ) == NULL ||
            ( r->depths = (unsigned char*) calloc( w*h,sizeof(unsigned char) ) ) == NULL ||
#endif
            ( r->id_depth = (int*) malloc( max_comp*sizeof(int) ) ) == NULL ||
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
            ( r->a_ids = (int*) malloc( 255*sizeof(int) ) ) == NULL || 
            ( r->b_ids = (int*) malloc( 255*sizeof(int) ) ) == NULL || 
            ( r->c_ids = (int*) malloc( 255*sizeof(int) ) ) == NULL || 
            ( r->d_ids = (int*) malloc( 255*sizeof(int) ) ) == NULL || 
            ( r->a_dep = (unsigned char*) malloc( 255*sizeof(unsigned char) ) ) == NULL || 
            ( r->b_dep = (unsigned char*) malloc( 255*sizeof(unsigned char) ) ) == NULL || 
            ( r->c_dep = (unsigned char*) malloc( 255*sizeof(unsigned char) ) ) == NULL || 
            ( r->d_dep = (unsigned char*) malloc( 255*sizeof(unsigned char) ) ) == NULL  
      ){
        // alloc failed
        depthtree_destroy_workspace( &r );
        return false;
    }

    /* setup first entry of *_ids and *_dep to 0->255. (0 is dummy entry with
     * depth 255).
     * The dummy entry will guarantee the existance of a child element in every
     * case. */
    r->a_ids[0] = 0; r->a_dep[0] = 255;
    r->b_ids[0] = 0; r->b_dep[0] = 255;
    r->c_ids[0] = 0; r->c_dep[0] = 255;
    r->d_ids[0] = 0; r->d_dep[0] = 255;

		r->real_ids = NULL;
		r->real_ids_inv = NULL;

    r->blob_id_filtered = NULL;

    *pworkspace=r;
    return true;
}

bool depthtree_realloc_workspace(
        const int max_comp,
        DepthtreeWorkspace **pworkspace
        ){

    DepthtreeWorkspace *r = *pworkspace;
    r->max_comp = max_comp;
    if( 
            ( r->id_depth = (int*) realloc(r->id_depth, max_comp*sizeof(int) ) ) == NULL ||
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
			depthtree_destroy_workspace( pworkspace );
			return false;
		}

    free(r->blob_id_filtered);//omit unnessecary reallocation and omit wrong/low size
		r->blob_id_filtered = NULL;//should be allocated later if needed.

    return true;
}

void depthtree_destroy_workspace(
        DepthtreeWorkspace **pworkspace 
        ){
		if( *pworkspace == NULL ) return;

    DepthtreeWorkspace *r = *pworkspace ;
    free(r->ids);
#ifndef NO_DEPTH_MAP
    free(r->depths);
#endif
    free(r->id_depth);
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
    free(r->a_ids);
    free(r->b_ids);
    free(r->c_ids);
    free(r->d_ids);
    free(r->a_dep);
    free(r->b_dep);
    free(r->c_dep);
    free(r->d_dep);

    free(r->real_ids);
    free(r->real_ids_inv);

    free(r->blob_id_filtered);

    free(r);
    *pworkspace = NULL;
}

Tree* find_depthtree(
		const unsigned char *data,
		const int w, const int h,
		const BlobtreeRect roi,
		const unsigned char *depth_map,
		const int stepwidth,
		DepthtreeWorkspace *workspace,
        Blob** tree_data )
{

//#define stepwidth 7 //speed up due faster addition for fixed stepwidth?!
#define stepheight stepwidth
    /* Marks of 10 Cases:
     *  x - stepwidth, y - stepheight, swr - (w-1)%x, shr - (h-1)%y
     * <----------------------- w --------------------------------->
     * |        <-- roi.width -------------->
     * |        <-- roi.width-swr -->
     * | 
     * | | |    A ←x→ B ←x→ … B ←x→ B ←swr→ C
     * | | roi  ↑                   ↑       ↑
     * h | hei  y                   y       y
     * | r ght  ↓                   ↓       ↓
     * | o -    E ←x→ F ←x→ … F ←x→ G ←swr→ H
     * | i shr  ↑                   ↑       ↑
     * | . |    y                   y       y
     * | h |    ↓                   ↓       ↓
     * | e |    E ←x→ F ←x→ … F ←x→ G ←swr→ H
     * | i |    …                   …       …
     * | g |    E ←x→ F ←x→ … F ←x→ G ←swr→ H
     * | h      ↑                   ↑       ↑
     * | t     shr                 shr     shr
     * | |      ↓                   ↓       ↓
     * h |      L ←x→ M ←x→ … M ←x→ N ←swr→ P
     * |  
     * |  
     * | 
     *
     */
    //init
    int const r=w-roi.x-roi.width; //right border
    int const b=h-roi.y-roi.height; //bottom border
    if( r<0 || b<0 ){
        fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
        *tree_data = NULL;
        return NULL;
    }

    int const swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
    int const shr = (roi.height-1)%stepheight; // remainder of height/stepheight;
    int const sh = stepheight*w;
    int const sh1 = (stepheight-1)*w;
    int const sh2 = shr*w;

    int id=-1;//id for next component
    int k; //loop variable
    int max_comp = workspace->max_comp; 
    int idA, idB; 
    unsigned char depX; 

    int* ids = workspace->ids; 
    int* comp_same = workspace->comp_same; 
    int* prob_parent = workspace->prob_parent; 
    int* id_depth = workspace->id_depth; 
#ifdef BLOB_COUNT_PIXEL
    int* comp_size = workspace->comp_size; 
#endif
#ifdef BLOB_DIMENSION
    int* top_index = workspace->top_index; 
    int* left_index = workspace->left_index;
    int* right_index = workspace->right_index;
    int* bottom_index = workspace->bottom_index;
    int s=roi.x,z=roi.y; //s-spalte, z-zeile
#endif
	int *a_ids, *b_ids/*, *c_ids, *d_ids*/;
	unsigned char *a_dep, *b_dep/*, *c_dep, *d_dep*/;  


#ifdef NO_DEPTH_MAP
    /* map depth on data array */
    unsigned char * const depths = data;
#else
    unsigned char * const depths = workspace->depths;
#endif

    unsigned char * const depS = depths+w*roi.y+roi.x;
    const unsigned char* const dS = data+(depS-depths);

    const unsigned char* depR = depS+roi.width; //Pointer to right border. Update on every line
    const unsigned char* depR2 = depR-swr; //cut last indizies.

    const unsigned char* const depE = depR + (roi.height-1)*w;
    const unsigned char* const depE2 = depE - shr*w;//remove last lines.

    int* iPi = ids+(dS-data); // Poiner to ids+i
    unsigned char *depPi = depS;



    /* Eval depth(roi) aka  *(depth_map+i) for i∈roi 
     * */
#ifndef NO_DEPTH_MAP
    const unsigned char* dPi = dS; // Pointer to data+i 

    for( ; depPi<depE2 ;  ){
        for( ; depPi<depR2 ; dPi += stepwidth, depPi += stepwidth ){
            *depPi = *(depth_map + *dPi);
        } 

        //handle last index of current line
        dPi -= stepwidth-swr;
        depPi -= stepwidth-swr;
        if( swr ){
            *depPi = *(depth_map + *dPi);
        }

        //move pointer to 'next' row
        dPi += r+roi.x+sh1+1;
        depPi += r+roi.x+sh1+1;
        depR += sh; //rechter Randindex wird in nächste Zeile geschoben.
        depR2 += sh;
    }

    //handle last line & last element
    if( shr ){
        dPi -= sh-sh2;
        depPi -= sh-sh2;
        depR -= sh-sh2;
        depR2 -= sh-sh2;

        for( ; depPi<depR2 ; dPi += stepwidth, depPi += stepwidth ){
            *depPi = *(depth_map + *dPi);
        } 

        //handle last index of current line
        dPi -= stepwidth-swr;
        depPi -= stepwidth-swr;
        if( swr ){
            *depPi = *(depth_map + *dPi);
        }
    }

    //reset pointer;
    depPi = depths+(dS-data);
    depR = depS+roi.width; 
    depR2 = depR-swr; //cut last indizies.

#if VERBOSE > 0
    printf("Depth matix(roi): \n");
    debug_print_matrix_char( depths, w, h, roi, 1, 1);
#endif
#else
#endif

    /* Dummy for foreground (id=0, depth=255). It's important to set id=0 for
     * this component. */
    NEW_COMPONENT(1, 255 );
    /* Dummy for background (id=1, depth=0) */
    NEW_COMPONENT(-1, 0 );

#ifdef BLOB_COUNT_PIXEL
    /* Set size of dummy component to 0. */
    *(comp_size+0) = 0;
#endif



    /**** A,A'-CASE *****/
    //top, left corner of BlobtreeRect get first id (=2).
    depX = *depPi;
    if( depX>0 ){
        /* Set size of dummy component to 0. */
        *(comp_size+1) = 0;
        NEW_COMPONENT(1, depX );
    }else{
        //use dummy component id=1 for corner element. This avoid wrapping of
        //all blobs.
        *iPi = 1;
    }

    iPi += stepwidth;
    depPi += stepwidth;
#ifdef BLOB_DIMENSION
    s += stepwidth;
#endif

    //top border
    /**** B-CASE *****/
    for( ; depPi<depR2; iPi += stepwidth, depPi += stepwidth ){
        idA = *(iPi-stepwidth);
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);

#ifdef BLOB_DIMENSION
        s += stepwidth;
#endif
    }

    //correct pointer shift of last for loop step.
    iPi -= stepwidth-swr;
    depPi -= stepwidth-swr;
#ifdef BLOB_DIMENSION
    s -= stepwidth-swr;
#endif

    //continue with +swr stepwidth
    if(swr){
        /**** C-CASE *****/
        idA = *(iPi-swr);
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);
    }

    //move pointer to 'next' row
    depPi += r+roi.x+sh1+1;
    iPi += r+roi.x+sh1+1;
    depR += sh; //rechter Randindex wird in nächste Zeile geschoben.
    depR2 += sh;
#ifdef BLOB_DIMENSION
    s=roi.x;
    z += stepheight;
#endif	

    //2nd,...,(h-shr)-row
    for( ; depPi<depE2 ; depPi += r+roi.x+sh1+1, iPi += r+roi.x+sh1+1, depR += sh, depR2 += sh ){

        //left border
        /**** E-CASE *****/
        idA = ARGMAX2( *(iPi-sh), *(iPi-sh+stepwidth), *(depPi-sh), *(depPi-sh+stepwidth) );
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);

        iPi += stepwidth;
        depPi += stepwidth;
#ifdef BLOB_DIMENSION
        s += stepwidth;
#endif

        /*inner elements till last colum before depR2 reached.
         * => Lefthand tests with -stepwidth
         * 		Righthand tests with +stepwidth
         */
        for( ; depPi<depR2-stepwidth; iPi += stepwidth, depPi += stepwidth ){
            /**** F-CASE *****/

        idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
                    *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
        idB = ARGMAX2( *(iPi-sh), *(iPi-sh+stepwidth),
                    *(depPi-sh), *(depPi-sh+stepwidth) ); /* max(b,c) */
        depX = *depPi;
        INSERT_ELEMENT2( idA, idB, iPi, *depPi);
#ifdef BLOB_DIMENSION
            s += stepwidth;
#endif
        }


        /* If depR2==depR, then the last column is reached. Thus it's not possibe
         * to check diagonal element. (only G case)
         * Otherwise use G and H cases. 
         */
        if( swr /*depR2!=depR*/ ){
            //structure: (depPi-stepwidth),(depPi),(depPi+swr)
            /**** G-CASE *****/
            idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = ARGMAX2( *(iPi-sh), *(iPi-sh+swr),
                        *(depPi-sh), *(depPi-sh+swr) ); /* max(b,c) */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

            iPi+=swr;
            depPi+=swr;
#ifdef BLOB_DIMENSION
            s+=swr;
#endif

            //right border, not check diag element
            /**** H-CASE *****/
            idA = ARGMAX2( *(iPi-sh-swr), *(iPi-swr),
                        *(depPi-sh-swr), *(depPi-swr) ); /* max(a,d) */
            idB = *(iPi-sh); /* b */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }else{
            /**** G-CASE *****/
            idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
                    *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = *(iPi-sh); /* b */ 
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }//end of else case of (depR2!=depR)

#ifdef BLOB_DIMENSION
        s=roi.x;
        z += stepheight;
#endif

    } //row for loop

    iPi -= sh-sh2;//(stepheight-1)*w;
    depPi -= sh-sh2;//(stepheight-1)*w;
    depR -= sh-sh2;
    depR2 -= sh-sh2;
#ifdef BLOB_DIMENSION
    z -= stepheight-shr;
#endif

    if( shr /*dE2<dE*/ ){

        //left border
        /**** L-CASE *****/
        idA = ARGMAX2( *(iPi-sh2), *(iPi-sh2+stepwidth), *(depPi-sh2), *(depPi-sh2+stepwidth) );
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);

        iPi += stepwidth;
        depPi += stepwidth;
#ifdef BLOB_DIMENSION
        s += stepwidth;
#endif

        /*inner elements till last colum before depR2 reached.
         * => Lefthand tests with -stepwidth
         * 		Righthand tests with +stepwidth
         */
        for( ; depPi<depR2-stepwidth; iPi += stepwidth, depPi += stepwidth ){
            /**** M-CASE *****/
            idA = ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = ARGMAX2( *(iPi-sh2), *(iPi-sh2+stepwidth),
                        *(depPi-sh2), *(depPi-sh2+stepwidth) ); /* max(b,c) */

            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

#ifdef BLOB_DIMENSION
            s += stepwidth;
#endif
        }

        /* If depR2==depR, then the last column is reached. Thus it's not possibe
         * to check diagonal element. (only N case)
         * Otherwise use N and P cases. 
         */
        if( swr/*depR2!=depR*/ ){
            //structure: (depPi-stepwidth),(depPi),(depPi+swr)
            /**** N-CASE *****/
            idA = ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = ARGMAX2( *(iPi-sh2), *(iPi-sh2+swr),
                        *(depPi-sh2), *(depPi-sh2+swr) ); /* max(b,c) */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

            iPi+=swr;
            depPi+=swr;
#ifdef BLOB_DIMENSION
            s+=swr;
#endif

            //right border, not check diag element
            /**** P-CASE *****/
            idA = ARGMAX2( *(iPi-sh2-swr), *(iPi-swr),
                        *(depPi-sh2-swr), *(depPi-swr) ); /* max(a,d) */
            idB = *(iPi-sh2); /* b */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }else{
            /**** N-CASE *****/
            idA =ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */ 
            idB = *(iPi-sh2); /* b */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }//end of else case of (depR2!=depR)

    } //end of if(depE2<depE)

    /* end of main algo */

#if VERBOSE > 0 
    //printf("Matrix of ids:\n");
    //print_matrix(ids,w,h);

    //printf("comp_same:\n");
    //print_matrix(comp_same, id+1, 1);
    debug_print_matrix( ids, w, h, roi, 1, 1);
    debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, true);
#endif

    /* Postprocessing.
     * Sum up all areas with connecteted ids.
     * Then create nodes and connect them. 
     * If BLOB_DIMENSION is set, detectet
     * maximal limits in [left|right|bottom]_index(*(real_ids+X)).
     * */
    int nids = id+1; //number of ids
    int tmp_id,tmp_id2, real_ids_size=0,l;
    //int* real_ids = calloc( nids,sizeof(int) ); //store join of ids.
    //int* real_ids_inv = calloc( nids,sizeof(int) ); //store for every id with position in real_id link to it's position.
		free(workspace->real_ids);
    workspace->real_ids = calloc( nids, sizeof(int) ); //store join of ids.
		int* const real_ids = workspace->real_ids;

		free(workspace->real_ids_inv);
    workspace->real_ids_inv = calloc( nids, sizeof(int) ); //store for every id with position in real_id link to it's position.
		int* const real_ids_inv = workspace->real_ids_inv;

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

    //set root node (the desired output are the child(ren) of this node.)
    Node * const root = nodes;
    Node *cur  = nodes;
    Blob *curdata  = blobs;

		/* Set root node which represents the whole image/ROI.
		 * Keep in mind, that the number of children depends 
		 * on the border pixels of the ROI.
		 * Almost in every cases it's only one child, but not always.
		 * */
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
			curdata->depth_level = *(id_depth + rid );
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
				add_child( root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id ),
						cur );
			}

		}

    //sum up node areas
#ifdef BLOB_COUNT_PIXEL
#if VERBOSE > 1 
    int ci;
    printf("comp_size Array:\n");
    for( ci=0 ; ci<nids; ci++){
	 printf("cs[%i]=%i\n",ci, *(comp_size + *(real_ids+ci) ) );
    }
#endif
    sum_areas(root->child, comp_size);
#endif

    /* If no pixel has depth=0, the dummy component with id=1
     * wrapping all blobs. In this case we could remove this
     * blob from the tree.
     * */
    if( *(comp_size+1)==0 ){
        root->child = root->child->child;
    }

#ifdef BLOB_DIMENSION 
#ifdef EXTEND_BOUNDING_BOXES
		extend_bounding_boxes( tree );
#endif
#endif

#ifdef BLOB_SORT_TREE
    //sort_tree(root->child);
    sort_tree(root);
#endif

		//current id indicates maximal used id in ids-array
		workspace->used_comp=id;

    //clean up
    free(tree_id_relation);

    //free(real_ids);
    //free(real_ids_inv);

    //set output parameter
    //*tree_size = real_ids_size+1;
    *tree_data = blobs;
    return tree;
}


void depthtree_find_blobs(Blobtree *blob, const unsigned char *data, const int w, const int h, const BlobtreeRect roi, const unsigned char *depth_map, DepthtreeWorkspace *workspace ){
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
	if( blob->grid.height == 11 ){
    blob->tree = find_depthtree11(data, w, h, roi, depth_map, 
				workspace, &blob->tree_data);
	}else{
    blob->tree = find_depthtree(data, w, h, roi, depth_map, 
				blob->grid.width,
				workspace, &blob->tree_data);
	}
}


void depthtree_filter_blob_ids(
		Blobtree* blob,
		DepthtreeWorkspace *pworkspace
		){

	int numNodes = blob->tree->size;
	VPRINTF("Num nodes: %i\n", numNodes);

	if(pworkspace->blob_id_filtered==NULL){
		//Attention, correct size of blob_id_filtered is assumed if != NULL.
		//See workspace reallocation
		pworkspace->blob_id_filtered= (int*) malloc( pworkspace->max_comp*sizeof(int) );
	}
	int * const bif = (int*) calloc( numNodes,sizeof(int) );
	int * const blob_id_filtered = pworkspace->blob_id_filtered;
	const int * const comp_same = pworkspace->comp_same;
	const int * const real_ids_inv = pworkspace->real_ids_inv;

	if( bif != NULL && blob_id_filtered != NULL ){
		bif[0]=0;
		bif[1]=1;

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
			//*(bif + node_id) = id;
			*(bif + node_id) = node_id;
			cur = blobtree_next(blob);
		}

		// 2. Take all nodes which are mapped to 0 and 
		// search parent node with nonzero mapping.
		// Start for index=i=2 because first node is dummy and second is root.
		int pn, ri; //parent real id, read id of parent node
		for( ri=2; ri<numNodes; ri++){
			if( bif[ri] == 0 ){
				//find parent node of 'ri' which was not filtered out
				Node *pi = (blob->tree->root +ri)->parent;
				while( pi != NULL ){
					pn = bif[pi-root];
					if( pn != 0 ){ 
						bif[ri] = pn;
						break;
					}
					pi = pi->parent;
				}//if no matching element is found, i is mapped to root id (=0).
			}
		}

		/*3. Expand bif map information on all ids
		 * 3a)	Use projection (yes, its project now) comp_same to map id
		 * 			on preimage of real_ids_inv. (=> id2)
		 * 3b) Get node for id2. The dummy node produce +1 shift.
		 * 3c) Finally, use bif map.
		 */
		int id=pworkspace->used_comp;//dec till 0
		while( id ){
			*(blob_id_filtered+id) = *(bif +	*(real_ids_inv + *(comp_same+id)) + 1 );
			id--;
		}

#if VERBOSE > 0
		printf("bif[realid] = realid\n");
		for( ri=0; ri<numNodes; ri++){
			int id = ((Blob*)((blob->tree->root +ri)->data))->id;
			printf("id=%i, bif[%i] = %i\n",id, ri, bif[ri]);
		}
#endif

		free(bif);

	}else{
		printf("(depthtree_filter_blob_ids) Critical error: Mem allocation failed\n");
	}


}



#ifdef EXTEND_BOUNDING_BOXES
void extend_bounding_boxes( Tree * const tree){

	Node* root = tree->root;
	Node* cur = tree->root;
	Node* child;

	while( cur != NULL ){
		if( cur->child != NULL ) cur = cur->child;
		else if( cur->silbing != NULL ) cur = cur->silbing;
		else{
			while(cur != root && cur->silbing == NULL){
				cur = cur->parent;

				//here, all children are handled. Compare properties of cur with child properties
				//the (x,y,width,height)-Format complicates the comparation.
				BlobtreeRect *dcur = &((Blob*)cur->data)->roi;
				child = cur->child;
				while( child != NULL ){
					BlobtreeRect *dchild = &((Blob*)child->data)->roi;
					int w = dchild->width;
					int h = dchild->height;
					int a = (dchild->x - dcur->x);
					int b = (dchild->y - dcur->y);
					if( a<0 ){ dcur->x += a; dcur->width -= a; }else{ w += a; }
					if( dcur->width<w ){ dcur->width=w; }
					if( b<0 ){ dcur->y += b; dcur->height -= b; }else{ h += b; }
					if( dcur->height<h ){ dcur->height=h; }

					VPRINTF("Compare parent node %i with %i \n", ((Blob*)cur->data)->id, ((Blob*)child->data)->id   );

					child = child->silbing;
				}

			}
			cur = cur->silbing;
		}
	}

}
#endif



//TEST
Tree* find_depthtree11(
		const unsigned char *data,
		const int w, const int h,
		const BlobtreeRect roi,
		const unsigned char *depth_map,
		DepthtreeWorkspace *workspace,
        Blob** tree_data )
{

#define stepwidth 1 
#define stepheight stepwidth
    /* Marks of 10 Cases:
     *  x - stepwidth, y - stepheight, swr - (w-1)%x, shr - (h-1)%y
     * <----------------------- w --------------------------------->
     * |        <-- roi.width -------------->
     * |        <-- roi.width-swr -->
     * | 
     * | | |    A ←x→ B ←x→ … B ←x→ B ←swr→ C
     * | | roi  ↑                   ↑       ↑
     * h | hei  y                   y       y
     * | r ght  ↓                   ↓       ↓
     * | o -    E ←x→ F ←x→ … F ←x→ G ←swr→ H
     * | i shr  ↑                   ↑       ↑
     * | . |    y                   y       y
     * | h |    ↓                   ↓       ↓
     * | e |    E ←x→ F ←x→ … F ←x→ G ←swr→ H
     * | i |    …                   …       …
     * | g |    E ←x→ F ←x→ … F ←x→ G ←swr→ H
     * | h      ↑                   ↑       ↑
     * | t     shr                 shr     shr
     * | |      ↓                   ↓       ↓
     * h |      L ←x→ M ←x→ … M ←x→ N ←swr→ P
     * |  
     * |  
     * | 
     *
     */
    //init
    int const r=w-roi.x-roi.width; //right border
    int const b=h-roi.y-roi.height; //bottom border
    if( r<0 || b<0 ){
        fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
        *tree_data = NULL;
        return NULL;
    }

    int const swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
    int const shr = (roi.height-1)%stepheight; // remainder of height/stepheight;
    int const sh = stepheight*w;
    int const sh1 = (stepheight-1)*w;
    int const sh2 = shr*w;

    int id=-1;//id for next component
    int k; //loop variable
    int max_comp = workspace->max_comp; 
    int idA, idB; 
    unsigned char depX; 

    int* ids = workspace->ids; 
    int* comp_same = workspace->comp_same; 
    int* prob_parent = workspace->prob_parent; 
    int* id_depth = workspace->id_depth; 
#ifdef BLOB_COUNT_PIXEL
    int* comp_size = workspace->comp_size; 
#endif
#ifdef BLOB_DIMENSION
    int* top_index = workspace->top_index; 
    int* left_index = workspace->left_index;
    int* right_index = workspace->right_index;
    int* bottom_index = workspace->bottom_index;
    int s=roi.x,z=roi.y; //s-spalte, z-zeile
#endif
	int *a_ids, *b_ids/*, *c_ids, *d_ids*/;
	unsigned char *a_dep, *b_dep/*, *c_dep, *d_dep*/;  


#ifdef NO_DEPTH_MAP
    /* map depth on data array */
    unsigned char * const depths = data;
#else
    unsigned char * const depths = workspace->depths;
#endif

    unsigned char * const depS = depths+w*roi.y+roi.x;
    const unsigned char* const dS = data+(depS-depths);

    const unsigned char* depR = depS+roi.width; //Pointer to right border. Update on every line
    const unsigned char* depR2 = depR-swr; //cut last indizies.

    const unsigned char* const depE = depR + (roi.height-1)*w;
    const unsigned char* const depE2 = depE - shr*w;//remove last lines.

    int* iPi = ids+(dS-data); // Poiner to ids+i
    unsigned char *depPi = depS;



    /* Eval depth(roi) aka  *(depth_map+i) for i∈roi 
     * */
#ifndef NO_DEPTH_MAP
    const unsigned char* dPi = dS; // Pointer to data+i 

    for( ; depPi<depE2 ;  ){
        for( ; depPi<depR2 ; dPi += stepwidth, depPi += stepwidth ){
            *depPi = *(depth_map + *dPi);
        } 

        //handle last index of current line
        dPi -= stepwidth-swr;
        depPi -= stepwidth-swr;
        if( swr ){
            *depPi = *(depth_map + *dPi);
        }

        //move pointer to 'next' row
        dPi += r+roi.x+sh1+1;
        depPi += r+roi.x+sh1+1;
        depR += sh; //rechter Randindex wird in nächste Zeile geschoben.
        depR2 += sh;
    }

    //handle last line & last element
    if( shr ){
        dPi -= sh-sh2;
        depPi -= sh-sh2;
        depR -= sh-sh2;
        depR2 -= sh-sh2;

        for( ; depPi<depR2 ; dPi += stepwidth, depPi += stepwidth ){
            *depPi = *(depth_map + *dPi);
        } 

        //handle last index of current line
        dPi -= stepwidth-swr;
        depPi -= stepwidth-swr;
        if( swr ){
            *depPi = *(depth_map + *dPi);
        }
    }

    //reset pointer;
    depPi = depths+(dS-data);
    depR = depS+roi.width; 
    depR2 = depR-swr; //cut last indizies.

#if VERBOSE > 0
    printf("Depth matix(roi): \n");
    debug_print_matrix_char( depths, w, h, roi, 1, 1);
#endif
#else
#endif

    /* Dummy for foreground (id=0, depth=255). It's important to set id=0 for
     * this component. */
    NEW_COMPONENT(1, 255 );
    /* Dummy for background (id=1, depth=0) */
    NEW_COMPONENT(-1, 0 );

#ifdef BLOB_COUNT_PIXEL
    /* Set size of dummy component to 0. */
    *(comp_size+0) = 0;
#endif



    /**** A,A'-CASE *****/
    //top, left corner of BlobtreeRect get first id (=2).
    depX = *depPi;
    if( depX>0 ){
        /* Set size of dummy component to 0. */
        *(comp_size+1) = 0;
        NEW_COMPONENT(1, depX );
    }else{
        //use dummy component id=1 for corner element. This avoid wrapping of
        //all blobs.
        *iPi = 1;
    }

    iPi += stepwidth;
    depPi += stepwidth;
#ifdef BLOB_DIMENSION
    s += stepwidth;
#endif

    //top border
    /**** B-CASE *****/
    for( ; depPi<depR2; iPi += stepwidth, depPi += stepwidth ){
        idA = *(iPi-stepwidth);
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);

#ifdef BLOB_DIMENSION
        s += stepwidth;
#endif
    }

    //correct pointer shift of last for loop step.
    iPi -= stepwidth-swr;
    depPi -= stepwidth-swr;
#ifdef BLOB_DIMENSION
    s -= stepwidth-swr;
#endif

    //continue with +swr stepwidth
    if(swr){
        /**** C-CASE *****/
        idA = *(iPi-swr);
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);
    }

    //move pointer to 'next' row
    depPi += r+roi.x+sh1+1;
    iPi += r+roi.x+sh1+1;
    depR += sh; //rechter Randindex wird in nächste Zeile geschoben.
    depR2 += sh;
#ifdef BLOB_DIMENSION
    s=roi.x;
    z += stepheight;
#endif	

    //2nd,...,(h-shr)-row
    for( ; depPi<depE2 ; depPi += r+roi.x+sh1+1, iPi += r+roi.x+sh1+1, depR += sh, depR2 += sh ){

        //left border
        /**** E-CASE *****/
        idA = ARGMAX2( *(iPi-sh), *(iPi-sh+stepwidth), *(depPi-sh), *(depPi-sh+stepwidth) );
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);

        iPi += stepwidth;
        depPi += stepwidth;
#ifdef BLOB_DIMENSION
        s += stepwidth;
#endif

        /*inner elements till last colum before depR2 reached.
         * => Lefthand tests with -stepwidth
         * 		Righthand tests with +stepwidth
         */
        for( ; depPi<depR2-stepwidth; iPi += stepwidth, depPi += stepwidth ){
            /**** F-CASE *****/

        idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
                    *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
        idB = ARGMAX2( *(iPi-sh), *(iPi-sh+stepwidth),
                    *(depPi-sh), *(depPi-sh+stepwidth) ); /* max(b,c) */
        depX = *depPi;
        INSERT_ELEMENT2( idA, idB, iPi, *depPi);
#ifdef BLOB_DIMENSION
            s += stepwidth;
#endif
        }


        /* If depR2==depR, then the last column is reached. Thus it's not possibe
         * to check diagonal element. (only G case)
         * Otherwise use G and H cases. 
         */
        if( swr /*depR2!=depR*/ ){
            //structure: (depPi-stepwidth),(depPi),(depPi+swr)
            /**** G-CASE *****/
            idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = ARGMAX2( *(iPi-sh), *(iPi-sh+swr),
                        *(depPi-sh), *(depPi-sh+swr) ); /* max(b,c) */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

            iPi+=swr;
            depPi+=swr;
#ifdef BLOB_DIMENSION
            s+=swr;
#endif

            //right border, not check diag element
            /**** H-CASE *****/
            idA = ARGMAX2( *(iPi-sh-swr), *(iPi-swr),
                        *(depPi-sh-swr), *(depPi-swr) ); /* max(a,d) */
            idB = *(iPi-sh); /* b */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }else{
            /**** G-CASE *****/
            idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
                    *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = *(iPi-sh); /* b */ 
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }//end of else case of (depR2!=depR)

#ifdef BLOB_DIMENSION
        s=roi.x;
        z += stepheight;
#endif

    } //row for loop

    iPi -= sh-sh2;//(stepheight-1)*w;
    depPi -= sh-sh2;//(stepheight-1)*w;
    depR -= sh-sh2;
    depR2 -= sh-sh2;
#ifdef BLOB_DIMENSION
    z -= stepheight-shr;
#endif

    if( shr /*dE2<dE*/ ){

        //left border
        /**** L-CASE *****/
        idA = ARGMAX2( *(iPi-sh2), *(iPi-sh2+stepwidth), *(depPi-sh2), *(depPi-sh2+stepwidth) );
        depX = *depPi;
        INSERT_ELEMENT1( idA, iPi, *depPi);

        iPi += stepwidth;
        depPi += stepwidth;
#ifdef BLOB_DIMENSION
        s += stepwidth;
#endif

        /*inner elements till last colum before depR2 reached.
         * => Lefthand tests with -stepwidth
         * 		Righthand tests with +stepwidth
         */
        for( ; depPi<depR2-stepwidth; iPi += stepwidth, depPi += stepwidth ){
            /**** M-CASE *****/
            idA = ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = ARGMAX2( *(iPi-sh2), *(iPi-sh2+stepwidth),
                        *(depPi-sh2), *(depPi-sh2+stepwidth) ); /* max(b,c) */

            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

#ifdef BLOB_DIMENSION
            s += stepwidth;
#endif
        }

        /* If depR2==depR, then the last column is reached. Thus it's not possibe
         * to check diagonal element. (only N case)
         * Otherwise use N and P cases. 
         */
        if( swr/*depR2!=depR*/ ){
            //structure: (depPi-stepwidth),(depPi),(depPi+swr)
            /**** N-CASE *****/
            idA = ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
            idB = ARGMAX2( *(iPi-sh2), *(iPi-sh2+swr),
                        *(depPi-sh2), *(depPi-sh2+swr) ); /* max(b,c) */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

            iPi+=swr;
            depPi+=swr;
#ifdef BLOB_DIMENSION
            s+=swr;
#endif

            //right border, not check diag element
            /**** P-CASE *****/
            idA = ARGMAX2( *(iPi-sh2-swr), *(iPi-swr),
                        *(depPi-sh2-swr), *(depPi-swr) ); /* max(a,d) */
            idB = *(iPi-sh2); /* b */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }else{
            /**** N-CASE *****/
            idA =ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
                        *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */ 
            idB = *(iPi-sh2); /* b */
            depX = *depPi;
            INSERT_ELEMENT2( idA, idB, iPi, *depPi);

        }//end of else case of (depR2!=depR)

    } //end of if(depE2<depE)

    /* end of main algo */

#if VERBOSE > 0 
    //printf("Matrix of ids:\n");
    //print_matrix(ids,w,h);

    //printf("comp_same:\n");
    //print_matrix(comp_same, id+1, 1);
    debug_print_matrix( ids, w, h, roi, 1, 1);
    debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, true);
#endif

    /* Postprocessing.
     * Sum up all areas with connecteted ids.
     * Then create nodes and connect them. 
     * If BLOB_DIMENSION is set, detectet
     * maximal limits in [left|right|bottom]_index(*(real_ids+X)).
     * */
    int nids = id+1; //number of ids
    int tmp_id,tmp_id2, real_ids_size=0,l;
    //int* real_ids = calloc( nids,sizeof(int) ); //store join of ids.
    //int* real_ids_inv = calloc( nids,sizeof(int) ); //store for every id with position in real_id link to it's position.
		free(workspace->real_ids);
    workspace->real_ids = calloc( nids, sizeof(int) ); //store join of ids.
		int* const real_ids = workspace->real_ids;

		free(workspace->real_ids_inv);
    workspace->real_ids_inv = calloc( nids, sizeof(int) ); //store for every id with position in real_id link to it's position.
		int* const real_ids_inv = workspace->real_ids_inv;

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

    //set root node (the desired output are the child(ren) of this node.)
    Node * const root = nodes;
    Node *cur  = nodes;
    Blob *curdata  = blobs;

		/* Set root node which represents the whole image/ROI.
		 * Keep in mind, that the number of children depends 
		 * on the border pixels of the ROI.
		 * Almost in every cases it's only one child, but not always.
		 * */
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
			curdata->depth_level = *(id_depth + rid );
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
				add_child( root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id ),
						cur );
			}

		}

    //sum up node areas
#ifdef BLOB_COUNT_PIXEL
#if VERBOSE > 1 
    int ci;
    printf("comp_size Array:\n");
    for( ci=0 ; ci<nids; ci++){
	 printf("cs[%i]=%i\n",ci, *(comp_size + *(real_ids+ci) ) );
    }
#endif
    sum_areas(root->child, comp_size);
#endif

    /* If no pixel has depth=0, the dummy component with id=1
     * wrapping all blobs. In this case we could remove this
     * blob from the tree.
     * */
    if( *(comp_size+1)==0 ){
        root->child = root->child->child;
    }

#ifdef BLOB_DIMENSION 
#ifdef EXTEND_BOUNDING_BOXES
		extend_bounding_boxes( tree );
#endif
#endif

#ifdef BLOB_SORT_TREE
    //sort_tree(root->child);
    sort_tree(root);
#endif

		//current id indicates maximal used id in ids-array
		workspace->used_comp=id;

    //clean up
    free(tree_id_relation);

    //free(real_ids);
    //free(real_ids_inv);

    //set output parameter
    //*tree_size = real_ids_size+1;
    *tree_data = blobs;
    return tree;
}








#endif
