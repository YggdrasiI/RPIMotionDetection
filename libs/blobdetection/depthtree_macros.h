#ifndef DEPTHTREE_MACROS
#define DEPTHTREE_MACROS

//#define DEPTHPRINTF(...) VPRINTF(__VA_ARGS__)
#define DEPTHPRINTF(...) 

/*
 *
 * Notes:
 */

#define	ARGMAX2( x, y, fx, fy) ( (fx) > (fy) )?(x):(y)

#define ARGMAX4( x, y, z, u, fx, fy, fz, fu) \
	( (fx) > (fy) )? \
( \
	(fz) > (fu)?( \
		ARGMAX2(x,z,fx,fz) \
		):( \
			ARGMAX2(x,u,fx,fu) \
			) \
):( \
	(fz) > (fu)?( \
		ARGMAX2(y,z,fy,fz) \
		):( \
			ARGMAX2(y,u,fy,fu) \
			) \
	)

/*
 * Map bigger id on smaller id
 * and set ID to smaller id.
 * */
#define JOIN_IDS_A(ID, A, B) \
	if( (A) < (B) ){ \
		*(comp_same	+ (B)) = (A); \
		(ID) = (A); \
	}else{  \
		*(comp_same	+ (A)) = (B); \
		(ID) = (B); \
	}

/*
 * Map bigger (real) id on smaller id
 * */
#define JOIN_IDS_B( pA, pB) \
	if( *(pA) < *(pB) ){ \
		*(comp_same	+ *(pB)) = *(pA); \
		*(prob_parent + *(pB-1)) = *(pA); \
	}else{  \
		*(comp_same	+ *(pA)) = *(pB); \
		*(prob_parent + *(pA-1)) = *(pB); \
	}


#ifdef BLOB_COUNT_PIXEL
#define COUNT(X) X;
#define BLOB_REALLOC_COMP_SIZE comp_size = realloc(comp_size, max_comp*sizeof(int) );
#define BLOB_INIT_COMP_SIZE *(comp_size+id) = 1;
#define BLOB_INC_COMP_SIZE(ID) *(comp_size+ID) += 1; 
#else
/* empty definitions */
#define COUNT(X) ;
#define BLOB_REALLOC_COMP_SIZE
#define BLOB_INIT_COMP_SIZE
#define BLOB_INC_COMP_SIZE(ID) 
#endif

#ifdef PIXEL_POSITION
#define SZ(X) X;
#else
#define SZ(X) 
#endif

#ifdef BLOB_DIMENSION
#define BD(X) X;

#define BLOB_DIMENSION_LEFT(ID, STEPWIDTH) \
	/*if( *( left_index+ID ) > s ) *( left_index+ID ) -= STEPWIDTH; */ \
	if( *( left_index+ID ) > s ) *( left_index+ID ) = s; /* Is this correct for STEPWITDTH>1?! */

#define BLOB_DIMENSION_RIGHT(ID, STEPWIDTH) \
	/*if( *( right_index+(ID) ) < s ) *( right_index+(ID) ) += STEPWIDTH; */ \
	if( *( right_index+(ID) ) < s ) *( right_index+(ID) ) = s; 

#define BLOB_DIMENSION_BOTTOM(ID, STEPHEIGHT) \
	/*if( *( bottom_index+(ID) ) < z ) *( bottom_index+(ID) ) += STEPHEIGHT; */ \
	if( *( bottom_index+(ID) ) < z ) *( bottom_index+(ID) ) = z;

#else
/* empty definitions */
#define BD(X) 
#define BLOB_DIMENSION_LEFT(ID, STEPWIDTH)
#define BLOB_DIMENSION_RIGHT(ID, STEPWIDTH)
#define BLOB_DIMENSION_BOTTOM(ID, STEPHEIGHT)
#endif

#ifdef BLOB_BARYCENTER
#define BARY(X) X;
#define BLOB_REALLOC_BARY \
	pixel_sum_X = realloc(pixel_sum_X, max_comp*sizeof(BLOB_BARYCENTER_TYPE) ); \
pixel_sum_X = realloc(pixel_sum_X, max_comp*sizeof(BLOB_BARYCENTER_TYPE) );

#define BLOB_INIT_BARY *(pixel_sum_X+id) = s; *(pixel_sum_Y+id) = z;
#define BLOB_INC_BARY(ID) *(pixel_sum_X+ID) += s;  *(pixel_sum_Y+ID) += z;
#else
/* empty definitions */
#define BARY(X) ;
#define BLOB_REALLOC_BARY 
#define BLOB_INIT_BARY 
#define BLOB_INC_BARY(ID) 
#endif

