//some old stuff
#include "threshtree_macros_old.h"


Tree* find_connection_components_coarse(
		const unsigned char *data,
		const int w, const int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		const int stepwidth,
		const int stepheight,
		Blob **tree_data,
		ThreshtreeWorkspace *workspace )
{
	/* Marks of 10 Cases:
	 *  x - stepwidth, y - stepheigt, swr - (w-1)%x, shr - (h-1)%y
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
	int r=w-roi.x-roi.width; //right border
	int b=h-roi.y-roi.height; //bottom border
	if( r<0 || b<0 ){
		fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
		*tree_data = NULL;
		return NULL;
	}

	int swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
	int shr = (roi.height-1)%stepheight; // remainder of height/stepheight;
	int sh = stepheight*w;
	int sh1 = (stepheight-1)*w;
	int sh2 = shr*w;

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
#endif


	const unsigned char* const dS = data+w*roi.y+roi.x;
	const unsigned char* dR = dS+roi.width; //Pointer to right border. Update on every line
	const unsigned char* dR2 = dR-swr; //cut last indizies.

	const unsigned char* const dE = dR + (roi.height-1)*w;
	const unsigned char* const dE2 = dE - sh2;//remove last lines.

	//int i = w*roi.y+roi.x;
	const unsigned char* dPi = dS; // Pointer to data+i //TODO
	int* iPi = ids+(dS-data); // Poiner to ids+i

	/**** A,A'-CASE *****/
	//top, left corner of BlobtreeRect get first id.
	NEW_COMPONENT_OLD(-1)
		iPi += stepwidth;
	dPi += stepwidth;

#ifdef BLOB_DIMENSION
	s += stepwidth;
#endif

	/* Split all logic to two subcases:
	 * *(dPi)<=thresh (marked as B,C,…),
	 * *(dpi)>thresh (marked as B',C',…)
	 * to avoid many 'x&&y || x|y==0' checks.
	 * */

	//top border
	for(;dPi<dR2;iPi += stepwidth,dPi += stepwidth){
		if( *(dPi) > thresh ){
			/**** B-CASE *****/
			if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
				LEFT_CHECK(stepwidth)
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-stepwidth) )
			}
		}else{
			/**** B'-CASE *****/
			if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
				LEFT_CHECK(stepwidth)
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-stepwidth) )
			}
		}
#ifdef BLOB_DIMENSION
		s += stepwidth;
#endif
	}

	//correct pointer shift of last for loop step.
	iPi -= stepwidth-swr;
	dPi -= stepwidth-swr;
#ifdef BLOB_DIMENSION
	s -= stepwidth-swr;
#endif

	//continue with +swr stepwidth
	if(swr){
		if( *(dPi) > thresh ){
			/**** C-CASE *****/
			if( *(dPi-swr) > thresh ){//same component as left neighbour
				LEFT_CHECK(swr)
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-swr) )
			}
		}else{
			/**** C'-CASE *****/
			if( *(dPi-swr) <= thresh ){//same component as left neighbour
				LEFT_CHECK(swr)
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-swr) )
			}
		}
	}

	//move pointer to 'next' row
	dPi += r+roi.x+sh1+1;
	iPi += r+roi.x+sh1+1;
	dR += sh; //rechter Randindex wird in nächste Zeile geschoben.
	dR2 += sh;
#ifdef BLOB_DIMENSION
	s=roi.x;
	z += stepheight;
#endif	

	//2nd,...,(h-shr)-row
	for( ; dPi<dE2 ; dPi += r+roi.x+sh1+1,iPi += r+roi.x+sh1+1, dR += sh, dR2 += sh ){

		//left border
		if( *(dPi) > thresh ){
			/**** E-CASE *****/
			if( *(dPi-sh) > thresh ){//same component as top neighbour
				TOP_CHECK(stepheight, sh)
#ifdef BLOB_DIAGONAL_CHECK
			}else if( *(dPi-sh+stepwidth) > thresh ){//same component as diagonal neighbour
				DIAG_CHECK(stepwidth, stepheight, sh)
#endif
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-sh) )
			}
		}else{
			/**** E'-CASE *****/
			if( *(dPi-sh) <= thresh){//same component as top neighbour
				TOP_CHECK(stepheight, sh)
#ifdef BLOB_DIAGONAL_CHECK
			}else if( *(dPi-sh+stepwidth) <= thresh ){//same component as diagonal neighbour
				DIAG_CHECK(stepwidth, stepheight, sh)
#endif
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-sh) )
			}
		}

		iPi += stepwidth;
		dPi += stepwidth;
