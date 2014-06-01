#include <stdio.h>
#include <math.h>

#define INLINE inline
#include "tree.h"

/* Allocate tree struct. If you use
 * the data pointer of the nodes you has to
 * setup/handle the storage for this data
 * separatly */
Tree *tree_create(unsigned int size){
	Tree *tree = (Tree*) malloc(sizeof(Tree));
	tree->root = (Node*) malloc( size*sizeof(Node));
	tree->size = size;
	return tree;
}


/* Dealloc tree. Attention, target of data pointer is not free'd. */
void tree_destroy(Tree **ptree){
	if( *ptree == NULL ) return;
	Tree *tree = *ptree;
	if( tree->root != NULL ){
		free( tree->root );
		tree->root = NULL;
		tree->size = 0;
	}
	free(tree);
	*ptree = NULL;
}


/* Eval height and number of children for each Node */
void gen_redundant_information(Node * const root, unsigned int *pheight, unsigned int *psilbings){
	Node *node = root;
	do{
		/* Reset values */
		node->height = 0;
		node->width = 0;

		/* To to next node. update parent node if possible */
		if( node->child != NULL ){
			node->width++;
			//node->height++;
			node = node->child;
			continue;
		}else if( node->silbing != NULL ){
			if( node->parent->height < node->height+1 ){
				node->parent->height = node->height+1;
			}
			node->parent->width++;
			node = node->silbing;
			continue;
		}else{
			while( node->parent != NULL ){
				if( node->parent->height < node->height+1 ){
					node->parent->height = node->height+1;
				}
				node = node->parent;
				if( node->silbing != NULL ){
					node->parent->width++;
					node = node->silbing;
					break;
				}
			}
		}
	}while( node != root );

}


/* Eval height and number of children for each Node */
void gen_redundant_information_recursive(Node* root, unsigned int *pheight, unsigned int *psilbings){
	root->width = 0;
	if( root->child != NULL ){
		unsigned int height2=0;
		gen_redundant_information(root->child, &height2, &root->width);
		if( *pheight < height2+1 ) *pheight = height2+1;//update height of parent node
	}
	if( root->silbing != NULL ){
		(*psilbings)++;//update number of children for parent node
		gen_redundant_information(root->silbing,pheight,psilbings);
	}
}


void add_child(Node *parent, Node *child){
	if( parent->child == NULL ){
		parent->child = child;
	}else{
		Node *cur = parent->child;
		while( cur->silbing != NULL ){
			cur = cur->silbing;
		}
		cur->silbing = child;
	}
	//set parent of child
	child->parent = parent;

	//update redundant information
	parent->width++;
	Node *p=parent, *c=child;
	while( p != NULL && p->height < c->height+1 ){
		p->height = c->height+1;
		c=p;
		p=p->parent;
	}
}


unsigned int number_of_nodes(Node *root){
 unsigned int n = 1;
 Node *cur = root->child;
 while( cur != NULL ){
	 n++;
	 if( cur->child != NULL ) cur = cur->child;
	 else if( cur->silbing != NULL ) cur = cur->silbing;
	 else{
		 cur = cur->parent;
		 while(cur != root && cur->silbing == NULL) cur = cur->parent;
		 cur = cur->silbing;
	 }
 }
 return n;
}


void print_tree(Node *root, int shift){
	return print_tree_filtered(root,shift,0);
}


void print_tree_filtered(Node *root, int shift, unsigned int minA){
	int i;
	int shift2=0;
	Blob* data = (Blob*)root->data;
	//printf("• ");
	//printf("%u (%u) ",root->data.id, root->data.area);
	//printf("%2i (w%u,h%u,a%2i) ",root->data.id, root->width, root->height, root->data.area);
	//shift2+=9+4;
#ifdef SAVE_DEPTH_MAP_VALUE
	printf("%2i (lvl:%3i, a:%4i) ",data->id, data->depth_level, data->area);
	//printf("%2i (wxh:%3i, a:%4i) ",data->id, data->roi.width*data->roi.height, data->area);
	shift2+=22;
#else
	printf("%2i (area:%4i) ",data->id, data->area);
	shift2+=9+6;
#endif

	if( data->area < minA){
		printf("\n");
		return;
	}
	if( root->child != NULL){
		printf("→");
		print_tree_filtered(root->child,shift+shift2,minA);
	}else{
		printf("\n");
	}

	if( root->silbing != NULL){
	//	printf("\n");
		for(i=0;i<shift-1;i++) printf(" ");
		printf("↘");
		print_tree_filtered(root->silbing,shift,minA);
	}
}