#define NEW_COMPONENT(PARENTID, DEPTH) id++; \
*(iPi) = id; \
*(id_depth+id) = (DEPTH); \
*(prob_parent+id) = (PARENTID); \
*(comp_same+id) = id; \
BLOB_INIT_COMP_SIZE; \
BD( \
*(left_index+id) = s; \
*(right_index+id) = s; \
*(top_index+id) = z; \
*(bottom_index+id) = z; \
	) \
BLOB_INIT_BARY; \
if( id>=max_comp ){ \
	int max_comp2 = (int) ( (float)w*h*max_comp/(dPi-data) ); /*try estimation */ \
	if( true || max_comp2 < max_comp*1.5 ){ \
		max_comp += max_comp; \
	}else{ \
		max_comp = max_comp2; \
	}\
	VPRINTF("Extend max_comp=%i\n", max_comp); \
	depthtree_realloc_workspace(max_comp, &workspace); \
	/* Reallocation requires update of pointers */ \
		ids = workspace->ids; \
		comp_same = workspace->comp_same; \
		prob_parent = workspace->prob_parent; \
		id_depth = workspace->id_depth; \
		COUNT( comp_size = workspace->comp_size; ) \
		BD( \
		top_index = workspace->top_index; \
		left_index = workspace->left_index; \
		right_index = workspace->right_index; \
		bottom_index = workspace->bottom_index; \
		) \
		BARY( pixel_sum_X = workspace->pixel_sum_X; pixel_sum_Y = workspace->pixel_sum_Y; ) \
} \


#define INSERT_ELEMENT1( IDA, pIDX, DEPX )  \
			\
		a_ids = workspace->a_ids; \
		a_dep = workspace->a_dep; \
		\
		DEPTHPRINTF("(insert_element) Input: IDA=%i, \t\t DEPX=%i\n", IDA, DEPX); \
		\
		/* 0. Go to parent ids until no depth exceed DEPX. */ \
		do{ \
			a_ids++; \
			a_dep++; \
			*a_ids = getRealId( comp_same, IDA); \
			*a_dep = *(id_depth + *a_ids ); \
			IDA = *( prob_parent + *a_ids); \
		}while( *a_dep>DEPX); \
		\
		DEPTHPRINTF("(insert_element) Start values: rIDA=%i, depA=%i, DEPX=%i\n", *a_ids, *a_dep, DEPX); \
		\
		/* Now, the following conditions are held: \
		 * \
		 * *(β_dep-1) > DEPX >= *(β_dep), β∈{a} \
		 * \
		 * */ \
		\
		/* 1. Compare DEPX with the depth of a \
		 * */ \
		if( *a_dep < DEPX){ \
			DEPTHPRINTF("(insert_element) Case 1a) Generate new component\n");  \
			NEW_COMPONENT( *a_ids, DEPX ); \
			*pIDX = id; \
			\
			/* Update parent relation with connection to pIDX. */ \
			*(prob_parent + *(a_ids-1) ) = *pIDX; \
			*(prob_parent + *pIDX ) = *a_ids; \
			DEPTHPRINTF("(insert_element) P(%i)=%i, P(%i)=%i\n", \
					*(a_ids-1), *pIDX, *pIDX, *a_ids ); \
			\
		}else{ /* Same depth as DEPX */ \
			DEPTHPRINTF("(insert_element) Case 1b) same component\n");  \
			*pIDX = *a_ids; \
			BLOB_INC_COMP_SIZE( *pIDX );  \
			/* TODO: Disticnt this function into to cases. One with R-Check for first row, \
			 * one with L,B-Check for first column */  \
			BLOB_DIMENSION_RIGHT( *pIDX, stepwidth ); \
			BLOB_DIMENSION_LEFT( *pIDX, stepwidth ); \
			BLOB_DIMENSION_BOTTOM( *pIDX, stepheight ); \
			BLOB_INC_BARY( *pIDX );  \
		} \
/* End of INSERT_ELEMENT1. */

