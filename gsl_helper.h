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

#ifdef __cplusplus
}
#endif

#endif
