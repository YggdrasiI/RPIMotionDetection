//#include "RaspiVid.h"
#include "norm2.h"
#include "RaspiImv.h"

MOTION_DATA motion_data = {0, NULL, NULL, 0,0,0,0 };

int init_motion_data(MOTION_DATA *md, RASPIVID_STATE *state){
	
	//prefill some global arrays for fast 2-norm evaluation.
	norm2_init_arrays();

	md->width = (state->width+15)/16 + 1;//1920 => 121
	md->height = (state->height+15)/16;
	md->available = 0;
	md->mutex = 0;

	//md->imv_array_len = 120*68*4;
	if( md->width < 0 || md->height < 0 ){
		fprintf(stderr, "%s: Input dimension negative.", __func__);
		return -2;
	}
	md->imv_array_len = md->width * md->height;
	md->imv_array_in = (char*) malloc( md->imv_array_len * sizeof(INLINE_MOTION_VECTOR) );
	md->imv_array_buffer = (char*) malloc( md->imv_array_len * sizeof(INLINE_MOTION_VECTOR) );
	md->imv_norm = (unsigned char*) malloc( md->imv_array_len );
	if( md->imv_array_in == NULL
			|| md->imv_array_buffer == NULL 
			|| md->imv_norm == NULL )
	{
		uninit_motion_data(md, state);
		return -1;
	}
	return 0;
}

void uninit_motion_data(MOTION_DATA *md, RASPIVID_STATE *state){
	md->imv_array_len = 0;
	free( md->imv_array_in); md->imv_array_in = NULL;
	free( md->imv_array_buffer); md->imv_array_buffer = NULL;
	free( md->imv_norm); md->imv_norm = NULL;
}

//uses global var.
void handle_imv_data(char *data, size_t data_len){
	if( data_len <= motion_data.imv_array_len * sizeof(INLINE_MOTION_VECTOR) ){
		memcpy( motion_data.imv_array_in, data, data_len);
		while(motion_data.mutex){}
		motion_data.mutex = 1;
		motion_data.available = 1;
		motion_data.mutex = 0;
	}
}

/* Eval 1-norm of imv vector. 
 * Note: x=y=128 (signed char) will be mapped on 0=256 (unsigned char), 
 * but this motion will not occur.
 * */
void imv_eval_norm(MOTION_DATA *md){
	INLINE_MOTION_VECTOR *curImv = (INLINE_MOTION_VECTOR*) md->imv_array_buffer; 
	INLINE_MOTION_VECTOR *curImvEnd = curImv+md->imv_array_len;
	unsigned char* curNorm = md->imv_norm;

#if 0
#if 0
	int j,i,J,I;
	for( j=0,J=md->height; j<J; ++j){
		for( i=0,I=md->width-1; i<I; ++i){
			curNorm[i+(md->width-1)*j] = abs(curImv[i+md->width*j].x_vector)
				+ abs(curImv[i+md->width*j].y_vector);
		}
	}
#else
	int j,i,J,I;
	for( j=0,J=md->height; j<J; ++j){
		for( i=0,I=md->width; i<I; ++i){
			curNorm[i+(md->width)*j] = abs(curImv[i+md->width*j].x_vector)
				+ abs(curImv[i+md->width*j].y_vector);
		}
	}
#endif
#else
	while( curImv<curImvEnd ){
		*curNorm = abs(curImv->x_vector) + abs(curImv->y_vector);
		++curImv;
		++curNorm;
	}
#endif

}

/* Eval 2-norm of imv vector. 
 * */
void imv_eval_norm2(MOTION_DATA *md){
	INLINE_MOTION_VECTOR *curImv = (INLINE_MOTION_VECTOR*) md->imv_array_buffer; 
	INLINE_MOTION_VECTOR *curImvEnd = curImv+md->imv_array_len;
	unsigned char* curNorm = md->imv_norm;

	while( curImv<curImvEnd ){
		*curNorm = norm2(curImv->x_vector,curImv->y_vector);
		++curImv;
		++curNorm;
	}

}

/* 
 * avg_direction: avg. x and y value for md.
 * support_len: Number of pixels != 0
 * variance: E(X^2) - E(X)^2
 */
void imv_eval_avg_direction(MOTION_DATA *md, AVG_DIRECTION_DATA *ret){
	INLINE_MOTION_VECTOR *curImv = (INLINE_MOTION_VECTOR*) md->imv_array_buffer; 
	INLINE_MOTION_VECTOR *curImvEnd = curImv+md->imv_array_len;
	/* This values should not overflow because the input dimensions are holding
	 * width*height*range << 2^31.
	 * */
	int sum_x = 0, sum_y = 0;
	int sum_x2 = 0, sum_y2 = 0;
	size_t num_not_zero = 0;

	while( curImv<curImvEnd ){
			if( curImv->x_vector || curImv->y_vector ){
					++num_not_zero;
					sum_x += curImv->x_vector;
					sum_x2 += *(norm2_pow_map+(unsigned char)curImv->x_vector);
					sum_y += curImv->y_vector;
					sum_y2 += *(norm2_pow_map+(unsigned char)curImv->y_vector);
					++curImv;
			}
	}
	ret->direction_avg[0] = ((double)sum_x)/num_not_zero;
	ret->direction_avg[1] = ((double)sum_y)/num_not_zero;
	ret->direction_variance[0] = ((double)sum_x2)/num_not_zero - ret->direction_avg[0] * ret->direction_avg[0];
	ret->direction_variance[1] = ((double)sum_y2)/num_not_zero - ret->direction_avg[1] * ret->direction_avg[1];
	ret->direction_support_len	= num_not_zero;
}