void quicksort_silbings(Node **begin, Node **end) {
    Node **ptr;
    Node **split;
    if (end - begin <= 1)
        return;
    ptr = begin;
    split = begin + 1;
    while (++ptr != end) {
        //if ( **ptr < **begin ) {
				if ( cmp(*ptr,*begin)  ) {
            swap_pnode(ptr, split);
            ++split;
        }
    }
    swap_pnode(begin, split - 1);
    quicksort_silbings(begin, split - 1);
    quicksort_silbings(split, end);
}


/* Sorting the nodes such that topological equal trees has
 * the same image. The algorithm sort rekursivly all children
 * and then the silbings with quicksort.
 *
 * */
void sort_tree(Node *root){
	 	if( root->width == 0) return;//leaf reached
	  /* Sort children and store pointer to this children */
	  Node** children = (Node**) malloc( root->width*sizeof(Node*) );
		Node** next = children;
		Node* c = root->child;
		while( c != NULL ){
			sort_tree(c);
			*next=c;
			c = c->silbing;
			next++;
		}
		//now, next points behind children array
		if( root->width > 1){
			Node** end = next;

			quicksort_silbings(children, end);

			//rearange children of root like sorted array propose.
			c = *children;
			root->child = c;
			next = children+1;
			while(next<end){
				c->silbing = *next;
				c=*next;
				next++;
			}
			c->silbing = NULL;//remove previous anchor in last child.
		}

		free( children );
 }



/* Generate unique id for sorting trees.
 * [DE] Wird ein Baum durchlaufen und für jeden Schritt angegeben, ob als nächstes ein
 * Kindknoten oder der nächste Geschwisterknoten auf x-ter Ebene ist, so entsteht eine
 * eindeutige Id eines Baumes. Diese kann später für vergleiche genutzt werdne.
 * Kann man das noch komprimieren, wenn man als Basis die maximale Tiefe wählt?!
 *
 * */
void _gen_tree_id(Node *root,unsigned int **id, unsigned int *d){
	if( root->child != NULL ){
		//printf("o ");
		**id = 0;
		(*id)++;//set pointer to next array element
		_gen_tree_id(root->child, id, d);
	}else{
		*d = root->height;//store height of leaf.
	}
	if( root->silbing != NULL ){
		//print difference from last leaf and this node and add 1
		//printf("%u ", root->height -*d +1 );
		**id = (root->height - *d + 1);
		(*id)++;//set pointer to next array element
		*d=root->height;
		_gen_tree_id(root->silbing, id, d);
	}
}


/* Generate Unique Number [xyz...] for node
 * Preallocate id-array with #nodes(root).
 * */
void gen_tree_id(Node *root, unsigned int* id, unsigned int size){
 unsigned int last_height=0;
 //store size of array in first element
 *id = size;
 id++;
 _gen_tree_id(root,&id,&last_height);
 printf("\n");
}


#ifdef BLOB_COUNT_PIXEL
unsigned int sum_areas(Node * const root, const unsigned int * const comp_size){

#if 1
	Node *node = root;
	Blob* data = (Blob*)node->data;
	if( root->child == NULL){
		data->area = *(comp_size + data->id );
		return data->area;
	}

	do{
		data->area = *(comp_size + data->id );

		/* Go to next node. update parent node on uprising flank */
		if( node->child != NULL ){
			node = node->child;
			data = (Blob*)node->data;
			continue;
		}
		
		((Blob*)node->parent->data)->area += data->area;

		if( node->silbing != NULL ){
			node = node->silbing;
			data = (Blob*)node->data;
			continue;
		}

		while( node != root ){
			node = node->parent;
			data = (Blob*)node->data;
			if(node != root ){
				((Blob*)node->parent->data)->area += data->area;
			}
			if( node->silbing != NULL ){
				node = node->silbing;
				data = (Blob*)node->data;
				break;
			}
		}

	}while( node != root );
	return data->area;
#else
	/* Recursive formulation */
    Blob* data = (Blob*)root->data;
	int *val=&data->area;
	*val = *(comp_size + data->id );
	if( root->child != NULL) *val += sum_areas(root->child,comp_size);
	if( root->silbing != NULL) return *val+sum_areas(root->silbing, comp_size);
	else return *val;
#endif
}

