#define INLINE inline
#include "tree.h"

/* Allocate tree struct. If you use 
 * the data pointer of the nodes you has to 
 * setup/handle the storage for this data 
 * separatly */
Tree *tree_create(int size){
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
void gen_redundant_information(Node * const root, int *pheight, int *psilbings){
	Node *node = root;
	while( node != NULL ){
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
			while( node != NULL ){
				if( node->parent->height < node->height+1 ){
					node->parent->height = node->height+1;
				}
				node = node->parent;
				if( node->silbing != NULL ){
					node->parent->width++;
					node = node->silbing;
					continue;
				}
			}
		}
	}

}


/* Eval height and number of children for each Node */
void gen_redundant_information_recursive(Node* root, int *pheight, int *psilbings){
	root->width = 0;
	if( root->child != NULL ){
		int height2=0;
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


int number_of_nodes(Node *root){
 int n = 1;
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
	return print_tree_filtered(root,shift,-1);
}


void print_tree_filtered(Node *root, int shift, int minA){
	int i;
	int shift2=0;
	Blob* data = (Blob*)root->data;
	//printf("• ");
	//printf("%i (%i) ",root->data.id, root->data.area);
	//printf("%2i (w%i,h%i,a%2i) ",root->data.id, root->width, root->height, root->data.area);
	//shift2+=9+4;
#ifdef SAVE_DEPTH_MAP_VALUE
	printf("%2i (level: %3i) ",data->id, data->depth_level);
	shift2+=17;
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
void _gen_tree_id(Node *root,int **id, int *d){
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
		//printf("%i ", root->height -*d +1 );
		**id = (root->height - *d + 1);
		(*id)++;//set pointer to next array element
		*d=root->height;
		_gen_tree_id(root->silbing, id, d);
	}
}


/* Generate Unique Number [xyz...] for node
 * Preallocate id-array with #nodes(root).
 * */
void gen_tree_id(Node *root, int* id, int size){
 int last_height=0;
 //store size of array in first element
 *id = size;
 id++;
 _gen_tree_id(root,&id,&last_height);
 printf("\n");
}


#ifdef BLOB_COUNT_PIXEL
int sum_areas(const Node *root, int *comp_size){
    Blob* data = (Blob*)root->data;
	int *val=&data->area;
	*val = *(comp_size + data->id );
	if( root->child != NULL) *val += sum_areas(root->child,comp_size);
	if( root->silbing != NULL) return *val+sum_areas(root->silbing, comp_size);
	else return *val;
}
#endif

#ifdef BLOB_DIMENSION
/* Assume type(data) = Blob* */
void set_area_prop(Node * const root){
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
					continue;
				}
			}
		}
	}while( node != root );

	/* Recursive formulation */
	/*
  Blob* data = (Blob*)root->data;
	data->area = data->roi.width * data->roi.height;
	if( root->child != NULL) set_area_prop(root->child);
	if( root->silbing != NULL) set_area_prop(root->silbing);
	*/
}
#endif



/* tree_hashval, (NEVER FINISHED)
 * Berechnet für einen Baum eine Id, die eindeutig ist, 
 * wenn die Bäume eine bestimmte Struktur einhalten.
 * Die Struktur der Bäume (z.B. max. Anzahl der Kinder)
 *  ist grob einstellbar.
 */
static const int TREE_CHILDREN_MAX = 5;
static const int TREE_DEPTH_MAX = 5;//height of root is 0.

static int tree_hashval( Node *root){
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


void debug_print_matrix( int* data, int w, int h, BlobtreeRect roi, int gridw, int gridh){
	int i,j, wr, hr, w2, h2;
	int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			d = *(data+i*w+j);
			//printf("%i ",d);
			//printf("%s", d==0?"■⬛":"□");
			//printf("%s", d==0?"✘":" ");
			if(d>-1)
				printf("%s%i",d<10&&d>=0?" ":"", d);
			else
				printf("  ");
		}
		j-=gridw-wr;

		if(w2<roi.width){
			for(;j<roi.x+roi.width;j+=1){
				d = *(data+i*w+j);
				if(d>-1)
					printf("%s%i",d<10&&d>=0?" ":"", d);
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
				if(d>-1)
					printf("%s%i",d<10&&d>=0?" ":"", d);
				else
					printf("  ");
			}
			j-=gridw-wr;

			if(w2<roi.width){
				for(;j<roi.x+roi.width;j+=1){
					d = *(data+i*w+j);
					if(d>-1)
						printf("%s%i",d<10&&d>=0?" ":"", d);
					else
						printf("  ");
				}
			}
			printf("\n");
		}
	}
	printf("\n");
}


void debug_print_matrix2(int* ids, int* data, int w, int h, BlobtreeRect roi, int gridw, int gridh, char twice){
	int i,j, wr, hr, w2, h2;
	int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			if( *(ids+i*w+j) > -1 ){
				d = *(data+*(ids+i*w+j));
				if(twice) d=*(data+d);
				printf("%s%i",d<10&&d>=0?" ":"", d);
			}else{
				printf("  ");
			}
		}
		j-=gridw-wr;

		if(w2<roi.width){
			for(;j<roi.x+roi.width;j+=1){
				if( *(ids+i*w+j) > -1 ){
					d = *(data+*(ids+i*w+j));
					if(twice) d=*(data+d);
					printf("%s%i",d<10&&d>=0?" ":"", d);
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
				if( *(ids+i*w+j) > -1 ){
					d = *(data+*(ids+i*w+j));
					printf("%s%i",d<10&&d>=0?" ":"", d);
				}else{
					printf("  ");
				}
			}
			j-=gridw-wr;

			if(w2<roi.width){
				for(;j<roi.x+roi.width;j+=1){
					if( *(ids+i*w+j) > -1 ){
						d = *(data+*(ids+i*w+j));
						printf("%s%i",d<10&&d>=0?" ":"", d);
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


void debug_print_matrix_char( unsigned char * data, int w, int h, BlobtreeRect roi, int gridw, int gridh){
	int i,j, wr, hr, w2, h2;
	int d;
	wr = (roi.width-1) % gridw;
	hr = (roi.height-1) % gridh;
	w2 = roi.width - wr;
	h2 = roi.height - hr;
	for(i=roi.y;i<roi.y+h2;i+=gridh){
		for(j=roi.x;j<roi.x+w2;j+=gridw){
			d = *(data+i*w+j);
			//printf("%i ",d);
			//printf("%s", d==0?"■⬛":"□");
			//printf("%s", d==0?"✘":" ");
			if(d>-1)
				printf("%s%i",d<10&&d>=0?" ":"", d);
			else
				printf("  ");
		}
		j-=gridw-wr;

		if(w2<roi.width){
			for(;j<roi.x+roi.width;j+=1){
				d = *(data+i*w+j);
				if(d>-1)
					printf("%s%i",d<10&&d>=0?" ":"", d);
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
				if(d>-1)
					printf("%s%i",d<10&&d>=0?" ":"", d);
				else
					printf("  ");
			}
			j-=gridw-wr;

			if(w2<roi.width){
				for(;j<roi.x+roi.width;j+=1){
					d = *(data+i*w+j);
					if(d>-1)
						printf("%s%i",d<10&&d>=0?" ":"", d);
					else
						printf("  ");
				}
			}
			printf("\n");
		}
	}
	printf("\n");
}
