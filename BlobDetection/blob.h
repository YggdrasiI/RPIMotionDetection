#ifndef BLOBTREE_H
#define BLOBTREE_H

/* Blobtree struct definitions. See tree.h for struct 'Blob'.
 * The blobtree struct bundles settings
 * for the main algorithms and
 * contains the result.
 * ( Some internal variables are stored in the workspace struct.	Interesting could be:
 *	• *ids : Every pixel of Image (or ROI) gets an id. Attention blobs can contain multiple ids.
 *	• *comp_same : Projection on ids maps all ids of one area on unique id.
 *	• *prop_parent : Store ids of parent areas. Attention, there is no guarantee that
 *				prop_parent(id) = comp_same( prop_parent( comp_same(id) ) )
 *		holds for every pixel.
 * )
 *
 * Workflow:
 * 1. Create workspace (one time, optional(?) )
 * 2. Create blobtree struct (blobtree_create() ) 
 * For every image:
 * 	3. Call blob search algorithm ( *_find_blobs(...) )
 * 	4. Setup blob filter  ( blobtree_set_filter(...) )
 * 	5. Loop through blobs ( blobtree_first(), blobtree_next() )
 * Cleanup:	
 * 6. Destroy blobtree
 * 7. Destroy workspace
 *
 * */

#include "tree.h"

typedef struct {
	unsigned int width;
	unsigned int height;
} Grid;

typedef enum {
	F_TREE_DEPTH_MIN=1,
	F_TREE_DEPTH_MAX=2,
	F_AREA_MIN=4,
	F_AREA_MAX=8,
	F_ONLY_LEAFS=16,
	F_AREA_DEPTH_MIN=32,
	F_AREA_DEPTH_MAX=64
} FILTER;

/* Filter handler to mark nodes for filtering out.
 * Interpretation of return value:
 * 0 - The node will not filtered out.
 * 1 -	The node will filtered out. Algorithm continues with child, if existing.
 * 2 -	The node will filtered out and all children, too.
 * 			All children will be skiped during tree cycle.
 * >2 -	The node will filtered out and all children and all silbings
 * 			will be skiped.
 * */
typedef int (FilterNodeHandler)(Node *node);

typedef struct {
	unsigned int tree_depth_min;
	unsigned int tree_depth_max;
	unsigned int min_area;
	unsigned int max_area;
	unsigned char only_leafs;/*0 or 1*/
	unsigned char area_depth_min;
	unsigned char area_depth_max;
	FilterNodeHandler* extra_filter;
} Filter;

typedef struct {
	Node *node;
	int depth;
} Iterator;

typedef struct {
	Tree *tree; 
	Blob *tree_data;
	Filter filter;
	Grid grid; // width between compared pixels (Could leave small blobs undetected.)
	Iterator it; //node itarator for intern usage
	Iterator it_next; //node itarator for intern usage
} Blobtree;


/* Create blob struct. Use threshtree_destroy to free mem. */
void blobtree_create(Blobtree **blob);
void blobtree_destroy(Blobtree **blob );

/* Set one of the default filter values */
void blobtree_set_filter( Blobtree *blob,const FILTER f,const unsigned int val);
/* Add own node filter function */
void blobtree_set_extra_filter(Blobtree *blob, FilterNodeHandler* extra_filter);

/* Set difference between compared pixels. Could ignore small blobs. */
void blobtree_set_grid(Blobtree *blob, const unsigned int gridwidth, const unsigned int gridheight );

/* Returns first node which is matching
 * the filter criteria or NULL. */
Node *blobtree_first( Blobtree *blob);

/* Returns next element under the given filters. 
 * or NULL */
Node *blobtree_next( Blobtree *blob);


#endif