#define INSERT_ELEMENT2( IDA, IDB, pIDX, DEPX )  \
			\
		a_ids = workspace->a_ids; \
		b_ids = workspace->b_ids; \
		a_dep = workspace->a_dep; \
		b_dep = workspace->b_dep; \
		\
		DEPTHPRINTF("(eval_chains) Input: IDA=%i, IDB=%i\n\t\t DEPX=%i\n", IDA, IDB, DEPX); \
		/* 0. Go to parent ids until no depth exceed DEPX. */ \
		\
		do{ \
			a_ids++; \
			a_dep++; \
			*a_ids = getRealId( comp_same, IDA); \
			*a_dep = *(id_depth + *a_ids ); \
			IDA = *( prob_parent + *a_ids); \
		}while( *a_dep>DEPX); \
		do{ \
			b_ids++; \
			b_dep++; \
			*b_ids = getRealId( comp_same, IDB); \
			*b_dep = *(id_depth + *b_ids ); \
			IDB = *( prob_parent + *b_ids); \
		}while( *b_dep>DEPX); \
		\
		DEPTHPRINTF("(eval_chains) Start values: rIDA=%i, rIDB=%i, depA=%i, depB=%i, DEPX=%i\n", *a_ids, *b_ids, *a_dep, *b_dep, DEPX); \
		\
		/* Now, the following conditions are held: \
		 * \
		 * *(β_dep-1) > DEPX >= *(β_dep), β∈{a,b} \
		 * \
		 * */ \
		\
		/* 1. Compare DEPX with the depth of a, and b. (5 Cases) \
		 * */ \
