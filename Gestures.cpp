#include <cfloat>
#include <assert.h>
#include <algorithm>
#include "Gestures.h"


//Sorting functions to find best fitting gestures.
static bool sortByLevel0(const GestureDistance *lhs, const GestureDistance *rhs) { 
	return lhs->m_L2NormSquared[0] < rhs->m_L2NormSquared[0];
}
static bool sortByLevel1(const GestureDistance *lhs, const GestureDistance *rhs) { 
	return lhs->m_L2NormSquared[1] < rhs->m_L2NormSquared[1];
}
static bool sortByLevel2(const GestureDistance *lhs, const GestureDistance *rhs) { 
	return lhs->m_L2NormSquared[2] < rhs->m_L2NormSquared[2];
}
static bool sortByLevel3(const GestureDistance *lhs, const GestureDistance *rhs) { 
	return lhs->m_L2NormSquared[3] < rhs->m_L2NormSquared[3];
}
static bool sortByWeight(const GestureDistance *lhs, const GestureDistance *rhs) { 
	if( lhs->m_sorting_weight == rhs->m_sorting_weight){
		return lhs->m_L2NormSquared[1] < rhs->m_L2NormSquared[1];
	}
	return lhs->m_sorting_weight < rhs->m_sorting_weight;
}

typedef bool (*sortDef)(const GestureDistance*, const GestureDistance*);
static const sortDef sortFunctions[4] = { sortByLevel0, sortByLevel1, sortByLevel2, sortByLevel3};

/* Approx |f(t)-g(t)|_2^2 over I=[0,1] with N equidistant supporting points.
 *
 * Requires N>1. 
 * \sum_i( (d_{i+1}+d_i)/2 ) = (d_0+d_{N-1})/2 + sum_{i=1}^{N-2} d_i
 * */
static double quadratureSquared( double *inA, double *inB, const size_t abLen ){

	if( abLen<2 ) return 0.0;
	
	double d,sum;
	size_t N = abLen;

	d = *inA - *inB;
	sum = d*d/2;
	++inA;
	++inB;
	--N;

	while( N ){
		d = *inA - *inB;
		sum += d*d;
		++inA;
		++inB;
		--N;
	}
	sum -= d*d/2;

	return sum/(abLen-1);
}