#ifdef BLOB_DIMENSION

/*
 * Let i be the id of current blob. We search
 * • A = Number of pixels of current blob.
 *
 * The Problem: We do not has information about
 * every pixel because only the pixels of the coarse
 * grid was processed.
 *
 * Solution: Use the counting on the coarse values
 * and the bounding boxes to estimate the correct
 * values. Use the fact, that bounding boxes of children
 * propagate the correct values on subsets.
 *
 * Definition of algorithm:
 *
 * C - (Coarse) Bounding box of current blob
 * F - (Fine) Bounding box(es) of child(ren) blob(s).
 *  ┌──────┐
 *  │┌─┐   │
 *  ││F│ C │
 *  │└─┘   │
 *  └──────┘
 *
 * • coarse pixel: Pixel which is part of the coarse grid
 *         => p.x%stepwidth = 0 = p.y%stepwidth
 *
 * • A_C - Area of coarse box a.k.a. number of pixels in this box.
 * • A_F - Sum of areas of fine boxes.
 * • N_C - Number of coarse pixels in C.
 *         ( approx. A_C/stepwidth^2. )
 * • N_F - Number of coarse pixels in all children bounding boxes.
 *
 * • S_C - Number of coarse pixels in the blob.
 *         ( This value would stored in data->area after the call
 *           of sum_areas, but we eval it on the fly, here. )
 * • S_F - Number of coarse pixels of children blobs.
 *
 * Fine bounding boxes are complete in the blob, thus
 * => A = A_F + X
 *
 * Y := N_F - S_F is the number of coarse pixels in F which are not
 *      in one of the children blobs.
 *
 * Estimation of X:
 *
 *	X = (A_C - A_F) * (S_C - Y)/(N_C - N_F)
 *
 * Moreover, S_C-Y = 2S_F + comp_size(i) - N_F.
 *
 * The algorithm loops over every node and eval
 * 2S_F + comp_size(i) - N_F, which will stored in node->data->area.
 * After all children of a node was processed the approimation
 * starts, which will replace node->data->area.
 *
 * */
static inline unsigned int number_of_coarse_roi(BlobtreeRect* roi, unsigned int sw, unsigned int sh);

void approx_areas(const Tree * const tree, Node * const startnode,
		const unsigned int * const comp_size,
		const unsigned int stepwidth, const unsigned int stepheight)
{

	Node *node = startnode;
	Node *root = tree->root;
	Blob* data = (Blob*)node->data;

	if( node->child == NULL ){
		//tree has only one node: startnode.
		const unsigned int N_C = number_of_coarse_roi(&data->roi, stepwidth, stepheight);
		const unsigned int A_C = (data->roi.width*data->roi.height);
		data->area = A_C * ((float)data->area/N_C);
		return;
	}

	/*store A_F= Sum of areas of children bounding boxes.
	 * We use *(pA_F +(node-root) ) for access. Thus, we
	 * need the root node of the tree as anchor (or doubles the array size)
	 * to avoid access errors.
	 * */
	unsigned int * const pA_F = (unsigned int*) calloc(tree->size, sizeof(unsigned int) );

	do{
		data->area = *(comp_size + data->id );

		/* To to next node. update parent node on uprising flank */
		if( node->child != NULL ){
			node = node->child;
			data = (Blob*)node->data;
			continue;
		}

		printf("Id: %u Roi: (%u,%u,%u,%u)\n",data->id,
				data->roi.x, data->roi.y, data->roi.width, data->roi.height);
		const unsigned int N_C = number_of_coarse_roi(&data->roi, stepwidth, stepheight);
		const unsigned int A_C = (data->roi.width*data->roi.height);
		printf("N_C=%u, A_C=%u\n\n", N_C, A_C);


		/* Update parent node. N_C,A_C of this level is part of N_F, A_F from parent*/
		((Blob*)node->parent->data)->area += (data->area <<1) - N_C;
		*(pA_F + (node->parent - root) ) += A_C;

		/*Eval approximation, use A = A_C * S_C/N_C (for leafs is A_F=N_F=S_F=0 ) */
		if( N_C ){
			data->area = A_C * ((float)data->area/N_C) + 0.5f;
		}else{
			data->area = 0;//area contains only subpixel
		}

		if( node->silbing != NULL ){
			node = node->silbing;
			data = (Blob*)node->data;
			continue;
		}

		while( node->parent != root ){
			/*
			 * All children of parent processed.
			 * We can start the approximation for the parent node.
			 * */
			node = node->parent;
			data = (Blob*)node->data;

			const unsigned int N_C = number_of_coarse_roi(&data->roi, stepwidth, stepheight);
			const unsigned int A_C = (data->roi.width*data->roi.height);

			if( node!=startnode ){//required to avoid changes over startnode
				/* Update parent node. N_C,A_C of this level is part of N_F, A_F from parent*/
				((Blob*)node->parent->data)->area += (data->area <<1) - N_C;
				*(pA_F + (node->parent - root) ) += A_C;
			}

			const unsigned int A_F = *(pA_F + (node - root) );
			/* A = A_F + (A_C - A_F) * (2*S_F + comp_size(i) - N_F) */
			if( N_C ){
				data->area = A_F + (A_C - A_F) * ((float)data->area/N_C) +0.5f;
			}else{
				data->area = 0;//area contains only subpixel
			}

			if( node->silbing != NULL ){
				node = node->silbing;
				data = (Blob*)node->data;
				break;
			}
		}

	}//while( node != startnode );
	while( node->parent != root );

	free( pA_F);
}