#ifdef BLOB_DIMENSION
		s += stepwidth;
#endif

		/*inner elements till last colum before dR2 reached.
		 * => Lefthand tests with -stepwidth
		 * 		Righthand tests with +stepwidth
		 */
		for( ; dPi<dR2-stepwidth; iPi += stepwidth, dPi += stepwidth ){
			if( *(dPi) > thresh ){
				/**** F-CASE *****/
				if( *(dPi-sh) > thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) > thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh+stepwidth) > thresh ){
							LEFT_DIAG_COMP(stepwidth,sh)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh+stepwidth) > thresh ){
							ANTI_DIAG_COMP(stepwidth,sh)
						}
				}else if( *(dPi-sh+stepwidth) > thresh ){//same component as diagonal neighbour
					DIAG_CHECK(stepwidth, stepheight, sh)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** F'-CASE *****/
				if( *(dPi-sh) <= thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) <= thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh+stepwidth) <= thresh ){
							LEFT_DIAG_COMP(stepwidth,sh)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh+stepwidth) <= thresh ){
							ANTI_DIAG_COMP(stepwidth,sh)
						}
				}else if( *(dPi-sh+stepwidth) <= thresh ){//same component as diagonal neighbour
					DIAG_CHECK(stepwidth, stepheight, sh)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}
#ifdef BLOB_DIMENSION
			s += stepwidth;
#endif
		}


		/* If dR2==dR, then the last column is reached. Thus it's not possibe
		 * to check diagonal element. (only G case)
		 * Otherwise use G and H cases. 
		 */
		if( dR2==dR ){
			if( *(dPi) > thresh ){
				/**** G-CASE *****/
				if( *(dPi-sh) > thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) > thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
#endif
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** G'-CASE *****/
				if( *(dPi-sh) <= thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) <= thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
#endif
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}

		}else{
			//structure: (dPi-stepwidth),(dPi),(dPi+swr)
			if( *(dPi) > thresh ){
				/**** G-CASE *****/
				if( *(dPi-sh) > thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) > thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh+swr) > thresh ){
							LEFT_DIAG_COMP(swr,sh)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh+swr) > thresh ){
							ANTI_DIAG_COMP(swr,sh)
						}
				}else if( *(dPi-sh+swr) > thresh ){//same component as diagonal neighbour
					DIAG_CHECK(swr,stepheight,sh)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** G'-CASE *****/
				if( *(dPi-sh) <= thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) <= thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh+swr) <= thresh ){
							LEFT_DIAG_COMP(swr,sh)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-stepwidth) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh+swr) <= thresh ){
							ANTI_DIAG_COMP(swr,sh)
						}
				}else if( *(dPi-sh+swr) <= thresh ){//same component as diagonal neighbour
					DIAG_CHECK(swr,stepheight,sh)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}
#ifdef BLOB_DIMENSION
			s+=swr;
#endif
			iPi+=swr;
			dPi+=swr;

			//right border, not check diag element
			if( *(dPi) > thresh ){
				/**** H-CASE *****/
				if( *(dPi-sh) > thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-swr) > thresh ){
							TOP_LEFT_COMP(swr);
						}
				}else	if( *(dPi-swr) > thresh ){//same component as left neighbour
					LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-swr) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(swr, stepheight, sh)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** H'-CASE *****/
				if( *(dPi-sh) <= thresh ){//same component as top neighbour
					TOP_CHECK(stepheight, sh)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-swr) <= thresh ){
							TOP_LEFT_COMP(swr);
						}
				}else	if( *(dPi-swr) <= thresh ){//same component as left neighbour
					LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh-swr) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(swr, stepheight, sh)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}
		}//end of else case of (dR2==dR)

#ifdef BLOB_DIMENSION
		s=roi.x;
		z += stepheight;
