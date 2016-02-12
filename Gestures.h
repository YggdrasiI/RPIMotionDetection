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
 * 	and no 'local' properties (like derivatives). We use secants 
 * 	s(t) :=  distance( P(t+0.5) - P(t) )/length(spline)
 *
 * 	TODO: A RANSAC algorithm should produce better results. It could be
 * 	useful to look into the OpenCV Ransac framework.
 * 	A non-uniform	placement of the bspline knots or a similar technique
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
#include <map>
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
 * Note that 'F_j - F_{j-1} > 1' if a blob was not found in intermediate frames.
 * The difference between both values return the number of skipped frames.
 * MAX_DURATION_STEPS determine the number of preevaluated Basis elements
 */
#define MAX_DURATION_STEPS 100

/*
 * Describe the number of uniform distributed 
 * evaluation points (in [0,1]) of the spline.
 *
 */
#define NUM_EVALUATION_POINTS 50

/*
 * Higher values (>3) reduce the detection of edges.
 * 2 degrees of freedom = piecewise linear 
 */
#define SPLINE_DEG					3

/* Preevaluate the Spline values on MAX_DURATION_STEPS positions for splines with
 * nc coefficients with NCOEFFS_MIN <= nc < NCOEFFS_MAX
 * On runtime, the selected value for nc depends on the NCOEFFS_FUNC.
 * It's important for the smoothing (and again overshooting) to avoid high nc values
 * for a small amount of datapoints (n). 
 */
#define NCOEFFS_MIN 4
#define NCOEFFS_MAX 9
#define NCOEFFS_FUNC(n) (std::max(NCOEFFS_MIN,std::min((int)(n/4),NCOEFFS_MAX-1)))
//#define NBREAK   (NCOEFFS + 2 - SPLINE_DEG) //Should be at least 2

/*
 * Uncomment ROBUST_FIT to gsl_multifit_robust_est() for approximation.
 * Otherwise, gsl_multifit_linear_est() will be used.
 * Robust version does not work. The algorithm is probably misunderstand by the author...
 */
//#define ROBUST_FIT

/* Uniform grid on [0,1] with NUM_EVALUATION_POINTS. (Currently) all further 
 * evaluatationn base on values of this positions.
 */
static gsl_vector *EvalGrid;
/* Store basis function values for EvalGrid */
static gsl_matrix *EvalGridBasis[NCOEFFS_MAX-NCOEFFS_MIN]; 

/* Flag to indicate if the spline estimation should be saved in 
 * the gesture objects or a global variable */
#define STORE_IN_MEMBER_VARIABLE

/* Number of nodes which are used for the mean value of start and
 * end of a gesture. */
#define NUM_END_NODES 3

/* Number of ignored nodes at the beginning and end of input values. If the
 * gesture ends, mostly the last hand movements disturbs the
 * curve.
 */
#define DEFAULT_SKIPPED_BEGIN_NODES 0
#define DEFAULT_SKIPPED_END_NODES 0

/* Distance between two spline evaluation points which will be
 * used for the generation of the gesture invariance function.
 */
static const size_t CompareDistancesNum = 4;
static const size_t CompareDistances[] = {
	(NUM_EVALUATION_POINTS*3/4),
	(NUM_EVALUATION_POINTS/2),
	(NUM_EVALUATION_POINTS/4),
	(NUM_EVALUATION_POINTS/3),
};

/* Basisfunction on all knots of the FullGrid
 */
static gsl_matrix *FullBasis[NCOEFFS_MAX-NCOEFFS_MIN]; 
static gsl_vector *Global_c[NCOEFFS_MAX-NCOEFFS_MIN][DIM];
static gsl_matrix *Global_cov[NCOEFFS_MAX-NCOEFFS_MIN][DIM];

static gsl_bspline_workspace *bw[NCOEFFS_MAX-NCOEFFS_MIN];
#ifdef ROBUST_FIT
static gsl_multifit_robust_workspace *rw = NULL;//more stable agains outliners
#else
static gsl_multifit_linear_workspace *mw = NULL;
#endif

static size_t gestureIdCounter = 0;

