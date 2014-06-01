#ifndef THRESHTREE_MACROS
#define THRESHTREE_MACROS

/*
 *
 * Notes:
 * - This macros just avoid repeat of many code parts.
 * - Not all vars in this macro are params...
 * - This macros require the following if-else-order:
 *
				if( *(dPi-sh) COND ){//same component as top neighbour
					TOP_CHECK(...)
					// check if left neighbour id can associate with top neigbour id.
					if( *(dPi-stepwidth) > thresh ){
						TOP_LEFT_COMP(...)
					}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(...)
					// check if diagonal neighbour id can associate with left neigbour id.
					if( *(dPi-sh+stepwidth) > thresh ){
						LEFT_DIAG_COMP(...)
					}
				#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) COND ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(...)
					// check if diagonal neighbour id can associate with anti diagonal neigbour id.
					if( *(dPi-sh+stepwidth) > thresh ){
						ANTI_DIAG_COMP(...)
					}
				}else if( *(dPi-sh+stepwidth) COND ){//same component as diagonal neighbour
					DIAG_CHECK(...)
				#endif				
				}else{//new component
					NEW_COMPONENT
				}

 *	(COND i.E. > thresh)
 */

#ifdef BLOB_COUNT_PIXEL
#define COUNT(X) X;
#define BLOB_REALLOC_COMP_SIZE comp_size = realloc(comp_size, max_comp*sizeof(unsigned int) );
#define BLOB_INIT_COMP_SIZE *(comp_size+id) = 0; /*Increase now every pixel. => Can't start with 1 anymore. Overhead of |ids| operations */
//#define BLOB_INC_COMP_SIZE *(comp_size+*(iPi)) += 1;
#define BLOB_INC_COMP_SIZE(ID) *(comp_size+ID) += 1; 
#else
/* empty definitions */
#define COUNT(X) ;
#define BLOB_REALLOC_COMP_SIZE
#define BLOB_INIT_COMP_SIZE
//#define BLOB_INC_COMP_SIZE
#define BLOB_INC_COMP_SIZE(ID)
#endif

#ifdef PIXEL_POSITION
#define SZ(X) X;
#else
#define SZ(X) 
#endif

#ifdef BLOB_DIMENSION
#define BD(X) X;
#define BLOB_INIT_INDEX_ARRAYS \
*(top_index+id) = z; \
*(left_index+id) = s; \
*(right_index+id) = s; \
*(bottom_index+id) = z;

#define BLOB_DIMENSION_LEFT(STEPWIDTH) \
	/*	if( *( left_index+*(iPi) ) > s ) *( left_index+*(iPi) ) -= STEPWIDTH; */ \
	if( *( left_index+*(iPi) ) > s ) *( left_index+*(iPi) ) = s; /* Is this correct for STEPWITDTH>1?! */

#define BLOB_DIMENSION_RIGHT(STEPWIDTH) \
	/*	if( *( right_index+*(iPi) ) < s ) *( right_index+*(iPi) ) += STEPWIDTH; */ \
	if( *( right_index+*(iPi) ) < s ) *( right_index+*(iPi) ) = s;

#define BLOB_DIMENSION_BOTTOM(STEPHEIGHT) \
	/*	if( *( bottom_index+*(iPi) ) < z ) *( bottom_index+*(iPi) ) += STEPHEIGHT; */ \
	if( *( bottom_index+*(iPi) ) < z ) *( bottom_index+*(iPi) ) = z;

#else
/* empty definitions */
#define BD(X)
#define BLOB_INIT_INDEX_ARRAYS
#define BLOB_DIMENSION_LEFT
#define BLOB_DIMENSION_RIGHT
#define BLOB_DIMENSION_BOTTOM
#endif

#ifdef BLOB_BARYCENTER
#define BARY(X) X;
#define BLOB_REALLOC_BARY \
	pixel_sum_X = realloc(pixel_sum_X, max_comp*sizeof(BLOB_BARYCENTER_TYPE) ); \
pixel_sum_X = realloc(pixel_sum_X, max_comp*sizeof(BLOB_BARYCENTER_TYPE) );