Gesture::Gesture( cBlob &blob):
m_n(0),m_ncoeffs(0),m_nbreak(0),
	m_time_max(1.0),
		//m_grid(NULL),
		m_curvePointDistances(NULL),
		m_debugDurationGrid(NULL),
		m_gestureName(NULL),
		m_gestureId(gestureIdCounter++),
		//m_bw(NULL),
		m_ReducedBasis(NULL)
{
	//init static variables
	if( mw == NULL ){
		initStaticGestureVariables();
	}

	m_raw_values[0]= NULL;
	m_raw_values[1]= NULL;
	m_splineCurve[0] = NULL;
	m_splineCurve[1] = NULL;

#ifdef STORE_IN_MEMBER_VARIABLE
	m_c[0]= NULL;
	m_c[1]= NULL;
	m_cov[0]= NULL;
	m_cov[1]= NULL;
#endif

#ifdef WITH_HISTORY

	if( blob.history.get() != nullptr ){

		/* 0. Setup sizes and init vectors */
		//size_t m_n = std::min( 1 + blob.history.get()->size(), MAX_DURATION_STEPS);
		m_n = 1 + blob.history.get()->size();
		if( m_n > MAX_DURATION_STEPS ) m_n = MAX_DURATION_STEPS;

		m_ncoeffs = NCOEFFS_FUNC(m_n);
		m_nbreak = m_ncoeffs + 2 - SPLINE_DEG;
		
		/* Note: m_nbreak is size_t and could be underflow.
		 * Moreover, m_nbreak must be at least 2 for a successful
		 * bspline initialisation.
		 * This check was replaced because m_ncoeffs has the
		 * correct lower bound, now. It's fine to check m_n directly.
			*/
		//if( m_nbreak-2 < 1000 )
		if( m_n > 10 )	
		{
			printf("n,nc,nb=%lu,%lu,%lu \n", m_n, m_ncoeffs, m_nbreak);

			//m_grid = gsl_vector_alloc( m_nbreak/*+2*(SPLINE_DEG-1)*/ );
			m_ReducedBasis = gsl_matrix_alloc(m_n, m_ncoeffs);

			m_raw_values[0] = gsl_vector_alloc(m_n);
			m_raw_values[1] = gsl_vector_alloc(m_n);
			m_debugDurationGrid = gsl_vector_alloc(m_n);

#ifdef STORE_IN_MEMBER_VARIABLE
			m_c[0] = gsl_vector_alloc(m_ncoeffs);
			m_c[1] = gsl_vector_alloc(m_ncoeffs);
			m_cov[0] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
			m_cov[1] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
#endif

			//m_bw = gsl_bspline_alloc(SPLINE_DEG, m_nbreak);
			const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;

			/* 1. Filling vectors */
			//gsl_vector_set(m_grid, 0, 0.0);
			const gsl_vector_const_view subrow = gsl_matrix_const_subrow ( FullBasis[basisIndex], 0, 0, m_ncoeffs);
			gsl_matrix_set_row( m_ReducedBasis, 0, &subrow.vector );

			gsl_vector_set_fast(m_raw_values[0], 0, blob.location.x);
			gsl_vector_set_fast(m_raw_values[1], 0, blob.location.y);
			gsl_vector_set_fast(m_debugDurationGrid, 0, 0.0);

			int knot=0;
			int duration=blob.duration;
			int globKnot;
			std::deque<cBlob>::iterator it = blob.history.get()->begin();
			const std::deque<cBlob>::iterator itEnd = blob.history.get()->end();

			for ( ; it != itEnd ; ++it){
				//go to next global knot.
				globKnot = duration - (*it).duration + 1; /* TODO Ok, die +1 sollte weg und duration bei den Blobs korrigiert werden. */
				++knot;

				if( globKnot >= MAX_DURATION_STEPS ){
					fprintf(stderr,"%s:Excess max_jmal duration for gestures.\n",__FILE__);
					/* The tracked data is to far in the past. Cut of at this position and short
					 * the dimension of the basis matrix.
					 * Note: The selection of m_ncoeffs could be to high for the new m_n.
					 */
					m_n = knot;
					m_ReducedBasis->size1 = m_n; //Now, the internal array size not matching anymore.
					m_raw_values[0]->size = m_n;
					m_raw_values[1]->size = m_n;
					break;
				}

				gsl_vector_set_fast(m_raw_values[0], knot, (*it).location.x);
				gsl_vector_set_fast(m_raw_values[1], knot, (*it).location.y);
				gsl_vector_set_fast(m_debugDurationGrid, knot, ((double)globKnot)/MAX_DURATION_STEPS);
				//gsl_vector_set_fast( m_grid, knot, gsl_vector_get( FullGrid, globKnot ) );

				const gsl_vector_const_view subrow2 = gsl_matrix_const_subrow( FullBasis[basisIndex], globKnot, 0, m_ncoeffs);
				gsl_matrix_set_row( m_ReducedBasis, knot, &subrow2.vector );
			}

			//printf("Matrix:\n");
			//gsl_matrix_fprintf(stdout,m_ReducedBasis, "%f");

			//gsl_bspline_knots(m_grid, m_bw);//internally, m_grid data will be copied, which is omitable

			m_time_max = ((double)globKnot)/MAX_DURATION_STEPS;
		}else{
			// To less points
			m_n = 0;
		}
	}


#else
	fprinf(stderr, "Gesture construtor require history of blob position.\n");
#endif
	
	construct();
}

