
#define NEW_COMPONENT_OLD(PARENTID) \
	id++; \
*(iPi) = id; \
/* *(anchors+id) = dPi-dS; */\
*(prob_parent+id) = PARENTID; \
*(comp_same+id) = id; \
*(comp_size+id) = 0; /*Increase now every pixel. => Can't start with 1 anymore. Overhead of |ids| operations */ \
*(left_index+id) = s; \
*(right_index+id) = s; \
*(top_index+id) = z; \
*(bottom_index+id) = z; \
if( id>=max_comp ){ \
	max_comp = (unsigned int) ( (float)w*h*max_comp/(dPi-data) ); \
	VPRINTF("Extend max_comp=%i\n", max_comp); \
	threshtree_realloc_workspace(max_comp, &workspace); \
	/* Reallocation requires update of pointers */ \
		ids = workspace->ids; \
		comp_same = workspace->comp_same; \
		prob_parent = workspace->prob_parent; \
		COUNT( comp_size = workspace->comp_size; ) \
		SZ( \
		top_index = workspace->top_index; \
		left_index = workspace->left_index; \
		right_index = workspace->right_index; \
		bottom_index = workspace->bottom_index; \
		) \
}