/* Returns the number of coarse pixels of a roi, see sketch for 
	 stepwidth=3=stepheight, W=10=H and roi={4,0,4,7}:

	 x - - 0 1 0 0 - x -
	 - - - 0 0 0 0 - - -
	 - - - 0 0 0 0 - - -
	 - - - 0 0 0 0 - - -
	 x - - 0 1 0 0 - x -
	 - - - 0 0 0 0 - - -
	 - - - 0 0 0 0 - - -
	 - - - - - - - - - -
	 x - - - x - - - x -
	 - - - - - - - - - -
*/
static inline unsigned int number_of_coarse_roi(BlobtreeRect* roi, unsigned int sw, unsigned int sh){
	/* Note:
	 * Three steps for each dimension of [a1,b1]x[a2,b2], a_i < b_i (not <= !)
	 * 1. Shift roi to [0,b-a]
	 * 2. Decrease length if startpoint is not on coarse grid (a%sw!=0)
	 *    => [0,b-a-m]
	 * 3. Divide by stepwidth, stepheight [0,(b-a-m)/sw]
	 * 4. Transform   (0) 0 0 1 1 1 2 2 2 … (Interval length remainder)
	 *            to  (1) 1 1 2 2 2 3 3 3
	 *            (This will be done by adding sw-1 before step 3.)
	 *    
	 * Do not cut of +sw in (... +sw)%sw because -1%sw = -1 != sw-1 !!
	 * */
	return (  (roi->width + sw-1 - (sw-1-(roi->x +sw-1 )%sw)  ) /sw)
		*(  (roi->height + sh-1 - (sh-1-(roi->y +sh-1 )%sh)  ) /sh);
}

#endif //BLOB_DIMENSION
#endif //BLOB_COUNT_PIXEL

#ifdef BLOB_BARYCENTER
/* 
 * This functions loops through the tree. If all children of
 * of a node P was handled, the values pixel_sum_*[P.id] will
 * be accumulate by the values of the children.
 * Finally, the barycenter will be evaluated.
 *
 * Notes:
 * - This function SUPERSEDS sum_areas and changes the area value, too.
 *   => Call other functions, which set the area value (i.e. approx areas)
 *   after this function.
 * - This function changes the values of the arguments pixel_sum_*.
 * - This function requires the values of comp_size because the 
 *   value of [node]->data->area could be unusable (for stepwidth>1).
 *   Thats the reason for setting the area value during the loop, too.
 * */
