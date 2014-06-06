#ifndef GESTURES_H
#define GESTURES_H

/* Problem: Given is a set of points, which describe a movement/gesture.
 * 					How can we dectect which gesture it is?
 *
 * Ansatz of this approach: 
 * 1) Create cubic BSplines to approximate the moving.
 *    It's important to use a low number of spline cofficients.
 *		The splines mapping T=[0,1] on the curve.
 *
 * 2) The spline will be used to evaluate a 1d function which is invariant 
 * 	under rotation and translation and scaling of the gesture.
 * 	Moreover, this function try to conservate 'global' properties of the curve
 * 	and no 'local' properties. We use secants 
 * 	s(t) :=  distance( P(t+0.5) - P(t) )/length(spline)
 *
 * 	TODO: A RANSAC algorithm should produce better results. It could be
 * 	useful to look into the OpenCV Ransac framework.
 * 	A non-uniform	placment of the bspline knots or a similar technique
 * 	could reduce the overshooting at the curve ends, too.
 * 
 * 3) To compare the motion with an other we check the L1- (or L2-)norm
 * (via quadrature of very low order) of
 * 		s2(t) - s1(t)
 * 	Some metadata will be used to respect the rotation, oriention, and/or scaling of
 * 	the gesture. (Nearly) Closed motion curves set a flag.
 *
 * 4) To Load/Save gestures the GestureStore class should be useful.
 *
 */

#include <cmath>
#include <vector>
#include <deque>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>

#include "gsl_helper.h"

#include "Blob.h"
#include "Tracker.h"

#define DIM 2

/* 2 dimensional float value */
struct fpoint2 {
	float val[2];
};

/* The frame history of a blob 
 * can contains MAX_HISTORY_LEN values with framenumbers
 * F_1 > F_2 > … > F_i
 * and F_1 - F_i <= MAX_DURATION_STEPS.
 *
 * A difference in the missing_duration property of F_j and F_{j+1} marks skipped frames.
 * MAX_DURATION_STEPS determine the number of preevaluated Basis elements
 */
#define MAX_DURATION_STEPS 100

/*
 * Describe the number of uniform distributed 
 * evaluation points (in [0,1]) of the spline.
 *
 */
#define NUM_EVALUATION_POINTS 50

#define K					4
#define N        100

/* Preevaluate the Spline values on N positions for splines with
 * nc coefficients with NCOEFFS_MIN <= nc < NCOEFFS_MAX
 * On runtime, the selected value for nc depends on the NCOEFFS_FUNC.
 * It's important for the smoothing to avoid high nc values
 * for a small amount of datapoints (n).
 */
#define NCOEFFS_MIN 4
#define NCOEFFS_MAX 9
#define NCOEFFS_FUNC(n) (std::max(NCOEFFS_MIN,std::min((int)(n/3),NCOEFFS_MAX-1)))
//#define NBREAK   (NCOEFFS + 2 - K) //Should be at least 2

/* Uniform grid on [0,1] with MAX_DURATION_STEPS + 2 - K.
 * The border values will be doubled (0,0,…,1,1).
 */
//static gsl_vector *FullGrid[NCOEFFS_MAX-NCOEFFS_MIN];

/* Basisfunction on all knots of the FullGrid
 */
static gsl_matrix *FullBasis[NCOEFFS_MAX-NCOEFFS_MIN]; 
static gsl_vector *Global_c[NCOEFFS_MAX-NCOEFFS_MIN][DIM];
static gsl_matrix *Global_cov[NCOEFFS_MAX-NCOEFFS_MIN][DIM];

static gsl_multifit_linear_workspace *mw = NULL;
//static gsl_multifit_robust_workspace *rw = NULL;//more stable agains outliners
static gsl_bspline_workspace *bw[NCOEFFS_MAX-NCOEFFS_MIN];

static void initStaticGestureVariables(){
	size_t i,j, ncoeffs;

	mw = gsl_multifit_linear_alloc(N, NCOEFFS_MAX-1);
	//rw = gsl_multifit_robust_alloc(gsl_multifit_robust_default, N, NCOEFFS);

	/* Evaluate the basisvalues for N points in [0,1] and different
	 * values of ncoeffs. */
	for ( i=0, ncoeffs = NCOEFFS_MIN; ncoeffs < NCOEFFS_MAX; ++ncoeffs, ++i ){
		size_t nbreak = ncoeffs + 2 - K;
		bw[i] = gsl_bspline_alloc(K, nbreak);
		gsl_bspline_knots_uniform(0.0, 1.0, bw[i]);

		FullBasis[i] = gsl_matrix_alloc(N, ncoeffs);
		gsl_vector *tmpB = gsl_vector_alloc(ncoeffs);
		double step = 1.0/(N-1);
		double t = 0;
		for (j = 0; j < N; ++j, t+=step )
		{
			gsl_bspline_eval(t, tmpB, bw[i]);
			gsl_matrix_set_row( FullBasis[i], j, tmpB);
		}
		gsl_vector_free(tmpB);
	}

	//Extract grid. (The bspline knots array is slightly bigger at both ends...)
	//FullGrid = gsl_vector_alloc(NBREAK);//NBREAK + 2*(K-1)
	//gsl_vector_const_view _grid = gsl_vector_const_subvector (bw->knots, K-1, NBREAK);
	//gsl_blas_dcopy(&_grid.vector,FullGrid);

	for ( i = NCOEFFS_MAX - NCOEFFS_MAX; i > 0 ;   ){
		--i;
		for ( j=0; j<DIM; ++j ){
			Global_c[i][j] = gsl_vector_alloc(m_ncoeffs);
			Global_cov[i][j] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
		}
	}

}

static void uninitStaticGestureVariables(){
	//gsl_vector_free(FullGrid);

	size_t ncoeffs;
	for ( ncoeffs = NCOEFFS_MIN; ncoeffs < NCOEFFS_MAX; ++ncoeffs){
		gsl_bspline_free(bw[ncoeffs]);
		bw[ncoeffs] = NULL;
		gsl_matrix_free(FullBasis[ncoeffs]);
		FullBasis[ncoeffs] = NULL;
	}

	/* reset internal mw vector/matrix values to original values
	 * (just to be on the save side) */
	gsl_multifit_linear_realloc(mw, N, NCOEFFS_MAX-1);
	gsl_multifit_linear_free(mw); mw = NULL;
	//gsl_multifit_robust_free(rw); rw = NULL;
	
	for ( i = NCOEFFS_MAX - NCOEFFS_MAX; i > 0 ;   ){
		--i;
		for ( j=0; j<DIM; ++j ){
			gsl_vector_free(Global_c[i][j]);
			gsl_matrix_free(Global_cov[i][j]);
		}
	}
}


class Gesture{
	private:
		gsl_vector *m_raw_values[DIM]; //input data
		//matching dimensions for raw_values
		size_t m_n;
		size_t m_ncoeffs;
		size_t m_nbreak;
		// Subset of Grid and FullBasis.
		//gsl_vector *m_grid;
		gsl_matrix *m_ReducedBasis;

		//Spline data, now globaly to omit some shorttime allocations
		//gsl_vector *m_c[DIM];
		//gsl_matrix *m_cov[DIM];

		//gsl_bspline_workspace *m_bw;
	public:
		Gesture( cBlob &blob);
		~Gesture();
		void evalSpline();

		//for debugging...
		void plotSpline(double *outY, double *outY, int *outLen );

};


class GestureStore{


};


#endif