Gesture::Gesture(int *inTimestamp, double *inX, double *inY, size_t xy_Len, size_t stride=1 ):
m_n(xy_Len),m_ncoeffs(0),m_nbreak(0),
	m_time_max(1.0),
		m_curvePointDistances(NULL),
		m_gestureName(NULL),
		m_gestureId(gestureIdCounter++),
		m_debugDurationGrid(NULL),
		m_ReducedBasis(NULL)
{
	//init static variables
	if( mw == NULL ){
		initStaticGestureVariables();
	}

	m_raw_values[0]= NULL;
	m_raw_values[1]= NULL;
	m_splineCurve[0] = NULL;
	m_splineCurve[1] = NULL;

#ifdef STORE_IN_MEMBER_VARIABLE
	m_c[0]= NULL;
	m_c[1]= NULL;
	m_cov[0]= NULL;
	m_cov[1]= NULL;
#endif

	if( m_n > MAX_DURATION_STEPS ) m_n = MAX_DURATION_STEPS;
	m_ncoeffs = NCOEFFS_FUNC(m_n);
	m_nbreak = m_ncoeffs + 2 - SPLINE_DEG;

	/* Note: m_nbreak is size_t and could be underflow.
	 * Moreover, m_nbreak must be at least 2 for a successful
	 * bspline initialisation.
	 * This check was replaced because m_ncoeffs has the
	 * correct lower bound, now. It's requred to check m_n directly.
	 */
	//if( m_nbreak-2 < 1000 )
	if( m_n > 10 )	
	{
		//m_grid = gsl_vector_alloc( m_nbreak/*+2*(SPLINE_DEG-1)*/ );
		m_ReducedBasis = gsl_matrix_alloc(m_n, m_ncoeffs);

		m_raw_values[0] = gsl_vector_alloc(m_n);
		m_raw_values[1] = gsl_vector_alloc(m_n);
		m_debugDurationGrid = gsl_vector_alloc(m_n);

#ifdef STORE_IN_MEMBER_VARIABLE
		m_c[0] = gsl_vector_alloc(m_ncoeffs);
		m_c[1] = gsl_vector_alloc(m_ncoeffs);
		m_cov[0] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
		m_cov[1] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
#endif

		//m_bw = gsl_bspline_alloc(SPLINE_DEG, m_nbreak);
		const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;

		/* 1. Filling vectors */
		//gsl_vector_set(m_grid, 0, 0.0);

		for (size_t i=0; i<xy_Len; ++i, inX+=stride, inY+=stride, ++inTimestamp ){

			if( *inTimestamp >= MAX_DURATION_STEPS ){
				fprintf(stderr,"%s:Excess max_jmal duration for gestures.\n",__FILE__);
				/* The tracked data is to far in the past. Cut of at this position and short
				 * the dimension of the basis matrix.
				 * Note: The selection of m_ncoeffs could be to high for the new m_n.
				 */
				m_n = i;
				m_ReducedBasis->size1 = m_n;
				m_raw_values[0]->size = m_n;
				m_raw_values[1]->size = m_n;
				break;
			}

			gsl_vector_set_fast(m_raw_values[0], i, *inX);
			gsl_vector_set_fast(m_raw_values[1], i, *inY);
			gsl_vector_set_fast(m_debugDurationGrid, i, *inTimestamp);

			const gsl_vector_const_view subrow2 = gsl_matrix_const_subrow( FullBasis[basisIndex], *inTimestamp, 0, m_ncoeffs);
			gsl_matrix_set_row( m_ReducedBasis, i, &subrow2.vector );
		}

		//printf("Matrix:\n");
		//gsl_matrix_fprintf(stdout,m_ReducedBasis, "%f");

		//gsl_bspline_knots(m_grid, m_bw);//internally, m_grid data will be copied, which is omitable

		m_time_max = ((double)*(inTimestamp-1))/MAX_DURATION_STEPS;
	}else{
		// To less points
		m_n = 0;
	}
	
	construct();
}

size_t Gesture::getNumberOfRawSupportNodes() const {
	return m_n;
}

Gesture::Gesture(int *inTimestamp, double *inXY, size_t xy_Len ):
m_n(xy_Len),m_ncoeffs(0),m_nbreak(0),
	m_time_max(1.0),
		m_curvePointDistances(NULL),
		m_gestureName(NULL),
		m_gestureId(gestureIdCounter++),
		m_debugDurationGrid(NULL),
		m_ReducedBasis(NULL)
