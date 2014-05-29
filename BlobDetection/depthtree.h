#ifndef DEPTHTREE_H
#define DEPTHTREE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "settings.h"
#include "tree.h"
#include "blob.h"

/* Workspace struct for array storage */
typedef struct {
	unsigned int max_comp; // = w+h;//maximal number of components. If the value is reached some arrays will reallocate.
	unsigned int used_comp; // number of used ids ; will be set after the main algorithm finishes ; <=max_comp
	unsigned int *ids;
	unsigned char *depths; // use monotone function to map image data into different depths
	unsigned int *id_depth; //store depth (group) of id anchor.
	unsigned int *comp_same; //map ids to unique ids g:{0,...,}->{0,....}
	unsigned int *prob_parent; //store ⊂-Relation.
#ifdef BLOB_COUNT_PIXEL
	unsigned int *comp_size;
#endif
#ifdef BLOB_DIMENSION
	unsigned int *top_index; //save row number of most top element of area.
	unsigned int *left_index; //save column number of most left element of area.
	unsigned int *right_index; //save column number of most right element.
	unsigned int *bottom_index; //save row number of most bottom element.
#endif
#ifdef BLOB_BARYCENTER
	BLOB_BARYCENTER_TYPE *pixel_sum_X; //summation of all x coordinates for an id.
	BLOB_BARYCENTER_TYPE *pixel_sum_Y; //summation of all x coordinates for an id.
#endif
	/* Geometric interpretation of the positions a,b,c,d
	 * in relation to x(=current) position:
	 * abc
	 * dx
	 * */
	unsigned int *a_ids; //save chain of ids with depth(a_ids[0])>depth(a_ids[1])>...
	unsigned char *a_dep; //save depth(a_ids[...]) to avoid lookup with id_depth(a_ids[k]).
	unsigned int *b_ids, *c_ids, *d_ids; //same for b,c,d 
	unsigned char *b_dep, *c_dep, *d_dep;  

	unsigned int *real_ids;
	unsigned int *real_ids_inv;

	//extra data
	unsigned int *blob_id_filtered; //like comp_same, but respect blob tree filter.

} DepthtreeWorkspace;


bool depthtree_create_workspace(
		const unsigned int w, const unsigned int h,
		DepthtreeWorkspace **pworkspace
		);
bool depthtree_realloc_workspace(
		const unsigned int max_comp,
		DepthtreeWorkspace **pworkspace
		);
void depthtree_destroy_workspace(
		DepthtreeWorkspace **pworkspace
		);

/* The tree contains all blobs 
 * and the id array does not respect
 * the setted filter values.
 * This function uses the filter to
 * generate an updated version of the
 * id mapping. 
 * After calling this method the workspace
 * contains the map blob_id_filtered
 * with the property 
 * 		blob_id_filtered(id(pixel)) ∈ {Ids of matching Nodes}
 *
 * This is useful to draw the map of filtered blobs.
 * */
void depthtree_filter_blob_ids(
		Blobtree* blob,
		DepthtreeWorkspace *pworkspace
		);

FORCEINLINE
Tree* find_depthtree(
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char *depth_map,
		const unsigned int stepwidth,
		DepthtreeWorkspace *workspace,
		Blob** tree_data );


unsigned int inline getRealId( unsigned int * const comp_same, unsigned int const id ){
	unsigned int rid1, rid2;
	rid1 = *(comp_same + id);
	if( (rid2 = *(comp_same + rid1)) != rid1 ){
		VPRINTF("Map %i from %i ", id, rid1);
		rid1 = rid2;
		while( (rid2 = *(comp_same + rid1)) != rid1 ){
			rid1 = rid2;
		}
		*(comp_same + id) = rid2;
		/* Attention, only the mapping of id are changed to rid2.
		 * Other elements of the while loop not. Thus,
		 * the calling of comp_same for all Ids does in general not
		 * transform comp_same to a projection.
		 *
		 */

		/*unsigned int rid0 = id;
		rid1 = *(comp_same + rid0);
		while( rid2 != rid1 ){
		 *(comp_same + rid0) = rid2;
			rid0 = rid1;
			rid1 = *(comp_same + rid1);
		}*/
		/* This approach would be more correct, but comp_same
		 * is still no projection because there is no guarantee
		 * that getRealId will be called for all ids!
		 * => Do not lay on the projection property here.
		 * comp_same will be modified during postprocessing.
		 */

		VPRINTF("to %i\n", rid2);
	}
	return rid1;
}

unsigned int inline getRealParent( unsigned int * const prob_parent, unsigned int * const comp_same, unsigned int const id ){
			return getRealId( comp_same, *(prob_parent + id) );
}


//------------------------------------

void depthtree_find_blobs(
		Blobtree *blob,
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char *depth_map,
		DepthtreeWorkspace *workspace
		);


#ifdef EXTEND_BOUNDING_BOXES
void extend_bounding_boxes( Tree * const tree);
#endif


#ifdef __cplusplus
}
#endif


#endif