//#define BLOB_INIT_BARY *(pixel_sum_X+id) = s; *(pixel_sum_Y+id) = z;
#define BLOB_INIT_BARY *(pixel_sum_X+id) = 0; *(pixel_sum_Y+id) = 0; /* (0,0) is the matching start value for BLOB_INIT_COMP_SIZE(...) = 0. Prevent values on 'subpixels'. */
#define BLOB_INC_BARY(ID) *(pixel_sum_X+ID) += s;  *(pixel_sum_Y+ID) += z;
#else
/* empty definitions */
#define BARY(X) ;
#define BLOB_REALLOC_BARY 
#define BLOB_INIT_BARY 
#define BLOB_INC_BARY(ID) 
#endif

#define NEW_COMPONENT(PARENTID) \
	id++; \
/*if(*(iPi) > 0 ){ \
	printf("Fehler: Id war schon gesetzt! (%i, %i), a=%i,n=%i, triT1=%i,triT2=%i\n",s,z,*(iPi),id, *(tri-triwidth), *(tri-triwidth+1) ); \
{ const BlobtreeRect roiVorne = {s-3-5,z-20+1,26,26}; debug_print_matrix( ids, w, h, roiVorne, 1, 1); unsigned int crash=1/0;} \
}{ \
unsigned int ss=(iPi-ids)%roi.width;if( ss!=s)printf("Spaltenproblem: %i!=%i\n",s,ss);\
}\*/ \
*(iPi) = id; \
/* *(anchors+id) = dPi-dS; */\
*(prob_parent+id) = PARENTID; \
*(comp_same+id) = id; \
BLOB_INIT_COMP_SIZE; \
BLOB_INIT_INDEX_ARRAYS; \
BLOB_INIT_BARY; \
if( id>=max_comp ){ \
	max_comp = (unsigned int) ( (float)w*h*max_comp/(dPi-data) ); \
	VPRINTF("Extend max_comp=%i\n", max_comp); \
	threshtree_realloc_workspace(max_comp, &workspace); \
	/* Reallocation requires update of pointers */ \
		ids = workspace->ids; \
		comp_same = workspace->comp_same; \
		prob_parent = workspace->prob_parent; \
		COUNT( comp_size = workspace->comp_size; ) \
		BD( \
		top_index = workspace->top_index; \
		left_index = workspace->left_index; \
		right_index = workspace->right_index; \
		bottom_index = workspace->bottom_index; \
		) \
}


#define TOP_CHECK(STEPHEIGHT,WIDTH) \
	*(iPi) = *(iPi-WIDTH); \
/*	BLOB_INC_COMP_SIZE;*/ \
BLOB_DIMENSION_BOTTOM(STEPHEIGHT);

/* check if left neighbour id can associate with top neigbour id. */
/* The diagonal and anti diagonal check is not ness. due to checks on top element.*/
/* => g(f(x)) == g(f(y)) */
#define TOP_LEFT_COMP(STEPWIDTH) \
	a1 = *(comp_same+*(iPi-STEPWIDTH)); \
	a2 = *(comp_same+*(iPi  )); \
	if( a1<a2 ){ \
VPRINTF("(%i=>%i), (%i=>%i) changed to ", *(iPi), a2, *(iPi-STEPWIDTH),a1); \
		*(comp_same+a2) = a1; \
		*(comp_same+*(iPi) ) = a1; \
VPRINTF("(%i=>%i), (%i=>%i), (%i,%i), TOP_LEFT_COMP\n", *(iPi), a1, *(iPi-STEPWIDTH),a1, s,z); \
	}else if(a1>a2){ \
VPRINTF("(%i=>%i), (%i=>%i) changed to ", *(iPi), a2, *(iPi-STEPWIDTH),a1); \
		*(comp_same+a1) = a2; \
		*(comp_same+*(iPi-STEPWIDTH) ) = a2; \
VPRINTF("(%i=>%i), (%i=>%i) (%i,%i), TOP_LEFT_COMP\n", *(iPi), a2, *(iPi-STEPWIDTH),a2, s,z); \
	} \

#define LEFT_CHECK(STEPWIDTH) \
	*(iPi) = *(iPi-STEPWIDTH); \
/*BLOB_INC_COMP_SIZE;*/ \
BLOB_DIMENSION_RIGHT(STEPWIDTH);

