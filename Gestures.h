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
#include <cstring>
//#include <string>

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
	//float val[2];
	float x;
	float y;
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

#define SPLINE_DEG					4
#define MAX_DURATION_STEPS        100

/* Preevaluate the Spline values on MAX_DURATION_STEPS positions for splines with
 * nc coefficients with NCOEFFS_MIN <= nc < NCOEFFS_MAX
 * On runtime, the selected value for nc depends on the NCOEFFS_FUNC.
 * It's important for the smoothing to avoid high nc values
 * for a small amount of datapoints (n).
 */
#define NCOEFFS_MIN 4
#define NCOEFFS_MAX 9
#define NCOEFFS_FUNC(n) (std::max(NCOEFFS_MIN,std::min((int)(n/3),NCOEFFS_MAX-1)))
//#define NBREAK   (NCOEFFS + 2 - SPLINE_DEG) //Should be at least 2

/* Uniform grid on [0,1] with MAX_DURATION_STEPS + 2 - SPLINE_DEG.
 * The border values will be doubled (0,0,…,1,1).
 */
//static gsl_vector *FullGrid[NCOEFFS_MAX-NCOEFFS_MIN];

/* Flag to indicate if the spline estimation should be saved in 
 * the gesture objects or a global variable */
#define STORE_IN_MEMBER_VARIABLE

/* Number of nodes which are used for the mean value of start and
 * end of a gesture. */
#define NUM_END_NODES 3

/* Distance between two spline evaluation points which will be
 * used for the generation of the gesture invariance function.
 */
#define COMPARE_DIST (NUM_EVALUATION_POINTS/2)

/* Basisfunction on all knots of the FullGrid
 */
static gsl_matrix *FullBasis[NCOEFFS_MAX-NCOEFFS_MIN]; 
static gsl_vector *Global_c[NCOEFFS_MAX-NCOEFFS_MIN][DIM];
static gsl_matrix *Global_cov[NCOEFFS_MAX-NCOEFFS_MIN][DIM];

static gsl_multifit_linear_workspace *mw = NULL;
//static gsl_multifit_robust_workspace *rw = NULL;//more stable agains outliners
static gsl_bspline_workspace *bw[NCOEFFS_MAX-NCOEFFS_MIN];

static size_t gestureIdCounter = 0;

