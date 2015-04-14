#include <stdlib.h>
#include <stdio.h>

#include "depthtree.h"

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
	gen_image_data3(sw,W,H,4);

	*(sw) = 3;

	printf("Input image data:\n");
	print_matrix_char_with_roi( (char*) sw,W,H,roi, 1, 1);

	//Init workspace
	DepthtreeWorkspace *workspace = NULL;
  depthtree_create_workspace( W, H, &workspace );
	if( workspace == NULL ){
		printf("Unable to create workspace.\n");
		free(sw);
		return -1;
	}

	// Init blobtree struct
	Blobtree *blob = NULL;
	blobtree_create(&blob);

	// Set distance between compared pixels.	
	// Look at blobtree.h for more information.
	blobtree_set_grid(blob, w,h);


	//Init depth_map
	unsigned char depth_map[256];
	unsigned int i; for( i=0; i<256; i++) depth_map[i] = i;

	// Now search the blobs.
	depthtree_find_blobs(blob, sw, W, H, roi, depth_map, workspace);
	
	// Print out result tree.
		printf("===========\n");
		printf("Treesize: %i, Tree null? %s\n", blob->tree->size, blob->tree->root==NULL?"Yes.":"No.");
		print_tree(blob->tree->root,0);

		// Filter results and loop over elements.
		printf("===========\n");
		blobtree_set_filter(blob, F_TREE_DEPTH_MIN, 1);
		blobtree_set_filter(blob, F_AREA_MIN, 5);
		blobtree_set_filter(blob, F_TREE_DEPTH_MAX, 1);

		Node *cur = blobtree_first(blob);
		while( cur != NULL ){
			// bounding box
			Blob *data = (Blob*)cur->data;
			BlobtreeRect *rect = &data->roi;
			printf("Blob with id %i: x=%i y=%i w=%i h=%i area=%i\n",data->id,
					rect->x, rect->y,
					rect->width, rect->height,
					data->area
					);

			cur = blobtree_next(blob);
		}

	// Clean up.
  depthtree_destroy_workspace( &workspace );

	blobtree_destroy(&blob);

	// Free image data
	free(sw);

	return 0;
}