/* check if left neighbour id can associate with diagonal neigbour id. */
#define LEFT_DIAG_COMP(STEPWIDTHRIGHT,WIDTH) \
	a1 = *(comp_same+*(iPi-WIDTH+STEPWIDTHRIGHT)); \
	a2 = *(comp_same+*(iPi    )); \
	if( a1<a2 ){ \
VPRINTF("(%i=>%i), (%i=>%i) changed to ", *(iPi), a2, *(iPi+STEPWIDTHRIGHT-WIDTH),a1); \
		*(comp_same+a2) = a1; \
		*(comp_same+*(iPi) ) = a1; \
VPRINTF("(%i=>%i), (%i=>%i) (%i,%i), LEFT_DIAG_COMP\n", *(iPi), a1, *(iPi+STEPWIDTHRIGHT-WIDTH),a1,s,z);  \
	}else if(a1>a2){ \
VPRINTF("(%i=>%i), (%i=>%i) changed to ", *(iPi), a2, *(iPi+STEPWIDTHRIGHT-WIDTH),a1); \
		*(comp_same+a1) = a2; \
		*(comp_same+ *(iPi-WIDTH+STEPWIDTHRIGHT) ) = a2; \
VPRINTF("(%i=>%i), (%i=>%i) (%i,%i), LEFT_DIAG_COMP\n", *(iPi), a2, *(iPi+STEPWIDTHRIGHT-WIDTH),a2,s,z);  \
	}


#define ANTI_DIAG_CHECK(STEPWIDTH,STEPHEIGHT,WIDTH) \
	*(iPi) = *(iPi-WIDTH-STEPWIDTH); \
/*BLOB_INC_COMP_SIZE;*/ \
BLOB_DIMENSION_RIGHT(STEPWIDTH); \
BLOB_DIMENSION_BOTTOM(STEPHEIGHT);


/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
#define ANTI_DIAG_COMP(STEPWIDTHRIGHT,WIDTH) \
	a1 = *(comp_same+*(iPi-WIDTH+STEPWIDTHRIGHT)); \
	a2 = *(comp_same+*(iPi    )); \
	if( a1<a2 ){ \
VPRINTF("(%i=>%i), (%i=>%i) changed to ", *(iPi), a2, *(iPi+STEPWIDTHRIGHT-WIDTH),a1); \
		*(comp_same+a2) = a1; \
		*(comp_same+*(iPi) ) = a1; \
VPRINTF("(%i=>%i), (%i=>%i) (%i,%i), ANTI_DIAG_COMP\n", *(iPi),a1 , *(iPi+STEPWIDTHRIGHT-WIDTH),a1,s,z);  \
	}else if(a1>a2){ \
VPRINTF("(%i=>%i), (%i=>%i) changed to ", *(iPi), a2, *(iPi+STEPWIDTHRIGHT-WIDTH),a1); \
		*(comp_same+a1) = a2; \
		*(comp_same+ *(iPi - WIDTH+STEPWIDTHRIGHT) ) = a2; \
VPRINTF("(%i=>%i), (%i=>%i) (%i,%i), ANTI_DIAG_COMP\n", *(iPi),a2 , *(iPi+STEPWIDTHRIGHT-WIDTH),a2,s,z);  \
	}


#define DIAG_CHECK(STEPWIDTH,STEPHEIGHT,WIDTH) \
	*(iPi) = *(iPi-WIDTH+STEPWIDTH); \
/*BLOB_INC_COMP_SIZE;*/ \
BLOB_DIMENSION_LEFT(STEPWIDTH); \
BLOB_DIMENSION_BOTTOM(STEPHEIGHT);



#ifdef BLOB_DIAGONAL_CHECK

#define BLOB_DIAGONAL_CHECK_1(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP) \
			/* check if diag neighbour id can associate with left neighbour id.*/ \
			if( *(DPI-W+1) COMP thresh ){ \
				LEFT_DIAG_COMP(1,W); \
			}

#define BLOB_DIAGONAL_CHECK_2a(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP) \
	else	if( *(DPI-W-1) COMP thresh ){/* same component as anti diagonal neighbour */ \
		ANTI_DIAG_CHECK(1, 1, W); \
		/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/ \
		if( *(DPI-W+1) COMP thresh ){ \
			ANTI_DIAG_COMP(1,W); \
		} \
	}

#define BLOB_DIAGONAL_CHECK_2b(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP) \
else	if( *(DPI-W-1) COMP thresh ){/* same component as anti diagonal neighbour */ \
		ANTI_DIAG_CHECK(1, 1, W); \
	}