void eval_barycenters( Node *const start_node,
		const Node * const root,
		const unsigned int * const comp_size,
		BLOB_BARYCENTER_TYPE * const pixel_sum_X,
		BLOB_BARYCENTER_TYPE * const pixel_sum_Y
		){

	Node *node = start_node;
	Blob *data = (Blob*)node->data;
	Blob *parentdata;
	if( node->child == NULL){
		data->area = *(comp_size + data->id );
		data->barycenter[0] = (*(pixel_sum_X + data->id )+(data->area>>1)) / data->area;
		data->barycenter[1] = (*(pixel_sum_Y + data->id )+(data->area>>1)) / data->area;
		return data->area;
	}

	do{
		data->area = *(comp_size + data->id );

		/* Go to next node. update parent node on uprising flank */
		if( node->child != NULL ){
			node = node->child;
			data = (Blob*)node->data;
			continue;
		}

		//Node is Leaf
#if 1
		data->barycenter[0] = (*(pixel_sum_X + data->id )+(data->area>>1)) / data->area;
		data->barycenter[1] = (*(pixel_sum_Y + data->id )+(data->area>>1)) / data->area;
#else
		data->barycenter[0] = round( *(pixel_sum_X + data->id )*1.0 / data->area);
		data->barycenter[1] = round( *(pixel_sum_Y + data->id )*1.0 / data->area);
#endif

		//((Blob*)node->parent->data)->area += data->area;
		parentdata = (Blob*)node->parent->data;
		parentdata->area += data->area;
		*(pixel_sum_X + parentdata->id ) += *(pixel_sum_X + data->id );
		*(pixel_sum_Y + parentdata->id ) += *(pixel_sum_Y + data->id );

		if( node->silbing != NULL ){
			node = node->silbing;
			data = (Blob*)node->data;
			continue;
		}

		while( node->parent != root ){
			node = node->parent;
			data = (Blob*)node->data;

			// All children was handled 
#if 1
		data->barycenter[0] = (*(pixel_sum_X + data->id )+(data->area>>1)) / data->area;
		data->barycenter[1] = (*(pixel_sum_Y + data->id )+(data->area>>1)) / data->area;
#else
			data->barycenter[0] = round( *(pixel_sum_X + data->id )*1.0 / data->area);
			data->barycenter[1] = round( *(pixel_sum_Y + data->id )*1.0 / data->area);
#endif

			if(node->parent != root ){
				parentdata = (Blob*)node->parent->data;
				parentdata->area += data->area;
				*(pixel_sum_X + parentdata->id ) += *(pixel_sum_X + data->id );
				*(pixel_sum_Y + parentdata->id ) += *(pixel_sum_Y + data->id );
			}
			if( node->silbing != NULL ){
				node = node->silbing;
				data = (Blob*)node->data;
				break;
			}
		}

	}while( node->parent != root );

}
#endif

#ifdef BLOB_DIMENSION
/* Assume type(data) = Blob* */
void set_area_prop(Node * const root){
#if 1
	Node *node = root;
	Blob *data;
	do{
		data = (Blob*)node->data;
		data->area = data->roi.width * data->roi.height;
		if( node->child != NULL){
			node = node->child;
		}else if( node->silbing != NULL ){
			node = node->silbing;
		}else{
			while( node != root ){
				node = node->parent;
				if( node->silbing != NULL ){
					node = node->silbing;
					break;
				}
			}
		}
	}while( node != root );

#else
	/* Recursive formulation */
  Blob* data = (Blob*)root->data;
	data->area = data->roi.width * data->roi.height;
	if( root->child != NULL) set_area_prop(root->child);
	if( root->silbing != NULL) set_area_prop(root->silbing);
#endif
}
#endif



/* tree_hashval, (NEVER FINISHED)
 * Berechnet für einen Baum eine Id, die eindeutig ist,
 * wenn die Bäume eine bestimmte Struktur einhalten.
 * Die Struktur der Bäume (z.B. max. Anzahl der Kinder)
 *  ist grob einstellbar.
 */
static const unsigned int TREE_CHILDREN_MAX = 5;
static const unsigned int TREE_DEPTH_MAX = 5;//height of root is 0.

static unsigned int tree_hashval( Node *root){
	return -1;
}






// Debug/Helper-Functions

char * debug_getline(void) {
    char * line = (char*) malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            char * linen = (char*) realloc(linep, lenmax *= 2);
            len = lenmax;

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
free(linep);
    return linep;
}