static void initStaticGestureVariables(){
	size_t i,j, ncoeffs;

	mw = gsl_multifit_linear_alloc(MAX_DURATION_STEPS, NCOEFFS_MAX-1);
	//rw = gsl_multifit_robust_alloc(gsl_multifit_robust_default, MAX_DURATION_STEPS, NCOEFFS);

	/* Evaluate the basisvalues for MAX_DURATION_STEPS points in [0,1] and different
	 * values of ncoeffs. */
	for ( i=0, ncoeffs = NCOEFFS_MIN; ncoeffs < NCOEFFS_MAX; ++ncoeffs, ++i ){
		size_t nbreak = ncoeffs + 2 - SPLINE_DEG;
		bw[i] = gsl_bspline_alloc(SPLINE_DEG, nbreak);
		gsl_bspline_knots_uniform(0.0, 1.0, bw[i]);

		FullBasis[i] = gsl_matrix_alloc(MAX_DURATION_STEPS, ncoeffs);
		gsl_vector *tmpB = gsl_vector_alloc(ncoeffs);
		double step = 1.0/(MAX_DURATION_STEPS-1);
		double t = 0;
		for (j = 0; j < MAX_DURATION_STEPS; ++j, t+=step )
		{
			gsl_bspline_eval(t, tmpB, bw[i]);
			gsl_matrix_set_row( FullBasis[i], j, tmpB);
		}
		gsl_vector_free(tmpB);
	}

	//Extract grid. (The bspline knots array is slightly bigger at both ends...)
	//FullGrid = gsl_vector_alloc(NBREAK);//NBREAK + 2*(SPLINE_DEG-1)
	//gsl_vector_const_view _grid = gsl_vector_const_subvector (bw->knots, SPLINE_DEG-1, NBREAK);
	//gsl_blas_dcopy(&_grid.vector,FullGrid);

	for ( i=0, ncoeffs = NCOEFFS_MIN; ncoeffs < NCOEFFS_MAX; ++ncoeffs, ++i ){
		for ( j=0; j<DIM; ++j ){
			Global_c[i][j] = gsl_vector_alloc(ncoeffs);
			Global_cov[i][j] = gsl_matrix_alloc(ncoeffs, ncoeffs);
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
	gsl_multifit_linear_realloc(mw, MAX_DURATION_STEPS, NCOEFFS_MAX-1);
	gsl_multifit_linear_free(mw); mw = NULL;
	//gsl_multifit_robust_free(rw); rw = NULL;

	size_t i,j;
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
		double m_time_max;
		// Subset of Grid and FullBasis.
		//gsl_vector *m_grid;
		gsl_matrix *m_ReducedBasis;
		gsl_vector *m_debugDurationGrid;

		char *m_gestureName;
		size_t m_gestureId;

#ifdef STORE_IN_MEMBER_VARIABLE
		gsl_vector *m_c[DIM];
		gsl_matrix *m_cov[DIM];
#endif


		/* Stores the result of evalSpline */
		gsl_vector *m_splineCurve[DIM];

		//gsl_bspline_workspace *m_bw;
	
		/* Derive spline coefficients and all metadata.
		 * Called in all Constructors. */
		void construct();

	public:
		fpoint2 m_orientationTriangle[3];

		/* Stores result of evalDistances() */
		gsl_vector *m_curvePointDistances;


		Gesture( cBlob &blob);
		Gesture(int *inTimestamp, double *inX, double *inY, size_t xy_Len, size_t stride );
		Gesture(int *inTimestamp, double *inXY, size_t xy_Len );
		~Gesture();

		size_t getNumberOfRawSupportNodes() const;

		void evalSplineCoefficients();
		void evalOrientation();
		void evalDistances();
		bool isClosedCurve(/* float *outAbs, float *outRel*/);

		//for debugging...
		void evalSpline(double **outX, double **outY, size_t *outLen );

		void setGestureName(const char* name);
		const char* getGestureName() const;

};

/* Store the result for gesture analyse. */
struct GesturePatternCompareResult{
	float minDist; 
	Gesture *minGest; //Gesture of gestureStore with minimal 'distance'.
	float avgDist; // Average distance to (currently all) gestures of the gesture store.
};

class GestureStore{
	private:
		/* Use pointers because Gesture objects contains many memory allocations (gsv_vector_*, etc.) */
			std::vector<Gesture*> gestures;

	public:
			GestureStore();
			~GestureStore();
			/* All patters will be deleted in the destrutor. */
			void addPattern(Gesture *pGesture);
			std::vector<Gesture*> & getPatterns();

			void compateWithPatterns(Gesture *pGesture, GesturePatternCompareResult &gpcr );

};


/* Create test gesture by function and store it. */
static void addGestureTestPattern(GestureStore &gestureStore){
	size_t n = 30;
	double xy[2*n];
	int time[n];
	double *pos=&xy[0];
	size_t i=0; 

	//L-shape
	for( ; i<20; ++i){
		*pos++ = 100; 
		*pos++ = 100 + i*5;
		time[i] = i;
	}
	for( ; i<30; ++i){
		*pos++ = 100 + (i-20)*5; 
		*pos++ = 100 + 20*5;
		time[i] = i;
	}
	Gesture *gest1 = new Gesture( time, xy, n); 
	gest1->setGestureName("L-shape");
	gestureStore.addPattern(gest1);


	//Circle
	pos=&xy[0];
	for(i=0 ; i<30; ++i){
		*pos++ = 300 + 20*cos(2*3.1415*i/30);
		*pos++ = 150 + 20*sin(2*3.1415*i/30);
	}
	Gesture *gest2 = new Gesture( time, xy, n); 
	gest2->setGestureName("Circle");
	gestureStore.addPattern(gest2);

	//Line
	pos=&xy[0];
	for(i=0 ; i<30; ++i){
		*pos++ = 200 - 6*i - (i*333)%7;
		*pos++ = 100 + 6*i + (i*111)%7;
	}
	Gesture *gest3 = new Gesture( time, xy, n); 
	gest3->setGestureName("Line");
	gestureStore.addPattern(gest3);

	//U-Shape
	pos=&xy[0];
	for(i=0 ; i<10; ++i){
		*pos++ = 300; 
		*pos++ = 100 + i*10;
	}
	for( ; i<20; ++i){
		*pos++ = 300 + (i-10)*5; 
		*pos++ = 200;
	}
	for( ; i<30; ++i){
		*pos++ = 350; 
		*pos++ = 200 - (i-20)*10;
	}
	Gesture *gest4 = new Gesture( time, xy, n); 
	gest4->setGestureName("U-Shape");
	gestureStore.addPattern(gest4);

}

#endif