#define BLOB_DIAGONAL_CHECK_3(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP) \
	else if( *(DPI-W+1) COMP thresh ){/*same component as diagonal neighbour */ \
		DIAG_CHECK(1, 1, W); \
	}

#else
/* empty definitions */
#define BLOB_DIAGONAL_CHECK_1(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP)
#define BLOB_DIAGONAL_CHECK_2a(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP)
#define BLOB_DIAGONAL_CHECK_2b(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP)
#define BLOB_DIAGONAL_CHECK_3(DPI,IPI,STEPWIDTH,W,SH,S,Z,COMP)
#endif



/* Macros for subchecks
 * Important: After the subcheck macros
 *	SUBCHECK_PART{1|2a|2b|3|4a|4b}
 * dPi, iPi points to the same element
 * as before the macro was called. For
 *	SUBCHECK_PART{NONE}
 *	it Points to dPi+1, iPi+1.
 * */

/* Test current point with top,left, anti diagonal
 * and diagonal value. */
#define SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z)  \
	if( *(DPI) > thresh ){ \
		if( *(DPI-W) > thresh ){/*same component as top neighbour */ \
			TOP_CHECK(1, W); \
			/* check if left neighbour id can associate with top neigbour id. */ \
			if( *(DPI-1) > thresh ){ \
				TOP_LEFT_COMP(1); \
			} \
		}else	if( *(DPI-1) > thresh ){/*same component as left neighbour */ \
			LEFT_CHECK(1); \
			BLOB_DIAGONAL_CHECK_1(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
		} \
		BLOB_DIAGONAL_CHECK_2a(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
		BLOB_DIAGONAL_CHECK_3(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
		else{/*new component*/ \
			NEW_COMPONENT( *(IPI-1) ); \
		} \
	}else{ \
		if( *(DPI-W) <= thresh ){/*same component as top neighbour */ \
			TOP_CHECK(1, W); \
			/* check if left neighbour id can associate with top neigbour id. */ \
			if( *(DPI-1) <= thresh ){ \
				TOP_LEFT_COMP(1); \
			} \
		}else	if( *(DPI-1) <= thresh ){/*same component as left neighbour */ \
			LEFT_CHECK(1); \
			BLOB_DIAGONAL_CHECK_1(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
		} \
		BLOB_DIAGONAL_CHECK_2a(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
		BLOB_DIAGONAL_CHECK_3(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
		else{/*new component*/ \
			NEW_COMPONENT( *(IPI-1) ); \
		} \
	} \


/* Test current point with top,left and anti diagonal value. */
#define SUBCHECK_TOPLEFT(DPI,IPI,STEPWIDTH,W,SH,S,Z) \
if( *(DPI) > thresh ){ \
	if( *(DPI-W) > thresh ){/*same component as top neighbour */ \
		TOP_CHECK(1, W); \
		/* check if left neighbour id can associate with top neigbour id. */ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else	if( *(DPI-1) > thresh ){/*same component as left neighbour */ \
		LEFT_CHECK(1); \
	} \
	BLOB_DIAGONAL_CHECK_2b(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
	else{/*new component*/ \
		NEW_COMPONENT( *(IPI-1) ); \
	} \
}else{ \
	if( *(DPI-W) <= thresh ){/*same component as top neighbour */ \
		TOP_CHECK(1, W); \
		/* check if left neighbour id can associate with top neigbour id. */ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else	if( *(DPI-1) <= thresh ){/*same component as left neighbour */ \
		LEFT_CHECK(1); \
	} \
	BLOB_DIAGONAL_CHECK_2b(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
	else{/*new component*/ \
		NEW_COMPONENT( *(IPI-1) ); \
	} \
} \


/* Test current point with top
 * and diagonal value. */
#define SUBCHECK_TOPDIAG(DPI,IPI,STEPWIDTH,W,SH,S,Z) \
if( *(DPI) > thresh ){ \
	if( *(DPI-W) > thresh ){/*same component as top neighbour */ \
		TOP_CHECK(1, W); \
	}else if( *(DPI-W+1) > thresh ){/*same component as diagonal neighbour */ \
		DIAG_CHECK(1, 1, W); \
	}else{/*new component*/ \
		NEW_COMPONENT( *(IPI-W) ); \
	} \
}else{ \
	if( *(DPI-W) <= thresh ){/*same component as top neighbour */ \
		TOP_CHECK(1, W); \
	}else if( *(DPI-W+1) <= thresh ){/*same component as diagonal neighbour */ \
		DIAG_CHECK(1, 1, W); \
	}else{/*new component*/ \
		NEW_COMPONENT( *(IPI-W) ); \
	} \
} \


/* Check of row without consider other rows. */
/* | s |swr|,  s-stepwidth
 * 1xxxXxxx
 * */
#define SUBCHECK_ROW(DPI,IPI,STEPWIDTH,W,SH,S,Z,SWR) { \
	const unsigned char * const pc = DPI+SWR; \
	DPI -= STEPWIDTH-1; \
	IPI -= STEPWIDTH-1; \
	SZ(S -= STEPWIDTH-1); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
	} \
	/*DPI -= SWR; */ \
	/*IPI -= SWR; */ \
	/*S -= SWR; */ \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			LEFT_CHECK(1) \
		}else{ \
			NEW_COMPONENT( *(IPI-1) ); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			LEFT_CHECK(1) \
		}else{ \
			NEW_COMPONENT( *(IPI-1) ); \
		} \
	} \
}

/* Check of row without consider other rows. */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 1xxx1
 * 00000
 * 00000
 * 00000
 * 0000X
 */
#define SUBCHECK_PART1a(DPI,IPI,STEPWIDTH,W,SH,S,Z) {\
	const unsigned char * const pc = DPI-SH; \
	DPI -= SH+STEPWIDTH-1; \
	IPI -= SH+STEPWIDTH-1; \
	SZ(S -= STEPWIDTH-1); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
	} \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	} \
	DPI += SH; \
	IPI += SH; \
	SZ(Z += STEPWIDTH); \
}

/* Check of row with top check. */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 *  111
 * 1xxx1
 * 00000
 * 00000
 * 00000
 * 0000X
 */
#define SUBCHECK_PART1b(DPI,IPI,STEPWIDTH,W,SH,S,Z) {\
	const unsigned char * const pc = DPI-SH; \
	DPI -= SH+STEPWIDTH-1; \
	IPI -= SH+STEPWIDTH-1; \
	SZ(S -= STEPWIDTH-1); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-W) > thresh ){ \
				TOP_CHECK(1,W) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) > thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-W) <= thresh ){ \
				TOP_CHECK(1,W) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) <= thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
	} \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	} \
	DPI += SH; \
	IPI += SH; \
	SZ(Z += STEPWIDTH); \
}