//:Gesture(inTimestamp, inXY, inXY+1, xy_Len, 2)
{
	//*this = Gesture(inTimestamp, inXY, inXY+1, xy_Len, 2);//does not work?!
	
	//init static variables
	if( mw == NULL ){
		initStaticGestureVariables();
	}

	m_raw_values[0]= NULL;
	m_raw_values[1]= NULL;
	m_splineCurve[0] = NULL;
	m_splineCurve[1] = NULL;

#ifdef STORE_IN_MEMBER_VARIABLE
	m_c[0]= NULL;
	m_c[1]= NULL;
	m_cov[0]= NULL;
	m_cov[1]= NULL;
#endif

	if( m_n > MAX_DURATION_STEPS ) m_n = MAX_DURATION_STEPS;
	m_ncoeffs = NCOEFFS_FUNC(m_n);
	m_nbreak = m_ncoeffs + 2 - SPLINE_DEG;

	if( m_n > 10 )	
	{
		m_ReducedBasis = gsl_matrix_alloc(m_n, m_ncoeffs);

		m_raw_values[0] = gsl_vector_alloc(m_n);
		m_raw_values[1] = gsl_vector_alloc(m_n);
		m_debugDurationGrid = gsl_vector_alloc(m_n);

#ifdef STORE_IN_MEMBER_VARIABLE
		m_c[0] = gsl_vector_alloc(m_ncoeffs);
		m_c[1] = gsl_vector_alloc(m_ncoeffs);
		m_cov[0] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
		m_cov[1] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
#endif

		const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;

		/* 1. Filling vectors */
		for (size_t i=0; i<xy_Len; ++i, inXY+=2, ++inTimestamp ){

			if( *inTimestamp >= MAX_DURATION_STEPS ){
				fprintf(stderr,"%s:Excess max_jmal duration for gestures.\n",__FILE__);
				/* The tracked data is to far in the past. Cut of at this position and short
				 * the dimension of the basis matrix.
				 * Note: The selection of m_ncoeffs could be to high for the new m_n.
				 */
				m_n = i;
				m_ReducedBasis->size1 = m_n;
				m_raw_values[0]->size = m_n;
				m_raw_values[1]->size = m_n;
				break;
			}

			gsl_vector_set_fast(m_raw_values[0], i, inXY[0]);
			gsl_vector_set_fast(m_raw_values[1], i, inXY[1]);
			gsl_vector_set_fast(m_debugDurationGrid, i, *inTimestamp);

			const gsl_vector_const_view subrow2 = gsl_matrix_const_subrow( FullBasis[basisIndex], *inTimestamp, 0, m_ncoeffs);
			gsl_matrix_set_row( m_ReducedBasis, i, &subrow2.vector );
		}

		m_time_max = ((double)*(inTimestamp-1))/MAX_DURATION_STEPS;
	}else{
		// To less points
		m_n = 0;
	}
	
	construct();
}


Gesture::~Gesture(){

	//gsl_vector_free(m_grid);
	gsl_matrix_free(m_ReducedBasis);

	gsl_vector_free(m_debugDurationGrid);

	if( m_curvePointDistances != NULL ){
		for( size_t i=0; i<CompareDistancesNum; ++i){
			gsl_vector_free(m_curvePointDistances[i]);
		}
		free(m_curvePointDistances);
	}

	gsl_vector_free(m_splineCurve[0]);
	gsl_vector_free(m_splineCurve[1]);

	gsl_vector_free(m_raw_values[0]);
	gsl_vector_free(m_raw_values[1]);

#ifdef STORE_IN_MEMBER_VARIABLE
	gsl_vector_free(m_c[0]);
	gsl_vector_free(m_c[1]);
	gsl_matrix_free(m_cov[0]);
	gsl_matrix_free(m_cov[1]);
#endif	
	//gsl_bspline_free(m_bw);
	
	free(m_gestureName);
}