#endif

	} //row for loop
	iPi -= sh-sh2;//(stepheight-1)*w;
	dPi -= sh-sh2;//(stepheight-1)*w;
	dR -= sh-sh2;
	dR2 -= sh-sh2;
#ifdef BLOB_DIMENSION
	z -= stepheight-shr;
#endif

	if(dE2<dE){

		//left border
		if( *(dPi) > thresh ){
			/**** L-CASE *****/
			if( *(dPi-sh2) > thresh ){//same component as top neighbour
				TOP_CHECK(shr, sh2)
#ifdef BLOB_DIAGONAL_CHECK
			}else if( *(dPi-sh2+stepwidth) > thresh ){//same component as diagonal neighbour
				DIAG_CHECK(stepwidth, shr, sh2)
#endif
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-sh2) )
			}
		}else{
			/**** L'-CASE *****/
			if( *(dPi-sh2) <= thresh ){//same component as top neighbour
				TOP_CHECK(shr, sh2)
#ifdef BLOB_DIAGONAL_CHECK
			}else if( *(dPi-sh2+stepwidth) <= thresh ){//same component as diagonal neighbour
				DIAG_CHECK(stepwidth, shr, sh2)
#endif
			}else{//new component
				NEW_COMPONENT_OLD( *(iPi-sh2) )
			}
		}

		iPi += stepwidth;
		dPi += stepwidth;
#ifdef BLOB_DIMENSION
		s += stepwidth;
#endif

		/*inner elements till last colum before dR2 reached.
		 * => Lefthand tests with -stepwidth
		 * 		Righthand tests with +stepwidth
		 */
		for( ; dPi<dR2-stepwidth; iPi += stepwidth, dPi += stepwidth ){
			if( *(dPi) > thresh ){
				/**** M-CASE *****/
				if( *(dPi-sh2) > thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) > thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh2+stepwidth) > thresh ){
							LEFT_DIAG_COMP(stepwidth,sh2)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-stepwidth) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, shr, sh2)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh2+stepwidth) > thresh ){
							ANTI_DIAG_COMP(stepwidth,sh2)
						}
				}else if( *(dPi-sh2+stepwidth) > thresh ){//same component as diagonal neighbour
					DIAG_CHECK(stepwidth, shr, sh2)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** M'-CASE *****/
				if( *(dPi-sh2) <= thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) <= thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh2+stepwidth) <= thresh ){
							LEFT_DIAG_COMP(stepwidth,sh2)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-stepwidth) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, shr, sh2)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh2+stepwidth) <= thresh ){
							ANTI_DIAG_COMP(stepwidth,sh2)
						}
				}else if( *(dPi-sh2+stepwidth) <= thresh ){//same component as diagonal neighbour
					DIAG_CHECK(stepwidth, shr, sh2)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}

#ifdef BLOB_DIMENSION
			s += stepwidth;
#endif
		}

		/* If dR2==dR, then the last column is reached. Thus it's not possibe
		 * to check diagonal element. (only N case)
		 * Otherwise use N and P cases. 
		 */
		if( dR2==dR ){
			if( *(dPi) > thresh ){
				/**** N-CASE *****/
				if( *(dPi-sh2) > thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) > thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-stepwidth) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, shr, sh2)
#endif
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** N'-CASE *****/
				if( *(dPi-sh2) <= thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) <= thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-stepwidth) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, shr, sh2)
#endif
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}

		}else{
			//structure: (dPi-stepwidth),(dPi),(dPi+swr)
			if( *(dPi) > thresh ){
				/**** N-CASE *****/
				if( *(dPi-sh2) > thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) > thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh2+swr) > thresh ){
							LEFT_DIAG_COMP(swr,sh2)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-stepwidth) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, shr, sh2)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh2+swr) > thresh ){
							ANTI_DIAG_COMP(swr,sh2)
						}
				}else if( *(dPi-sh2+swr) > thresh ){//same component as diagonal neighbour
					DIAG_CHECK(swr,shr,sh2)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** N'-CASE *****/
				if( *(dPi-sh2) <= thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-stepwidth) <= thresh ){
							TOP_LEFT_COMP(stepwidth);
						}
				}else	if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
					LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
						/* check if diag neighbour id can associate with left neighbour id.*/
						if( *(dPi-sh2+swr) <= thresh ){
							LEFT_DIAG_COMP(swr,sh2)
						}