/* Check of row with top check. */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 00000
 * 0001
 * 001
 * 01
 * 1xxx1
 * 00000
 * 00000
 * 00000
 * 0000X
 */
#define SUBCHECK_PART1bb(DPI,IPI,STEPWIDTH,W,SH,S,Z) {\
	const unsigned char * const pc = DPI-SH; \
	unsigned int shift = W, shh=1; \
	DPI -= SH+STEPWIDTH-1; \
	IPI -= SH+STEPWIDTH-1; \
	SZ(S -= STEPWIDTH-1); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-shift) > thresh ){ \
				TOP_CHECK(shh,shift) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) > thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1) \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-shift) <= thresh ){ \
				TOP_CHECK(shh,shift) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) <= thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
		shift += W; ++shh; \
	} \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	} \
	DPI += SH; \
	IPI += SH; \
	SZ(Z += STEPWIDTH); \
}

/* Check of row without consider other rows. */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 00001xxx1
 * 00000
 * 00000
 * 00000
 * 0000X
 */
#define SUBCHECK_PART1c(DPI,IPI,STEPWIDTH,W,SH,S,Z) {\
	const unsigned char * const pc = DPI-SH+STEPWIDTH; \
	DPI -= SH-1; \
	IPI -= SH-1; \
	SZ(++S); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
	} \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	} \
	DPI += SH-STEPWIDTH; \
	IPI += SH-STEPWIDTH; \
	SZ(S -= STEPWIDTH); \
	SZ(Z += STEPWIDTH); \
}



/* Check of row with top check */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 *      111
 * 00001xxx1
 * 00000
 * 00000
 * 00000
 * 0000X
 */
#define SUBCHECK_PART1d(DPI,IPI,STEPWIDTH,W,SH,S,Z) {\
	const unsigned char * const pc = DPI-SH+STEPWIDTH; \
	DPI -= SH-1; \
	IPI -= SH-1; \
	SZ(++S); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-W) > thresh ){ \
				TOP_CHECK(1,W) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) > thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-W) <= thresh ){ \
				TOP_CHECK(1,W) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) <= thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
	} \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	} \
	DPI += SH-STEPWIDTH; \
	IPI += SH-STEPWIDTH; \
	SZ(S -= STEPWIDTH); \
	SZ(Z += STEPWIDTH); \
}


