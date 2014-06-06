#include "gsl_helper.h"

void gsl_multifit_linear_realloc (gsl_multifit_linear_workspace *w, size_t n, size_t p){
	if( w->n == n && w->p == p) return;

	w->n = n;
	w->p = p;
	w->A->size1 = n;
	w->A->size2 = p;
	w->Q->size1 = p;
	w->Q->size2 = p;
	w->QSI->size1 = p;
	w->QSI->size2 = p;
	w->S->size = p;
	w->t->size = n;
	w->xt->size = p;
	w->D->size = p;
};
