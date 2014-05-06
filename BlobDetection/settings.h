#ifndef SETTINGS_H
#define SETTINGS_H

/* 0, 1, 2(with stops) */
//#define VERBOSE 1

/* Force inlining of main function to create
 * constant stepwidth variable.
 * Increases compile time by factor 12. Coment out to disalbe optimization.
 * */
#define FORCEINLINE __attribute__((always_inline))

/* Sort tree by child node structure and area size. This provide 
	 some robustness on rotation for tree comparison. */
#define BLOB_SORT_TREE

/* Set this env. variable to enable diagonal checks */
#define BLOB_DIAGONAL_CHECK

/* Eval vertiacal and horizontal dimension */
#define BLOB_DIMENSION


/* Extend blob dimension of parent blobs (for depthblob algorithm).
 * If darker pixels do not sourrounding lighter pixels the bounding box
 * of the darker layer id does not superseed the lighter layer bounding box.
 * Set this env. variable to checked and fix this mostly unwanted behaviour.
 * The used algorithm is non-recursive relating to the tree depth.
 */
#define EXTEND_BOUNDING_BOXES

/* Count pixels of each area. (Stored in node.data.area).
 * This is more accurate then
 * area.width*area.height but not calculable if stepwidth>1.
 * If stepwidth>1 and BLOB_DIMENSION is set,
 * the node.data.area value will set to
 * area.width * area.height.
 * If you finally always use stepwidth>1 do not define
 * BLOB_COUNT_PIXEL to to cut of some operations.
 */
#define BLOB_COUNT_PIXEL

/* See README
 */
#define BLOB_SUBGRID_CHECK 

/* For depthtree algorithm.
 * Use identity function to distict the pixel values into
 * different depth ranges. This saves a small amount of time,
 * but the result tree of blobs can be very big on inhomogenous
 * images.
 * */
//#define NO_DEPTH_MAP


/* For depthtree algorithm.
 * Save depth_map value for each node
 * (node.data.depth)
 * */
#define SAVE_DEPTH_MAP_VALUE


#if VERBOSE > 0
#define VPRINTF(...) printf(__VA_ARGS__);
#else
#define VPRINTF(...) 
#endif

#endif