/* Check of row with top check */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 0   00000
 *      001
 *      01
 *      1
 * 00001xxx1
 * 00000
 * 00000
 * 00000
 * 0000X
 */
#define SUBCHECK_PART1dd(DPI,IPI,STEPWIDTH,W,SH,S,Z) {\
	const unsigned char * const pc = DPI-SH+STEPWIDTH; \
	unsigned int shift=W, shh=1; \
	DPI -= SH-1; \
	IPI -= SH-1; \
	SZ(++S); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		if( *(DPI) > thresh ){ \
			if( *(DPI-shift) > thresh ){ \
				TOP_CHECK(shh,shift) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) > thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) > thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		}else{ \
			if( *(DPI-shift) <= thresh ){ \
				TOP_CHECK(shh,shift) \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-1) <= thresh ){ \
					TOP_LEFT_COMP(1); \
				} \
			}else if( *(DPI-1) <= thresh ){ \
				LEFT_CHECK(1); \
			}else{ \
				NEW_COMPONENT( *(IPI-1) ); \
			} \
		} \
		++DPI; ++IPI; \
		SZ(++S); \
		shift += W; ++shh; \
	} \
	if( *(DPI) > thresh ){ \
		if( *(DPI-1) > thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	}else{ \
		if( *(DPI-1) <= thresh ){ \
			TOP_LEFT_COMP(1); \
		} \
	} \
	DPI += SH-STEPWIDTH; \
	IPI += SH-STEPWIDTH; \
	SZ(S -= STEPWIDTH); \
	SZ(Z += STEPWIDTH); \
}


/* Switch between 1a or 1b */
#define SUBCHECK_PART1ab(DPI,IPI,STEPWIDTH,W,SH,S,Z) \
	if( *(tri-triwidth-1) > 1) { /*SUBCHECK_PART1b(DPI,IPI,STEPWIDTH,W,SH,S,Z)*/ } \
	else if( *(tri-triwidth-1) == 1 ) { SUBCHECK_PART1bb(DPI,IPI,STEPWIDTH,W,SH,S,Z) } \
	else { SUBCHECK_PART1a(DPI,IPI,STEPWIDTH,W,SH,S,Z) }


/* Switch between 1c or 1d */
#define SUBCHECK_PART1cd(DPI,IPI,STEPWIDTH,W,SH,S,Z) \
	if( *(tri-triwidth) > 1 ){ /*SUBCHECK_PART1d(DPI,IPI,STEPWIDTH,W,SH,S,Z)*/ } \
	else if( *(tri-triwidth) == 1 ) { SUBCHECK_PART1dd(DPI,IPI,STEPWIDTH,W,SH,S,Z) } \
	else { SUBCHECK_PART1c(DPI,IPI,STEPWIDTH,W,SH,S,Z) }



/* Check of col without consider other cols. */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 10000
 * x0000
 * x0000
 * x0000
 * 0000X
 */
#define SUBCHECK_PART2a(DPI,IPI,STEPWIDTH,W,SH,S,Z) { \
	const unsigned char * const pc = DPI-W; \
	DPI -= SH+STEPWIDTH; \
	IPI -= SH+STEPWIDTH; \
	SZ(S -= STEPWIDTH); \
	SZ(Z -= STEPWIDTH); \
	for( ;DPI<pc; ){ \
		DPI+=W; IPI+=W; \
		SZ(++Z); \
		if( *(DPI) > thresh ){ \
			if( *(DPI-W) > thresh ){ \
				TOP_CHECK(1,W) \
			}else{ \
				NEW_COMPONENT( *(IPI-W) ); \
			} \
		}else{ \
			if( *(DPI-W) <= thresh ){ \
				TOP_CHECK(1,W) \
			}else{ \
				NEW_COMPONENT( *(IPI-W) ); \
			} \
		} \
	} \
	DPI += STEPWIDTH; \
	IPI += STEPWIDTH; \
	SZ(S += STEPWIDTH); \
}


/* Left border case */
/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 11111
 * xxxx0
 * xxx00
 * xx000
 * X0000
 */
