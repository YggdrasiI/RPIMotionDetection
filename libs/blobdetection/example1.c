#include <stdlib.h>
#include <stdio.h>

#include "threshtree.h"

static const unsigned int W=28;
static const unsigned int H=28;

#include "example.h"


int main(int argc, char **argv) {
	// Stepwidth
	unsigned int w=1,h=1;
	// Region of interest
	BlobtreeRect roi= {0,0,W,H};
	//BlobtreeRect roi= {18,0,9,17};

	if( argc > 2){
			w = atoi(argv[1]);
			h = atoi(argv[2]);
	}

	// Generate test image
	unsigned char* sw;
	sw = calloc( W*H,sizeof(unsigned char) );
	if( sw == NULL ) return -1;
	gen_image_data2(sw,W,H,4);

	printf("Input image data:\n");
	print_matrix_char_with_roi( (char*) sw,W,H,roi, 1, 1);

	//Init workspace
	ThreshtreeWorkspace *workspace = NULL;
  threshtree_create_workspace( W, H, &workspace );

	// Init blobtree struct
	Blobtree *blob = NULL;
	blobtree_create(&blob);

	// Set distance between compared pixels.	
	// Look at blobtree.h for more information.
	blobtree_set_grid(blob, w,h);

	// Now search the blobs.
	threshtree_find_blobs(blob, sw, W, H, roi, 128, workspace);

	// Print out result tree.
	printf("===========\n");
	printf("Treesize: %u, Tree null? %s\n", blob->tree->size, blob->tree->root==NULL?"Yes.":"No.");
	print_tree(blob->tree->root,0);

	// Filter results and loop over elements.
	printf("===========\n");
	blobtree_set_filter(blob, F_TREE_DEPTH_MIN, 1);
	blobtree_set_filter(blob, F_AREA_MIN, 5000);
	blobtree_set_filter(blob, F_TREE_DEPTH_MAX, 1);

	Node *cur = blobtree_first(blob);
	while( cur != NULL ){
		// bounding box
		Blob *data = (Blob*)cur->data;
		BlobtreeRect *rect = &data->roi;
		printf("Blob with id %u: x=%u y=%u w=%u h=%u area=%u\n",data->id,
				rect->x, rect->y,
				rect->width, rect->height,
				data->area
				);

		cur = blobtree_next(blob);
	}

	// Clean up.
	blobtree_destroy(&blob);
  threshtree_destroy_workspace( &workspace );

	// Free image data
	free(sw);

	return 0;
}
