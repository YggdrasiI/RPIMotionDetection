#ifndef SETTINGS_H
#define SETTINGS_H

/* 0, 1, 2(with stops) */
//#define VERBOSE 1

/* Sort tree by child node structure and area size. This provide 
	 some robustness on rotation for tree comparison. */
//#define BLOB_SORT_TREE

/* Set this env. variable to enable diagonal checks between compared pixels. */
#define BLOB_DIAGONAL_CHECK

/* Eval vertical and horizontal dimension */
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
 * area.width*area.height but not exact if stepwidth>1.
 * If stepwidth>1 and BLOB_DIMENSION is set,
 * the node.data.area value will be estimated
 * by the 'wetted area' of the bounding box and the
 * bounding boxes of children areas.
 */
#define BLOB_COUNT_PIXEL

/* Evaluate the barycenter in both directions.
 *
 * Interally all pixel positions will be summed up
 * and finally divided by the area. Thus, bigger images
 * requires the long long data type. 
 * If your images not superseeded 2000x2000 pixels your
 * are still on the secure side with unsigned long (32bit).
 *
 * Requires BLOB_COUNT_PIXEL.
 */
#ifdef BLOB_COUNT_PIXEL
#define BLOB_BARYCENTER
#define BLOB_BARYCENTER_TYPE unsigned long
#endif


/* See README
 */
//#define BLOB_SUBGRID_CHECK 

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


/* Force inlining of main function to create
 * constant stepwidth variable.
 * Increases compile time by factor 12. Coment out to disable optimization.
 * */
#define FORCEINLINE __attribute__((always_inline)) static


#if VERBOSE > 0
#define VPRINTF(...) printf(__VA_ARGS__);
#else
#define VPRINTF(...) 
#endif


/* Derive some environment variables from the above settings.
 */
#ifdef BLOB_DIMENSION ## BLOB_BARYCENTER
/* Update current position for each step */
	#define PIXEL_POSITION
#endif


#endif