#define SUBCHECK_PART2b(DPI,IPI,STEPWIDTH,W,SH,S,Z) { \
	const unsigned char * const pc = DPI; \
	DPI -= sh1; \
	IPI -= sh1; \
	SZ(Z -= STEPWIDTH-1); \
	unsigned int ww = STEPWIDTH-1; \
	for( ; DPI<pc ; ){ \
		SUBCHECK_TOPDIAG(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		++DPI; ++IPI; \
		SZ(++S); \
		const	unsigned char *xx = DPI+ww; \
		for( ; DPI<xx ; ){ \
			SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
			++DPI; ++IPI; \
			SZ(++S);\
		} \
		DPI += W-ww-1; \
		IPI += W-ww-1; \
		SZ(S -= ww+1); \
		SZ(++Z); \
		--ww; \
	} \
	/* Corner */ \
	SUBCHECK_TOPDIAG(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
}


/*
 * 1111111
 * xxxxxx
 * xxxxx
 * 000X
 */
#define SUBCHECK_PART3a(DPI,IPI,STEPWIDTH,W,SH,S,Z) { \
	const unsigned char * const pc = DPI-STEPWIDTH; \
	DPI -= sh1/*SH-W*/+STEPWIDTH; \
	IPI -= sh1/*SH-W*/+STEPWIDTH; \
	SZ(S -= STEPWIDTH); \
	SZ(Z -= STEPWIDTH-1); \
	unsigned int ww = STEPWIDTH+STEPWIDTH-1; \
	for( ; DPI<pc ; ){ \
		SUBCHECK_TOPDIAG(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		++DPI; ++IPI; \
		SZ(++S); \
		const	unsigned char *xx = DPI+ww; \
		for( ; DPI<xx ; ){ \
			SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
			++DPI; ++IPI; \
			SZ(++S)\
		} \
		DPI += W-ww-1; \
		IPI += W-ww-1; \
		SZ(S -= ww+1) \
		SZ(++Z); \
		--ww; \
	} \
	DPI += STEPWIDTH; \
	IPI += STEPWIDTH; \
	SZ(S += STEPWIDTH) \
}


/* Second case: Some elements already set. */
/*
 * 0011111
 * 011xxx
 * 11xxx
 * 000X
 */
#define SUBCHECK_PART3b(DPI,IPI,STEPWIDTH,W,SH,S,Z) { \
	const unsigned char * const pc = DPI-W; \
	DPI -= sh1/*SH-W*/; \
	IPI -= sh1/*SH-W*/; \
	SZ(Z -= STEPWIDTH-1); \
	for( ; DPI<=pc ; ) { \
		const	unsigned char *xx = DPI+STEPWIDTH; \
		for( ; DPI<xx ; ){ \
			SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
			++DPI; ++IPI; \
			SZ(++S); \
		} \
		DPI += W-STEPWIDTH-1; \
		IPI += W-STEPWIDTH-1; \
		SZ(S -= STEPWIDTH+1); \
		SZ(++Z); \
	} \
	DPI += STEPWIDTH-1; \
	IPI += STEPWIDTH-1; \
	SZ(S += STEPWIDTH-1); \
}


/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 00000
 * 00000
 * 00000
 * 111111
 * 1xxxX
 */
#define SUBCHECK_PART4a(DPI,IPI,STEPWIDTH,W,SH,S,Z) { \
	const unsigned char * const pc = DPI; \
	DPI -= STEPWIDTH-1; \
	IPI -= STEPWIDTH-1; \
	SZ(S -= STEPWIDTH-1); \
	for( ;DPI<pc; ){ \
		SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		++DPI; ++IPI; \
		SZ(++S); \
	} \
	SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
}


/* Handle positions x, look at ones and x's. X mark DPI pointer.
 * 00000
 * 00000
 * 00000
 * 000111
 * 1---X
 */