/*Debug */ \
/*{int xxx;\
for(xxx=0;xxx<=id;xxx++){\
printf("CS(%i)=%i, P(%i)=%i, D(%i)=%i \n",xxx,*(comp_same+xxx), xxx, *(prob_parent+xxx)  ,xxx, *(id_depth+xxx));\
}}*/ \
		if( *a_dep < DEPX && *b_dep < DEPX ){ /* 1.a) */ \
			DEPTHPRINTF("(eval_chains) Case 1a) Generate new component\n");  \
			NEW_COMPONENT( \
					ARGMAX2(*a_ids, *b_ids, *a_dep, *b_dep ),  \
					DEPX ); \
			*pIDX = id; \
			DEPTHPRINTF("(eval_chains) pIDX=%i\n", *pIDX);  \
			\
			/* Update parent relation with connection to pIDX. */ \
			*(prob_parent + *(a_ids-1) ) = *pIDX; \
			*(prob_parent + *(b_ids-1) ) = *pIDX; \
			if( *a_dep > *b_dep ){ \
				*(prob_parent + *pIDX ) = *a_ids; \
			}else{ \
				*(prob_parent + *pIDX ) = *b_ids; \
			} \
			/* prob_parent + *[a|b]_ids will be set later. */ \
			DEPTHPRINTF("(eval_chains) P(%i)=%i, P(%i)=%i\n P(%i)=%i\n", \
					*(a_ids-1), *pIDX, *(b_ids-1), *pIDX, \
					*pIDX, *(prob_parent+*pIDX)	);  \
			\
		}else if( *a_dep > *b_dep){ /* 1.b) a_dep == DEPX */ \
			*pIDX = *a_ids; \
			BLOB_INC_COMP_SIZE( *pIDX );  \
			BLOB_DIMENSION_RIGHT( *pIDX, stepwidth ); \
			BLOB_DIMENSION_BOTTOM( *pIDX, stepheight ); \
			BLOB_INC_BARY( *pIDX );  \
			DEPTHPRINTF("(eval_chains) Case 1b) Same id as a-branch.\n");  \
			DEPTHPRINTF("(eval_chains) idX=%i\n", *pIDX);  \
			DEPTHPRINTF("(eval_chains) P(%i)=%i\n", *(b_ids-1), *pIDX); \
			\
			/* Update parent relation with connection to pIDX. */ \
			*(prob_parent + *(b_ids-1) ) = *pIDX; \
			\
		}else if( *a_dep < *b_dep){ /* 1.c) b_dep == DEPX */ \
			*pIDX = *b_ids; \
			BLOB_INC_COMP_SIZE( *pIDX );  \
			BLOB_DIMENSION_LEFT( *pIDX, stepwidth ); \
			BLOB_DIMENSION_BOTTOM( *pIDX, stepheight ); \
			BLOB_INC_BARY( *pIDX );  \
			DEPTHPRINTF("(eval_chains) Case 1c) Same id as b-branch.\n");  \
			DEPTHPRINTF("(eval_chains) idX=%i\n", *pIDX);  \
			DEPTHPRINTF("(eval_chains) P(%i)=%i\n", *(a_ids-1), *pIDX); \
			\
			/* Update parent relation with connection to pIDX. */ \
			*(prob_parent + *(a_ids-1) ) = *pIDX; \
			\
		}else if( *a_ids != *b_ids) { /* 1.d) a_dep == DEPX = b_dep, but different components */ \
			/* join components*/ \
			DEPTHPRINTF("(eval_chains) Case 1d) Join components.\n");  \
			DEPTHPRINTF("(eval_chains) Join (%i,%i)\n", *a_ids, *b_ids);  \
			/* TODO: Hier könnte man die Kinder der verbundenen Komponenten auf die bessere Real Id umlenken ?!*/ \
			/*JOIN_IDS_A(pIDX, *a_ids, *b_ids ); */ /*Achtung, das Joinen passiert in der Whileschleife in Schritt 2 und wäre hier kontraproduktiv!*/ \
			*pIDX = *a_ids; \
			BLOB_INC_COMP_SIZE( *pIDX );  \
			BLOB_DIMENSION_BOTTOM( *pIDX, stepheight ); \
			BLOB_INC_BARY( *pIDX );  \
			\
		}else{ /* 1.e) Same component. Hm, this case should be tested as first. */ \
			DEPTHPRINTF("(eval_chains) Case 1e) Extend id %i\n", *b_ids);  \
			*pIDX = *a_ids; \
			BLOB_INC_COMP_SIZE( *pIDX );  \
			BLOB_DIMENSION_BOTTOM( *pIDX, stepheight ); \
			BLOB_INC_BARY( *pIDX );  \
		} \
		\
		DEPTHPRINTF("(eval_chains) Start for Part 2: rIDA=%i, rIDB=%i, depA=%i, depB=%i\n", *a_ids, *b_ids, *a_dep, *b_dep);  \
		/* 2. Merge parent stacks of both ids. */ \
		while( *a_ids != *b_ids ){ \
			if( *a_dep > *b_dep ){ /* 2.a) remap a_ids to b_ids if parent of a_ids lower as b_ids */ \
				/* Get parent and write it into next position (Attend ++!)*/ \
				DEPTHPRINTF("(eval_chains) Case 2a)\n");  \
				/* *a_ids = getRealParent( prob_parent, comp_same, *(a_ids++)); */\
				*(a_ids+1) = getRealParent( prob_parent, comp_same, *a_ids);++a_ids; \
				*++a_dep = *(id_depth + *a_ids ); \
				if( *a_dep < *b_dep){ \
					DEPTHPRINTF("(eval_chains) Remap P(%i)=%i\n", *(a_ids-1), *b_ids );  \
					*(prob_parent + *(a_ids-1)) = *b_ids; \
				} \
				/* continue loop with a_dep[k+1] instead of a_dep[k]...*/ \
				\
			}else if( *a_dep < *b_dep ){ /* 2.b) Same as 2.a) with swaped variables */ \
				/* Get parent and write it into next position (Attend ++!)*/ \
				DEPTHPRINTF("(eval_chains) Case 2b)\n");  \
				/* *b_ids = getRealParent( prob_parent, comp_same, *(b_ids++)); */\
				*(b_ids+1) = getRealParent( prob_parent, comp_same, *b_ids);++b_ids; \
				*++b_dep = *(id_depth + *b_ids ); \
				if( *b_dep < *a_dep){ \
					DEPTHPRINTF("(eval_chains) Remap P(%i)=%i\n", *(b_ids-1), *a_ids );  \
					*(prob_parent + *(b_ids-1)) = *a_ids; \
				} \
				\
			}else{ /* 2.c) Join different components. */ \
				DEPTHPRINTF("(eval_chains) Case 2c) Join\n");  \
				JOIN_IDS_B( a_ids, b_ids );		 \
				/* Remap parent relation for bigger depth(?) */ \
				/* Get parent and write it into next position (Attend ++!) */ \
				/* *a_ids = getRealParent( prob_parent, comp_same, *(a_ids++)); */\
				*(a_ids+1) = getRealParent( prob_parent, comp_same, *a_ids);++a_ids; \
				/* *b_ids = getRealParent( prob_parent, comp_same, *(b_ids++)); */\
				*(b_ids+1) = getRealParent( prob_parent, comp_same, *b_ids);++b_ids; \
				*++a_dep = *(id_depth + *a_ids ); \
				*++b_dep = *(id_depth + *b_ids ); \
				if( *a_dep < *b_dep ){ \
					/*Update parent of *(a_ids-1) */ \
					*(prob_parent + *(a_ids-1)) = *b_ids; \
				}else{ \
					*(prob_parent + *(b_ids-1)) = *a_ids; \
				} \
			} \
			\
			DEPTHPRINTF("(eval_chains) Next loop step: rIDA=%i, rIDB=%i, depA=%i, depB=%i\n", *a_ids, *b_ids, *a_dep, *b_dep);  \
		} \


#endif