void debug_print_matrix( unsigned int* data, unsigned int w, unsigned int h, BlobtreeRect roi, unsigned int gridw, unsigned int gridh){
	unsigned int i,j, wr, hr, w2, h2;
	unsigned int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			d = *(data+i*w+j);
			//printf("%u ",d);
			//printf("%s", d==0?"■⬛":"□");
			//printf("%s", d==0?"✘":" ");
			if(d>0)
				//printf("%s%u",d<10&&d>=0?" ":"", d);
				printf("%3i", d);
			else
				printf("   ");
		}
		j-=gridw-wr;

		if(w2<roi.width){
			for(;j<roi.x+roi.width;j+=1){
				d = *(data+i*w+j);
				if(d>0)
					printf("%s%u",d<10&&d>=0?" ":"", d);
				else
					printf("  ");
			}
		}
		
		printf("\n");
	}

	i-=gridh-hr;
	if( h2 < roi.height ){
		for( ;i<roi.y+roi.height;i+=1){
			for(j=roi.x;j<roi.x+w2;j+=gridw){
				d = *(data+i*w+j);
				if(d>0)
					printf("%s%u",d<10&&d>=0?" ":"", d);
				else
					printf("  ");
			}
			j-=gridw-wr;

			if(w2<roi.width){
				for(;j<roi.x+roi.width;j+=1){
					d = *(data+i*w+j);
					if(d>0)
						printf("%s%u",d<10&&d>=0?" ":"", d);
					else
						printf("  ");
				}
			}
			printf("\n");
		}
	}
	printf("\n");
}


void debug_print_matrix2(unsigned int* ids, unsigned int* data, unsigned int w, unsigned int h, BlobtreeRect roi, unsigned int gridw, unsigned int gridh, char twice){
	unsigned int i,j, wr, hr, w2, h2;
	unsigned int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			if( *(ids+i*w+j) > 0 ){
				d = *(data+*(ids+i*w+j));
				if(twice) d=*(data+d);
				printf("%s%u",d<10&&d>=0?" ":"", d);
			}else{
				printf("  ");
			}
		}
		j-=gridw-wr;

		if(w2<roi.width){
			for(;j<roi.x+roi.width;j+=1){
				if( *(ids+i*w+j) > 0 ){
					d = *(data+*(ids+i*w+j));
					if(twice) d=*(data+d);
					printf("%s%u",d<10&&d>=0?" ":"", d);
				}else{
					printf("  ");
				}
			}
		}
		
		printf("\n");
	}

	i-=gridh-hr;
	if( h2 < roi.height ){
		for( ;i<roi.y+roi.height;i+=1){
			for(j=roi.x;j<roi.x+w2;j+=gridw){
				if( *(ids+i*w+j) > 0 ){
					d = *(data+*(ids+i*w+j));
					printf("%s%u",d<10&&d>=0?" ":"", d);
				}else{
					printf("  ");
				}
			}
			j-=gridw-wr;

			if(w2<roi.width){
				for(;j<roi.x+roi.width;j+=1){
					if( *(ids+i*w+j) > 0 ){
						d = *(data+*(ids+i*w+j));
						printf("%s%u",d<10&&d>=0?" ":"", d);
					}else{
						printf("  ");
					}
				}
			}
			printf("\n");
		}
	}
	printf("\n");
}


void debug_print_matrix_char( unsigned char * data, unsigned int w, unsigned int h, BlobtreeRect roi, unsigned int gridw, unsigned int gridh){
	unsigned int i,j, wr, hr, w2, h2;
	unsigned int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			d = *(data+i*w+j);
			//printf("%u ",d);
			//printf("%s", d==0?"■⬛":"□");
			//printf("%s", d==0?"✘":" ");
			if(d>0)
				printf("%s%u",d<10&&d>=0?" ":"", d);
			else
				printf("  ");
		}
		j-=gridw-wr;

		if(w2<roi.width){
			for(;j<roi.x+roi.width;j+=1){
				d = *(data+i*w+j);
				if(d>0)
					printf("%s%u",d<10&&d>=0?" ":"", d);
				else
					printf("  ");
			}
		}
		
		printf("\n");
	}

	i-=gridh-hr;
	if( h2 < roi.height ){
		for( ;i<roi.y+roi.height;i+=1){
			for(j=roi.x;j<roi.x+w2;j+=gridw){
				d = *(data+i*w+j);
				if(d>0)
					printf("%s%u",d<10&&d>=0?" ":"", d);
				else
					printf("  ");
			}
			j-=gridw-wr;

			if(w2<roi.width){
				for(;j<roi.x+roi.width;j+=1){
					d = *(data+i*w+j);
					if(d>0)
						printf("%s%u",d<10&&d>=0?" ":"", d);
					else
						printf("  ");
				}
			}
			printf("\n");
		}
	}
	printf("\n");
}