#define SUBCHECK_PART4b(DPI,IPI,STEPWIDTH,W,SH,S,Z) { \
	if( *(DPI) > thresh ){ \
		if( *(DPI-W) > thresh ){/*same component as top neighbour */ \
			TOP_CHECK(1, W); \
			/* check if left neighbour id can associate with top neigbour id. */ \
			if( *(DPI-STEPWIDTH) > thresh ){ \
				TOP_LEFT_COMP(STEPWIDTH); \
			} \
		}else	if( *(DPI-STEPWIDTH) > thresh ){/*same component as left neighbour */ \
			LEFT_CHECK(STEPWIDTH); \
			BLOB_DIAGONAL_CHECK_1(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
		} \
		BLOB_DIAGONAL_CHECK_2a(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
		BLOB_DIAGONAL_CHECK_3(DPI,IPI,STEPWIDTH,W,SH,S,Z,>) \
		else{/*new component*/ \
			NEW_COMPONENT( *(IPI-W) ); \
		} \
	}else{ \
		if( *(DPI-W) <= thresh ){/*same component as top neighbour */ \
			TOP_CHECK(1, W); \
				/* check if left neighbour id can associate with top neigbour id. */ \
				if( *(DPI-STEPWIDTH) <= thresh ){ \
					TOP_LEFT_COMP(STEPWIDTH); \
				} \
		}else	if( *(DPI-STEPWIDTH) <= thresh ){/*same component as left neighbour */ \
			LEFT_CHECK(STEPWIDTH); \
			BLOB_DIAGONAL_CHECK_1(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
		} \
		BLOB_DIAGONAL_CHECK_2a(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
		BLOB_DIAGONAL_CHECK_3(DPI,IPI,STEPWIDTH,W,SH,S,Z,<=) \
		else{/*new component*/ \
			NEW_COMPONENT( *(IPI-W) ); \
		} \
	} \
}


/* Rightborder: Eval all Neighbours */
/* 1111 11
 * xxxx xx
 * xxxx xx
 * 1xxX xx
 * */
#define SUBCHECK_PART6a(DPI,IPI,STEPWIDTH,W,SH,S,Z,SWR) { \
	const unsigned char * const pc = DPI-W+SWR; \
	DPI -= sh1/*SH-W*/+STEPWIDTH; \
	IPI -= sh1/*SH-W*/+STEPWIDTH; \
	SZ(S -= STEPWIDTH); \
	SZ(Z -= STEPWIDTH-1); \
	for( ; DPI<pc ; ){ \
		/* Left border */ \
		SUBCHECK_TOPDIAG(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		++DPI; ++IPI; \
		SZ(++S);\
		const	unsigned char *xx = DPI+STEPWIDTH+SWR-1; \
		for( ; DPI<xx ; ){ \
			SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
			++DPI; ++IPI; \
			SZ(++S);\
		} \
		/*Right border */ \
		SUBCHECK_TOPLEFT(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		DPI += W-STEPWIDTH-SWR; \
		IPI += W-STEPWIDTH-SWR; \
		SZ(S -= STEPWIDTH+SWR); \
		SZ(++Z); \
	} \
	/*Last row, skip corner */ \
	++DPI; ++IPI; \
	SZ(++S);\
	const	unsigned char *xx = DPI+STEPWIDTH+SWR-1; \
	for( ; DPI<xx ; ){ \
		SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		++DPI; ++IPI; \
		SZ(++S);\
	} \
	/*Right border */ \
	SUBCHECK_TOPLEFT(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
	DPI -= SWR; \
	IPI -= SWR; \
	SZ(S -= SWR); \
}


/* Second case: Some elements are already evaluated. */
/* 0011 11
 * 011x xx
 * 11xx xx
 * 1xxX xx
 * */
#define SUBCHECK_PART6b(DPI,IPI,STEPWIDTH,W,SH,S,Z,SWR) { \
	const unsigned char * const pc = DPI+SWR; \
	DPI -= sh1/*SH-W*/; \
	IPI -= sh1/*SH-W*/; \
	SZ(Z -= STEPWIDTH-1); \
	unsigned int ww = 0; \
	for( ; DPI<pc ; ){ \
		const	unsigned char *xx = DPI+SWR+ww; \
		for( ; DPI<xx ; ){ \
			SUBCHECK_ALLDIR(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
			++DPI; ++IPI; \
			SZ(++S);\
		} \
		/*Right border */ \
		SUBCHECK_TOPLEFT(DPI,IPI,STEPWIDTH,W,SH,S,Z); \
		++ww; \
		DPI += W-SWR-ww; \
		IPI += W-SWR-ww; \
		SZ(S -= SWR+ww); \
		SZ(++Z); \
	} \
	DPI -= W-STEPWIDTH; \
	IPI -= W-STEPWIDTH; \
	SZ(S += STEPWIDTH); \
	SZ(--Z); \
}


#endif