void Gesture::construct(){
	if( m_n == 0 ) return;

	evalSplineCoefficients();
	evalOrientation();

	double *outX, *outY;
	size_t outLen;
	evalSpline(&outX,&outY,&outLen);

	evalDistances();
}


void Gesture::evalSplineCoefficients(){
	if( m_n == 0 ) return;

	/* Shrink workspace on reduced dimension.
	 */
#if 1
	gsl_multifit_linear_realloc(mw, m_n, m_ncoeffs);
#else
	gsl_multifit_linear_free(mw); mw = NULL;
	mw = gsl_multifit_linear_alloc(m_n, m_ncoeffs);
#endif

	const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;
	for( size_t d=0; d<DIM; ++d){
		double chisq;
#ifdef STORE_IN_MEMBER_VARIABLE
		gsl_multifit_linear(m_ReducedBasis, m_raw_values[d], m_c[d], m_cov[d], &chisq, mw);
		//gsl_vector_fprintf(stdout, m_c[d], "%f ");
		//printf("====\n");
#else
		gsl_multifit_linear(m_ReducedBasis, m_raw_values[d], Global_c[basisIndex][d], Global_cov[basisIndex][d], &chisq, mw);
#endif
	}

}

void Gesture::evalSpline(double **outX, double **outY, size_t *outLen ){
	if( m_n == 0 ) return;

	size_t j;

	if( m_splineCurve[0] == NULL ){
		m_splineCurve[0] = gsl_vector_alloc(NUM_EVALUATION_POINTS);
		m_splineCurve[1] = gsl_vector_alloc(NUM_EVALUATION_POINTS);
	}else{
		//values are already evaluated
		*outX = m_splineCurve[0]->data;
		*outY = m_splineCurve[1]->data;
		*outLen = NUM_EVALUATION_POINTS;
		return;
	}

	*outX = m_splineCurve[0]->data;
	*outY = m_splineCurve[1]->data;
	*outLen = NUM_EVALUATION_POINTS;
	//*outX = new double[NUM_EVALUATION_POINTS]; 
	//*outY = new double[NUM_EVALUATION_POINTS]; 

	const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;
	
	// Eval basis on [0, m_time_max]
	double t_step = m_time_max/(NUM_EVALUATION_POINTS-1) - 1E-10;
	double t_pos = 0.0;
	gsl_vector *tmpB = gsl_vector_alloc(m_ncoeffs);
	for ( j=0; j<NUM_EVALUATION_POINTS; ++j, t_pos+=t_step )
	{
		double x_j,y_j, xerr, yerr;
		//Nun berechne ich die Basiswerte ja doch in jedem Objekt...
		gsl_bspline_eval(t_pos, tmpB, bw[basisIndex]);
#ifdef STORE_IN_MEMBER_VARIABLE
		gsl_multifit_linear_est(tmpB, m_c[0], m_cov[0], &x_j, &xerr);
		gsl_multifit_linear_est(tmpB, m_c[1], m_cov[1], &y_j, &yerr);
#else
		gsl_multifit_linear_est(tmpB, Global_c[basisIndex][0], Global_cov[basisIndex][0], &x_j, &xerr);
		gsl_multifit_linear_est(tmpB, Global_c[basisIndex][1], Global_cov[basisIndex][1], &y_j, &yerr);
		//gsl_multifit_robust_est(tmpB, Global_c[basisIndex][0], Global_cov[basisIndex][0], &x_j, &yerr);
		//gsl_multifit_robust_est(tmpB, Global_c[basisIndex][1], Global_cov[basisIndex][1], &x_j, &yerr);
#endif
		(*outX)[j] = x_j;
		(*outY)[j] = y_j;
		//printf("%f %f %f\n",t_pos,x_j,y_j);

	}
	gsl_vector_free(tmpB);

	/*
	// Use preevaluated basis. This does not work because basis was evaluated on
	// interval [0,1] not [0,m_time_max].
	for ( j=0; j<NUM_EVALUATION_POINTS; ++j)
	{
		double x_j,y_j, xerr, yerr;
		const double t_pos = gsl_vector_get_fast(EvalGrid, j);
		const gsl_vector_const_view t_basis = gsl_matrix_const_subrow ( EvalGridBasis[basisIndex], j, 0, m_ncoeffs);
		const gsl_vector *tmpB = &t_basis.vector;
#ifdef STORE_IN_MEMBER_VARIABLE
		gsl_multifit_linear_est(tmpB, m_c[0], m_cov[0], &x_j, &xerr);
		gsl_multifit_linear_est(tmpB, m_c[1], m_cov[1], &y_j, &yerr);
#else
		gsl_multifit_linear_est(tmpB, Global_c[basisIndex][0], Global_cov[basisIndex][0], &x_j, &xerr);
		gsl_multifit_linear_est(tmpB, Global_c[basisIndex][1], Global_cov[basisIndex][1], &y_j, &yerr);
		//gsl_multifit_robust_est(tmpB, Global_c[basisIndex][0], Global_cov[basisIndex][0], &x_j, &yerr);
		//gsl_multifit_robust_est(tmpB, Global_c[basisIndex][1], Global_cov[basisIndex][1], &x_j, &yerr);
#endif
		(*outX)[j] = x_j;
		(*outY)[j] = y_j;
		//printf("%f %f %f\n",t_pos,x_j,y_j);
	}
	*/

}

