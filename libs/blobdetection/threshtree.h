#ifndef THRESHTREE_H
#define THRESHTREE_H

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
	unsigned int *real_ids;
	unsigned int *real_ids_inv;

#ifdef BLOB_BARYCENTER
	BLOB_BARYCENTER_TYPE *pixel_sum_X; //summation of all x coordinates for an id.
	BLOB_BARYCENTER_TYPE *pixel_sum_Y; //summation of all x coordinates for an id.
#endif
#ifdef BLOB_SUBGRID_CHECK
	unsigned char *triangle;
	size_t triangle_len;
#endif

	//extra data
	unsigned int *blob_id_filtered; //like comp_same, but respect blob tree filter.

} ThreshtreeWorkspace;


bool threshtree_create_workspace(
		const unsigned int w, const unsigned int h,
		ThreshtreeWorkspace **pworkspace
		);
bool threshtree_realloc_workspace(
		const unsigned int max_comp,
		ThreshtreeWorkspace **pworkspace
		);
void threshtree_destroy_workspace(
		ThreshtreeWorkspace **pworkspace
		);

/* See depthtree equivalent for explanation */
void threshtree_filter_blob_ids(
		Blobtree* blob,
		ThreshtreeWorkspace *pworkspace
		);

/* Find Blobs. 
 *
 * Assumption: All border elements x fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *   0---0 
 * h | A |  }data
 *   0---0 
 *     w
 *
 * Parameter:
 * 	In: 
 * 		-unsigned char *data: Image data
 * 		-unsigned int w, unsigned int h: Dimension of image
 * 		-unsigned char thresh: Distinct values in x>thresh and x<=thresh.
 * 	Out:
 * 		-Blob **tree_data: 
 * 	Return: 
 * 		-node*: Pointer to the array of nodes. 
 * 		 It's the pointer to the root node, too. If all assumtions fulfilled, node has
 * 		 exact one child which represent the full area.
 *
 */
Tree* find_connection_components(
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const unsigned char thresh,
		Blob **tree_data,
		ThreshtreeWorkspace *workspace );

 
/* Find Blobs. With region of interrest (roi).
 *
 * Assumption: All border elements x of roi fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *  ____________________
 * |                    |
 * |            0---0   |
 * | roi.height | A |   |  } data
 * |            0---0   |
 * |         roi.width  |
 * |____________________|
 *
 * Parameter:
 * 	In: 
 * 		-unsigned char *data: Image data
 * 		-unsigned int w, unsigned int h: Dimension of image
 * 		-BlobtreeRect roi: Simple struct to describe roi, {x,y,width,height}.
 * 		-unsigned char thresh: Distinct values in x>thresh and x<=thresh.
 * 	Out:
 * 		-Blob **tree_data: 
 * 	Return: 
 * 		-node*: Pointer to the array of nodes. 
 * 		 It's the pointer to the root node, too. If all assumtions fulfilled, node has
 * 		 exact one child which represent the full area.
 *
 */
Tree* find_connection_components_roi(
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		Blob **tree_data,
		ThreshtreeWorkspace *workspace );


/* Find Blobs. With flexible stepwidth and
 *  region of interrest (roi).
 *  Only an subset of all pixels (grid structure) will checked.
 *  Stepwidth and stepheight control the mesh size.
 *  Attention: The founded bounding box values are not exact.
 *
 * Assumption: All border elements x of roi fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *  ____________________
 * |                    |
 * |            0---0   |
 * | roi.height | A |   |  } data
 * |            0---0   |
 * |         roi.width  |
 * |____________________|
 *
 * Parameter:
 * 	In: 
 * 		-unsigned char *data: Image data
 * 		-unsigned int w, unsigned int h: Dimension of image
 * 		-BlobtreeRect roi: Simple struct to describe roi, {x,y,width,height}.
 * 		-char thresh: Distinct values in x>thresh and x<=thresh.
 * 		-unsined int stepwidth: 
 * 	Out:
 * 		-Blob **tree_data: 
 * 	Return: 
 * 		-node*: Pointer to the array of nodes. 
 * 		 It's the pointer to the root node, too. If all assumtions fulfilled, node has
 * 		 exact one child which represent the full area.
 *
 */
Tree* find_connection_components_coarse(
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		const unsigned int stepwidth,
		const unsigned int stepheight,
		Blob **tree_data,
		ThreshtreeWorkspace *workspace );

/* Let compiler optimize code for fixed stepwidth and stepheight */
FORCEINLINE
Tree* find_connection_components_coarse2(
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		const unsigned int stepwidth,
		const unsigned int stepheight,
		Blob **tree_data,
		ThreshtreeWorkspace *workspace );

#ifdef BLOB_SUBGRID_CHECK 
/* Find Blobs. With flexible stepwidth and
 *  region of interrest (roi).
 *  An subset of all pixels (grid structure) will checked.
 *  Stepwidth controls the mesh size. If an blob was detected
 *  the neighbourhood will regard to get the exact blob dimensions.
 *  Nevertheless, there is no quarantee to recognise fine structures.
 *
 * Assumption: All border elements x of roi fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *  ____________________
 * |                    |
 * |            0---0   |
 * | roi.height | A |   |  } data
 * |            0---0   |
 * |         roi.width  |
 * |____________________|
 *
 * Parameter:
 * 	In: 
 * 		-unsigned char *data: Image data
 * 		-unsigned int w, unsigned int h: Dimension of image
 * 		-BlobtreeRect roi: Simple struct to describe roi, {x,y,width,height}.
 * 		-char thresh: Distinct values in x>thresh and x<=thresh.
 * 		-unsigned int stepwidth: 
 * 	Out:
 * 		-Blob **tree_data: 
 * 	Return: 
 * 		-node*: Pointer to the array of nodes. 
 * 		 It's the pointer to the root node, too. If all assumtions fulfilled, node has
 * 		 exact one child which represent the full area.
 *
 */

Tree* find_connection_components_subcheck(
		const unsigned char *data,
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		const unsigned int stepwidth,
		Blob **tree_data,
		ThreshtreeWorkspace *workspace );
#endif

/* Main function to eval blobs */
void threshtree_find_blobs( Blobtree *blob, 
		const unsigned char *data, 
		const unsigned int w, const unsigned int h,
		const BlobtreeRect roi,
		const unsigned char thresh,
		ThreshtreeWorkspace *workspace );

#ifdef __cplusplus
}
#endif



#endif
