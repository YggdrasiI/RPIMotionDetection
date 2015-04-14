#include "tree.h"
#include "blob.h"

//+++++++++++++++++++++++++++++
// Blobtree functions
//+++++++++++++++++++++++++++++

void blobtree_create(Blobtree **pblob){
	if( *pblob != NULL ){
		blobtree_destroy(pblob);
	}
	Blobtree* blob = (Blobtree*) malloc(sizeof(Blobtree) );
	blob->tree = NULL;
	blob->tree_data = NULL;
	Filter filter = {0,INT_MAX, 0,INT_MAX, 0, 0,255, NULL };
	blob->filter = filter;
	Grid grid = {1,1};
	blob->grid = grid;

	*pblob = blob;
}

void blobtree_destroy(Blobtree **pblob){
	if( *pblob == NULL ) return;
	Blobtree *blob = *pblob;
	if( blob->tree != NULL ){
        tree_destroy(&blob->tree);
	}
    if( blob->tree_data != NULL) {
        free(blob->tree_data);
        blob->tree_data = NULL;
    }
	free(blob);
	*pblob = NULL;
}

void blobtree_set_filter(Blobtree *blob, const FILTER f, const unsigned int val){
	switch(f){
		case F_TREE_DEPTH_MIN: blob->filter.tree_depth_min=val;
											break;
		case F_TREE_DEPTH_MAX: blob->filter.tree_depth_max=val;
											break;
		case F_AREA_MIN: blob->filter.min_area=val;
											break;
		case F_AREA_MAX: blob->filter.max_area=val;
											break;
		case F_ONLY_LEAFS: blob->filter.only_leafs=(val>0?1:0);
											 break;
		case F_AREA_DEPTH_MIN: { blob->filter.area_depth_min=val;
#if VERBOSE > 0
													 if(/*val<0||*/val>255) printf("(blobtree_set_filter) range error: val=%u leave range [0,255]");
#endif
													 }
								 break; 
		case F_AREA_DEPTH_MAX: { blob->filter.area_depth_max=val;
#if VERBOSE > 0
													 if(/*val<0||*/val>255) printf("(blobtree_set_filter) range error: val=%u leave range [0,255]");
#endif
													 }
													 break; 
	}
}

void blobtree_set_extra_filter(Blobtree *blob, FilterNodeHandler* extra_filter){
	blob->filter.extra_filter = extra_filter;
}

void blobtree_set_grid(Blobtree *blob, const unsigned int gridwidth, const unsigned int gridheight ){
	blob->grid.width = gridwidth;
	blob->grid.height =  gridheight;
}


void blobtree_next2(Blobtree *blob, Iterator* pit);

Node *blobtree_first( Blobtree *blob){
	blob->it.node = blob->tree->root;
	blob->it.depth = -1;
	blob->it_next.node = blob->tree->root;
	blob->it_next.depth = -1;
	if( blob->filter.only_leafs ){
		blobtree_next2(blob,&blob->it_next);
	}
	return blobtree_next(blob);
}

Node *blobtree_next( Blobtree *blob){

	if( blob->filter.only_leafs ){
		while(	blob->it_next.node != NULL ){
			blob->it.node = blob->it_next.node;
			blob->it.depth = blob->it_next.depth;
			blobtree_next2(blob,&blob->it_next);
			/*check if it_next.node is successor of it.node.
			 * This condition is wrong for leafs */
			if(!successor(blob->it.node, blob->it_next.node) ){
				return blob->it.node;
			}
		}
		return NULL;
	}else{
			blobtree_next2(blob,&blob->it);
			return blob->it.node;
	}
}

/* This method throws an NULL pointer exception
 * if it's has already return NULL and called again. Omit that. */
void blobtree_next2(Blobtree *blob, Iterator* pit){

	//go to next element
	Node *it = pit->node;
	int it_depth = pit->depth;

	if( it->child != NULL){
		it = it->child;
		it_depth++;
	}else	if( it->silbing != NULL ){
		it = it->silbing;
	}else{
		while( 1 ){
			it = it->parent;
			it_depth--;
			if(it->silbing != NULL){
				it = it->silbing;
				break;
			}
			if( it_depth < 0 ){
				pit->node = NULL;
				return;
			}
		}
	}

	const Node * const root = blob->tree->root;
	//check criteria/filters.
	do{
		if( it_depth < blob->filter.tree_depth_min 
#ifdef SAVE_DEPTH_MAP_VALUE
				|| ((Blob*)it->data)->depth_level < blob->filter.area_depth_min
#endif
				|| ((Blob*)it->data)->area > blob->filter.max_area ){
			if( it->child != NULL){
				it = it->child;
				it_depth++;
				continue;
			}
			if( it->silbing != NULL ){
				it = it->silbing;
				continue;
			}
			while( 1 ){
				it = it->parent;
				it_depth--;
				if(it->silbing != NULL){
					it = it->silbing;
					break;
				}
				if( it_depth < 0 ){
					pit->node = NULL;
					return;
				}
			}
			continue;
		}
		if( it_depth > blob->filter.tree_depth_max
#ifdef SAVE_DEPTH_MAP_VALUE
				|| ((Blob*)it->data)->depth_level > blob->filter.area_depth_max
#endif
				){
			while( 1 ){
				it = it->parent;
				it_depth--;
				if(it->silbing != NULL){
					it = it->silbing;
					break;
				}
				if( it_depth < 0 ){
					pit->node = NULL;
					return;
				}
			}
			continue;
		}
		if( ((Blob*)it->data)->area < blob->filter.min_area ){
			if( it->silbing != NULL ){
				it = it->silbing;
				continue;
			}
			while( 1 ){
				it = it->parent;
				it_depth--;
				if(it->silbing != NULL){
					it = it->silbing;
					break;
				}
				if( it_depth < 0 ){
					pit->node = NULL;
					return;
				}
			}
			continue;
		}

		//extra filter handling
		if( blob->filter.extra_filter != NULL ){
			unsigned int ef = (*blob->filter.extra_filter)(it);
			if( ef ){
				if( ef<2 && it->child != NULL){
					it = it->child;
					it_depth++;
					continue;
				}
				if( ef<3 && it->silbing != NULL ){
					it = it->silbing;
					continue;
				}
				while( 1 ){
					it = it->parent;
					it_depth--;
					if(it->silbing != NULL){
						it = it->silbing;
						break;
					}
					if( it_depth < 0 ){
						pit->node = NULL;
						return;
					}
				}
			}
		}

		// All filters ok. Return node
		pit->node = it;
		pit->depth = it_depth;
		return;

	}while( it != root );

	//should never reached
	pit->node = NULL;
	pit->depth = -1;
	return;
}