/* Interpretation of the three values in m_orientationTriangle:
 *  m_orientationTriangle[0] 'Mean' of curve start
 *  m_orientationTriangle[1] 'Mean' of middle values
 *  m_orientationTriangle[2] 'Mean' of curve end
 */
void Gesture::evalOrientation(){
	size_t pos;

	m_orientationTriangle[0].x = 0.0; m_orientationTriangle[0].y = 0.0;
	m_orientationTriangle[1].x = 0.0; m_orientationTriangle[1].y = 0.0;
	m_orientationTriangle[2].x = 0.0; m_orientationTriangle[2].y = 0.0;

	//assume m_n>2*NUM_END_NODES
	for( pos=0; pos<NUM_END_NODES; ++pos){
		m_orientationTriangle[0].x += gsl_vector_get_fast(m_raw_values[0],pos);
		m_orientationTriangle[0].y += gsl_vector_get_fast(m_raw_values[1],pos);
	}
	for( ; pos<m_n-NUM_END_NODES; ++pos){
		m_orientationTriangle[1].x += gsl_vector_get_fast(m_raw_values[0],pos);
		m_orientationTriangle[1].y += gsl_vector_get_fast(m_raw_values[1],pos);
	}
	for( ; pos<m_n; ++pos){
		m_orientationTriangle[2].x += gsl_vector_get_fast(m_raw_values[0],pos);
		m_orientationTriangle[2].y += gsl_vector_get_fast(m_raw_values[1],pos);
	}
	m_orientationTriangle[0].x /= NUM_END_NODES;
	m_orientationTriangle[0].y /= NUM_END_NODES;
	m_orientationTriangle[1].x /= m_n-2*NUM_END_NODES;
	m_orientationTriangle[1].y /= m_n-2*NUM_END_NODES;
	m_orientationTriangle[2].x /= NUM_END_NODES;
	m_orientationTriangle[2].y /= NUM_END_NODES;
}

/* Low order approximation of curve length */
double Gesture::evalCurveLength(){
	size_t pos;
	double curveLen = 0.0;

	for( pos=0; pos<NUM_EVALUATION_POINTS-1; ++pos){
		double tmpX, tmpY;
		tmpX = gsl_vector_get_fast(m_splineCurve[0],pos+1) - gsl_vector_get_fast(m_splineCurve[0],pos);
		tmpY = gsl_vector_get_fast(m_splineCurve[1],pos+1) - gsl_vector_get_fast(m_splineCurve[1],pos);
		curveLen += sqrt( tmpX*tmpX + tmpY*tmpY );
	}
	return curveLen;
}