static void initStaticGestureVariables(){
	size_t i,j, ncoeffs;

#ifdef ROBUST_FIT
	rw = gsl_multifit_robust_alloc(gsl_multifit_robust_default, MAX_DURATION_STEPS, NCOEFFS_MAX-1);
#else
	mw = gsl_multifit_linear_alloc(MAX_DURATION_STEPS, NCOEFFS_MAX-1);
#endif

	/* Evaluate the basisvalues for MAX_DURATION_STEPS points in [0,1] and different
	 * values of ncoeffs. */
	for ( i=0, ncoeffs = NCOEFFS_MIN; ncoeffs < NCOEFFS_MAX; ++ncoeffs, ++i ){
		size_t nbreak = ncoeffs + 2 - SPLINE_DEG;
		bw[i] = gsl_bspline_alloc(SPLINE_DEG, nbreak);
		gsl_bspline_knots_uniform(0.0, 1.0, bw[i]);

		FullBasis[i] = gsl_matrix_alloc(MAX_DURATION_STEPS, ncoeffs);
		//gsl_vector *tmpB = gsl_vector_alloc(ncoeffs);
		double step = 1.0/(MAX_DURATION_STEPS-1);
		double t = 0;
		for (j = 0; j < MAX_DURATION_STEPS; ++j, t+=step )
		{
			//gsl_bspline_eval(t, tmpB, bw[i]);
			//gsl_matrix_set_row( FullBasis[i], j, tmpB);
			gsl_vector_view subrow = gsl_matrix_subrow( FullBasis[i], j, 0, ncoeffs);
			gsl_bspline_eval(t, &subrow.vector, bw[i]);
		}
		//gsl_vector_free(tmpB);

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

	// Eval values of basis function for evaluation points
	EvalGrid = gsl_vector_alloc(NUM_EVALUATION_POINTS);
	for ( i=0, ncoeffs = NCOEFFS_MIN; ncoeffs < NCOEFFS_MAX; ++ncoeffs, ++i ){
		EvalGridBasis[i] = gsl_matrix_alloc(NUM_EVALUATION_POINTS, ncoeffs);
		double step = 1.0/(NUM_EVALUATION_POINTS-1);
		double t = 0.0;
		for (j = 0; j < NUM_EVALUATION_POINTS; ++j, t+=step )
		{
			if( t>1.0 ) t -= 1E-10; //Omit leaving of interval due rounding errors.
			gsl_vector_set(EvalGrid, j, t);
			gsl_vector_view subrow = gsl_matrix_subrow( EvalGridBasis[i], j, 0, ncoeffs);
			gsl_bspline_eval(t, &subrow.vector, bw[i]);
		}
	}
}

static void uninitStaticGestureVariables(){
	//gsl_vector_free(FullGrid);

	size_t i,j;
	for(i = 0;  i < NCOEFFS_MAX - NCOEFFS_MAX; ++i){
		gsl_bspline_free(bw[i]);
		bw[i] = NULL;
		gsl_matrix_free(FullBasis[i]);
		FullBasis[i] = NULL;
	}

#ifdef ROBUST_FIT
	if( rw != NULL ){
		gsl_multifit_robust_free(rw); rw = NULL;
		gsl_multifit_robust_realloc(rw, MAX_DURATION_STEPS, NCOEFFS_MAX-1);
	}
#else
	if( mw != NULL ){
		/* reset internal mw vector/matrix values to original values
		 * (just to be on the save side on following freeing) */
		gsl_multifit_linear_realloc(mw, MAX_DURATION_STEPS, NCOEFFS_MAX-1);
		gsl_multifit_linear_free(mw); mw = NULL;
	}
#endif

	for(i = 0;  i < NCOEFFS_MAX - NCOEFFS_MAX; ++i){
		for ( j=0; j<DIM; ++j ){
			gsl_vector_free(Global_c[i][j]);
			gsl_matrix_free(Global_cov[i][j]);
		}
	}

	//Evaluation grid points/basis.
	for(i = 0;  i < NCOEFFS_MAX - NCOEFFS_MAX; ++i){
		gsl_matrix_free(EvalGridBasis[i]);
	}
	gsl_vector_free(EvalGrid);
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
		//gsl_vector *m_curvePointDistances;
		gsl_vector **m_curvePointDistances;


		Gesture( cBlob &blob,
				size_t skipped_begin_nodes = DEFAULT_SKIPPED_BEGIN_NODES,
				size_t skipped_end_nodes = DEFAULT_SKIPPED_END_NODES );
		/* reversedTime flip's the order of the input values.
		 * Note that the history/values of the tracker 
		 * are ordered als
		 * [actual value, previous value, ...., oldes value ]
		 */
		Gesture(int *inTimestamp, double *inX, double *inY, size_t xy_Len, size_t stride = 1, bool reversedTime = true );
		Gesture(int *inTimestamp, double *inXY, size_t xy_Len, bool reversedTime = true );
		~Gesture();

		size_t getNumberOfRawSupportNodes() const;

		void evalSplineCoefficients();
		void evalOrientation();
		void evalDistances();
		double evalCurveLength();
		bool isClosedCurve(/* float *outAbs, float *outRel*/);

		//for debugging...
		void evalSpline(double **outX, double **outY, size_t *outLen );

		void setGestureName(const char* name);
		const char* getGestureName() const;
		size_t getGestureId() const;

		void printDistances() const{

			double *out = m_curvePointDistances[0]->data;
			for( int i=0; i<NUM_EVALUATION_POINTS-CompareDistances[0]; ++i){
				printf("%d %f\n", i, *out++);
			}
		}
};

class GestureDistance{
	public:
		const Gesture *m_from, *m_to;
		int m_sorting_weight; // weight by sorting position in each level.
		double m_L2_weight; // sum up error of each level.
		GestureDistance(const Gesture *from, const Gesture *to):m_from(from),m_to(to),
		m_sorting_weight(0), m_L2_weight(0.0){
		};
		double m_L2NormSquared[4] = {DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX };
		double evalL2Dist( size_t level);
};



/* Store the result for gesture analyse. */
struct GesturePatternCompareResult{
	float minDist; 
	const Gesture *minGest; //Gesture of gestureStore with minimal 'distance'.
	float avgDist; // Average distance to (currently all) gestures of the gesture store.
	//std::map<Gesture*, float[> d
};

class GestureStore{
	private:
		/* Use pointers because Gesture objects contains many memory allocations (gsv_vector_*, etc.) */
			std::vector<Gesture*> gestures;

	public:
			GestureStore();
			~GestureStore();
			/* All patters will be deleted in the destructor. */
			void addPattern(Gesture *pGesture);
			std::vector<Gesture*> & getPatterns();

			void compateWithPatterns(Gesture *pGesture, GesturePatternCompareResult &gpcr );

};


/* Create test gesture by function and store it. */
void addGestureTestPattern(GestureStore &gestureStore);

#ifdef WITH_OPENGL
class GfxTexture; 

/* Render spline resprentation of gesture.
 *
 * color_rgba: color of first node. Use NULL for default value
 * increment_rgba: offset for each further node. Value will be flipped if color leave [0,1].
 * 			Use NULL for default value.
 */
void gesture_drawSpline( Gesture *gesture, int screenWidth, int screenHeight, float *color_rgba = NULL, float *increment_rgba = NULL, GfxTexture *target = NULL);
#endif

#endif
