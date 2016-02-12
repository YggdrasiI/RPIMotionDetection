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

void gsl_multifit_robust_realloc (gsl_multifit_robust_workspace *w, size_t n, size_t p){
	if( w->n == n && w->p == p) return;

	gsl_multifit_linear_realloc(w->multifit_p, n, p);
	w->n = n;
	w->p = p;
	w->r->size = n;
	w->weights->size = n;
	w->c_prev->size = p;
	w->resfac->size = n;
	w->psi->size = n;
	w->dpsi->size = n;
	w->QSI->size1 = p;
	w->QSI->size2 = p;
	w->D->size = p;
	w->workn->size = n;
	w->stats.dof = n - p;
};