/* Compare spline(t_n) with spline(t_n+CompareDistances[i]) and scale with length of polygon. */
void Gesture::evalDistances(){
	if( m_curvePointDistances == NULL ){
		m_curvePointDistances = (gsl_vector**) malloc( CompareDistancesNum*sizeof(gsl_vector*) );
		for( size_t i=0; i<CompareDistancesNum; ++i){
			m_curvePointDistances[i] = gsl_vector_alloc(NUM_EVALUATION_POINTS-CompareDistances[i]);
		}
	}

	const double curveScale = 1/evalCurveLength();

	/* 2. Eval distances for certain levels. */
	for( size_t i=0; i<CompareDistancesNum; ++i){
		double *out = m_curvePointDistances[i]->data;
		size_t start = 0;
		size_t end = CompareDistances[i];

		for( start=0 ; end < NUM_EVALUATION_POINTS ; ++start, ++end ){
			double tmpX, tmpY;
			tmpX = gsl_vector_get_fast(m_splineCurve[0],end) - gsl_vector_get_fast(m_splineCurve[0],start);
			tmpY = gsl_vector_get_fast(m_splineCurve[1],end) - gsl_vector_get_fast(m_splineCurve[1],start);
			*out++ = sqrt( tmpX*tmpX + tmpY*tmpY )*curveScale;
		}

	}
}

/* Looking at absolute and relative distances
 * of first and third point of m_orientationTriangle
 * and decide if this gesture is alike a closed curve.
 */
bool Gesture::isClosedCurve(/* float *outAbs, float *outRel*/){
	float dx,dy;
	float outAbs, outRel;

	dx = m_orientationTriangle[0].x-m_orientationTriangle[2].x;
	dy = m_orientationTriangle[0].y-m_orientationTriangle[2].y;
	float a = sqrt( dx*dx + dy*dy );

	dx = m_orientationTriangle[1].x-m_orientationTriangle[0].x;
	dy = m_orientationTriangle[1].y-m_orientationTriangle[0].y;
	float b = sqrt( dx*dx + dy*dy );

	dx = m_orientationTriangle[2].x-m_orientationTriangle[1].x;
	dy = m_orientationTriangle[2].y-m_orientationTriangle[1].y;
	float c = sqrt( dx*dx + dy*dy );

	outAbs = a;
	outRel = a/(a+b+c);

	if( outAbs < 10.0 || outRel < 0.2 )
		return true;

	return false;
}


void Gesture::setGestureName(const char* name){
	size_t len = strnlen(name, 255);
	if( m_gestureName != NULL ){
		free( m_gestureName );
	}

	char *gestureName = (char*) malloc( (len+1)*sizeof(char) );
	memcpy( gestureName, name, len*sizeof(char) );
	gestureName[len] = '\0';

	m_gestureName = gestureName;
}

const char* Gesture::getGestureName() const{
	return m_gestureName;
}


//=======================================================

GestureStore::GestureStore():
gestures()
{

}

GestureStore::~GestureStore(){

	//Release stored gestures
	std::vector<Gesture*>::iterator it = gestures.begin();
	const std::vector<Gesture*>::iterator itEnd = gestures.end();
	for( ; it != itEnd ; ++it ){
		delete (*it);
	}
}
/* All patters will be deleted in the destrutor. */
void GestureStore::addPattern(Gesture *pGesture){
	gestures.push_back(pGesture);
}

std::vector<Gesture*> & GestureStore::getPatterns() {
	return gestures;
}


/* Use very simple quadrature-formular to measure
 * the distance between the input spline S and the
 * stored splines G_i.
 *
 * TODO:
 * Use triangle inequality
 * dist(D_i,S) - dist(D_i,D_j) <= dist(D_j,S)
 * or other inequalities to 
 * to cut of some integrations.
 */
