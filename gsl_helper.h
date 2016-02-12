#ifndef GSL_HELPER_H
#define GSL_HELPER_H

#include <gsl/gsl_multifit.h>

#ifdef __cplusplus
extern "C" 
{
#endif

/* Shrink to subvectors/submatrices.
 * Should be faster as reallocation?!
 */
void gsl_multifit_linear_realloc (gsl_multifit_linear_workspace *w, size_t n, size_t p);
void gsl_multifit_robust_realloc (gsl_multifit_robust_workspace *w, size_t n, size_t p);

/*assume stride=1*/
inline double gsl_vector_get_fast( gsl_vector *v, const size_t pos){
	return gsl_vector_get(v,pos);
	//return v->data+pos;
}
inline void gsl_vector_set_fast (gsl_vector * v, const size_t pos, double x){
	gsl_vector_set(v,pos,x);
	//*(v->data+pos) = x;
}


#ifdef __cplusplus
}
#endif

#endif