#endif
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-stepwidth) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(stepwidth, shr, sh2)
						/* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
						if( *(dPi-sh2+swr) <= thresh ){
							ANTI_DIAG_COMP(swr,sh2)
						}
				}else if( *(dPi-sh2+swr) <= thresh ){//same component as diagonal neighbour
					DIAG_CHECK(swr,shr,sh2)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}

			iPi+=swr;
			dPi+=swr;
#ifdef BLOB_DIMENSION
			s+=swr;
#endif

			//right border, not check diag element
			if( *(dPi) > thresh ){
				/**** P-CASE *****/
				if( *(dPi-sh2) > thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-swr) > thresh ){
							TOP_LEFT_COMP(swr);
						}
				}else	if( *(dPi-swr) > thresh ){//same component as left neighbour
					LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-swr) > thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(swr, shr, sh2)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}else{
				/**** P'-CASE *****/
				if( *(dPi-sh2) <= thresh ){//same component as top neighbour
					TOP_CHECK(shr, sh2)
						// check if left neighbour id can associate with top neigbour id.
						if( *(dPi-swr) <= thresh ){
							TOP_LEFT_COMP(swr);
						}
				}else	if( *(dPi-swr) <= thresh ){//same component as left neighbour
					LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
				}else	if( *(dPi-sh2-swr) <= thresh ){//same component as anti diagonal neighbour
					ANTI_DIAG_CHECK(swr, shr, sh2)
#endif				
				}else{//new component
					NEW_COMPONENT_OLD( *(iPi-stepwidth) )
				}
			}

		}//end of else case of (dR2==dR)

	} //end of if(dE2==dE)

	/* end of main algo */

#if VERBOSE > 0 
	//printf("Matrix of ids:\n");
	//print_matrix(ids,w,h);

	//printf("comp_same:\n");
	//print_matrix(comp_same, id+1, 1);
	debug_print_matrix( ids, w, h, roi, 1, 1);
	debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, 0);
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

	for(k=0;k<nids;k++){
		tmp_id = k;
		tmp_id2 = *(comp_same+tmp_id); 
#if VERBOSE > 0
		printf("%i: (%i->%i) ",k,tmp_id,tmp_id2);
#endif
		while( tmp_id2 != tmp_id ){

#ifdef BLOB_COUNT_PIXEL
			//move area size to other id.
			*(comp_size+tmp_id2) += *(comp_size+tmp_id); 
			*(comp_size+tmp_id) = 0;
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

			tmp_id = tmp_id2; 
			tmp_id2 = *(comp_same+tmp_id); 

#if VERBOSE > 0
			printf("(%i->%i) ",tmp_id,tmp_id2);
#endif
		}
#if VERBOSE > 0 
		printf("\n");
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

	curdata->id = -1;
	curdata->area = roi.width * roi.height;
	memcpy( &curdata->roi, &roi, sizeof(BlobtreeRect) );
	cur->data = curdata; // link to the data array.

	BlobtreeRect *rect;

	for(l=0;l<real_ids_size;l++){
		cur++;
		curdata++;
		cur->data = curdata; // link to the data array.

		rect = &curdata->roi;
		curdata->id = *(real_ids+l);	//Set id of this blob.
		//not useful?!
		//int anchor = *(anchors+*(real_ids+l)); //get anchor of this blob
#ifdef BLOB_DIMENSION
		rect->y = *(top_index + *(real_ids+l));
		rect->height = *(bottom_index + *(real_ids+l)) - rect->y + 1;
		rect->x = *(left_index + *(real_ids+l));
		rect->width = *(right_index + *(real_ids+l)) - rect->x + 1;
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

	//sum up node areas
#ifdef BLOB_COUNT_PIXEL
	sum_areas(root->child, comp_size);
#endif

#ifdef BLOB_SORT_TREE
	//sort_tree(root->child);
	sort_tree(root);
#endif

	//current id indicates maximal used id in ids-array
	workspace->used_comp=id;

	//clean up
	free(tree_id_relation);

	//set output parameter
	*tree_data = blobs;
	return tree;
}