void GestureStore::compateWithPatterns(Gesture *pGesture, GesturePatternCompareResult &gpcr ){
	gpcr.minDist = FLT_MAX;
	gpcr.minGest = NULL;

	if( pGesture->getNumberOfRawSupportNodes() == 0 ) return;

	std::vector<Gesture*>::iterator it = gestures.begin();
	const std::vector<Gesture*>::iterator itEnd = gestures.end();

	double avgDist = 0.0;
	//Number of dist, which will be used for averaging
	int countDist = 0;
	size_t level = 0;

	std::vector<GestureDistance> distObjects; 
	std::vector<GestureDistance*> distPointers; //for sorting
	for( ; it != itEnd ; ++it ){
		distObjects.emplace_back( GestureDistance(pGesture, (*it)) );
	}
	for( auto& x: distObjects){
		distPointers.push_back(&x);
	}

	/* Compare on stage 1. This compare
	 * by the l2 norm on the v=m_curvePointDistances[0] vectors.
	 * d = (v_from, v_to)_2^2
	 * Note: No not forget that this norm differs from the 
	 * L2 norm of the underlying functions. There is no scaling
	 * by the grid size (vector lengths). 
	 * The missing scaling must respect if you define thresholds
	 * for nearby gestures.
	 */
	for( auto& x: distObjects){
		x.evalL2Dist(0);
	}
	std::sort(distPointers.begin(), distPointers.end(), sortFunctions[0]);

	/* Filtering out high distances and sort nearby gestures for other levels*/	
	size_t i = 0;
	size_t iMin = 3;//minimal number of gestures for next level
	size_t iCut;
	for( size_t iL = 1; iL<CompareDistancesNum; ++iL){
		double l2Limit = 10 * distPointers[0]->m_L2NormSquared[iL-1];
		iCut = iMin;
		for( auto& x: distPointers){
			++i;
			x->m_sorting_weight += i;
			if( i <= iMin ) continue;
			if( x->m_L2NormSquared[iL-1] < l2Limit ) continue;
			iCut = i;
			break;
		}
		distPointers.erase(distPointers.begin()+iCut, distPointers.end());

		for( auto& x: distPointers){
			x->evalL2Dist(iL);
		}
		std::sort(distPointers.begin(), distPointers.end(), sortFunctions[iL]);
		i = 0;
		//--iMin;
	}
	for( auto& x: distPointers){
		++i;
		x->m_sorting_weight += i;
	}
	std::sort(distPointers.begin(), distPointers.end(), sortByWeight);

	i = 0;
	//for( auto& x: distObjects){
	for( auto& x: distPointers){
		printf("%i %i %1.5f %1.5f %1.5f %1.5f %s\n", i,
				x->m_sorting_weight,
				x->m_L2NormSquared[0],
				x->m_L2NormSquared[1],
				x->m_L2NormSquared[2],
				x->m_L2NormSquared[3],
				x->m_to->getGestureName());
		++i;
	}

	gpcr.minGest = distPointers[0]->m_to;
	gpcr.minDist = distPointers[0]->m_L2NormSquared[0];
	gpcr.avgDist = gpcr.minDist;// TODO: Remove this stuff.
  printf("Curve distance to %s: %f\n", gpcr.minGest->getGestureName(), gpcr.minDist);

	/* //old
		 for( ; it != itEnd ; ++it ){
		 double dist = quadratureSquared( 
		 pGesture->m_curvePointDistances[level]->data,
		 (*it)->m_curvePointDistances[level]->data, 
		 pGesture->m_curvePointDistances[level]->size );
		 if( dist < gpcr.minDist ){
		 gpcr.minDist = dist;
		 gpcr.minGest = (*it);
		 }
		 ++countDist;
		 avgDist += dist;

		 printf("Curve distance: %f\n", dist);
		 }
		 gpcr.avgDist = avgDist/countDist;
		 */

}

double GestureDistance::evalL2Dist( size_t level){
	assert(level<CompareDistancesNum);
	m_L2NormSquared[level] = quadratureSquared( 
			m_from->m_curvePointDistances[level]->data,
			m_to->m_curvePointDistances[level]->data, 
			m_from->m_curvePointDistances[level]->size );
	return m_L2NormSquared[level];
}
