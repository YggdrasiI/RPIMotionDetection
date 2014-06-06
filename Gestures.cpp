#include "Gestures.h"

Gesture::Gesture( cBlob &blob):
m_n(0),m_ncoeffs(0),m_nbreak(0),
		//m_grid(NULL),
		//m_bw(NULL),
		m_ReducedBasis(NULL)
{
	//init static variables
	if( mw == NULL ){
		initStaticGestureVariables();
	}

	m_raw_values[0]= NULL;
	m_raw_values[1]= NULL;
	/*
	m_c[0]= NULL;
	m_c[1]= NULL;
	m_cov[0]= NULL;
	m_cov[1]= NULL;
	*/

#ifdef WITH_HISTORY

	if( blob.history.get() != nullptr ){

		/* 0. Setup sizes and init vectors */
		//size_t m_n = std::min( 1 + blob.history.get()->size(), N);
		m_n = 1 + blob.history.get()->size();
		if( m_n > N ) m_n = N;

		m_ncoeffs = NCOEFFS_FUNC(m_n);
		m_nbreak = m_ncoeffs + 2 - K;
		
		/* Note: m_nbreak is size_t and could be underflow.
		 * Moreover, m_nbreak must be at least 2 for a successful
		 * bspline initialisation.
		 * This check was replaced because m_ncoeffs has the
		 * correct lower bound, now. It's requred to check m_n directly.
			*/
		//if( m_nbreak-2 < 1000 )
		if( m_n > 10 )	
		{


			printf("n,nc,nb=%u,%u,%u \n", m_n, m_ncoeffs, m_nbreak);

			//m_grid = gsl_vector_alloc( m_nbreak/*+2*(K-1)*/ );
			m_ReducedBasis = gsl_matrix_alloc(m_n, m_ncoeffs);

			m_raw_values[0] = gsl_vector_alloc(m_n);
			m_raw_values[1] = gsl_vector_alloc(m_n);

			/*
			m_c[0] = gsl_vector_alloc(m_ncoeffs);
			m_c[1] = gsl_vector_alloc(m_ncoeffs);
			m_cov[0] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
			m_cov[1] = gsl_matrix_alloc(m_ncoeffs, m_ncoeffs);
			*/

			//m_bw = gsl_bspline_alloc(K, m_nbreak);
			const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;

			/* 1. Filling vectors */
			//gsl_vector_set(m_grid, 0, 0.0);
			const gsl_vector_const_view subrow = gsl_matrix_const_subrow ( FullBasis[basisIndex], 0, 0, m_ncoeffs);
			gsl_matrix_set_row( m_ReducedBasis, 0, &subrow.vector );

			gsl_vector_set(m_raw_values[0], 0, blob.location.x);
			gsl_vector_set(m_raw_values[1], 0, blob.location.y);

			int knot=0;
			int duration=blob.duration;
			int globKnot;
			std::deque<cBlob>::iterator it = blob.history.get()->begin();
			const std::deque<cBlob>::iterator itEnd = blob.history.get()->end();

			for ( ; it != itEnd ; ++it){
				//go to next global knot.
				globKnot = duration - (*it).duration + 1; /* TODO Ok, die +1 sollte weg und duration bei den Blobs korrigiert werden. */
				++knot;

				if( globKnot >= N ){
					fprintf(stderr,"%s:Excess maximal duration for gestures.\n",__FILE__);
					/* The tracked data is to far in the past. Cut of at this position and short
					 * the dimension of the basis matrix.
					 * Note: The selection of m_ncoeffs could be to high for the new m_n.
					 */
					m_n = knot;
					m_ReducedBasis->size1 = m_n; //Now, the internal array size not matching anymore.
					break;
				}

				const gsl_vector_const_view subrow2 = gsl_matrix_const_subrow( FullBasis[basisIndex], globKnot, 0, m_ncoeffs);
				//gsl_vector_set( m_grid, knot, gsl_vector_get( FullGrid, globKnot ) );
				gsl_matrix_set_row( m_ReducedBasis, knot, &subrow2.vector );
			}

			//printf("Matrix:\n");
			//gsl_matrix_fprintf(stdout,m_ReducedBasis, "%f");

			//gsl_bspline_knots(m_grid, m_bw);//internally, m_grid data will be copied, which is omitable

		}else{
			// To less points
			m_n = 0;
		}
	}


#else
	fprinf(stderr, "Gesture construtor require history of blob position.\n");
#endif
}

Gesture::~Gesture(){

	//gsl_vector_free(m_grid);
	gsl_matrix_free(m_ReducedBasis);

	gsl_vector_free(m_raw_values[0]);
	gsl_vector_free(m_raw_values[1]);
	/*
	gsl_vector_free(m_c[0]);
	gsl_vector_free(m_c[1]);
	gsl_matrix_free(m_cov[0]);
	gsl_matrix_free(m_cov[1]);
	*/

	//gsl_bspline_free(m_bw);
}


void Gesture::evalSpline(){
	if( m_n == 0 ) return;

	/* Eindampfen des Workspaces auf die geringere Dimension.
	 */
	gsl_multifit_linear_realloc(mw, m_n, m_ncoeffs);

	const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;
	for( size_t d=0; d<DIM; ++d){
		double chisq;
		gsl_multifit_linear(m_ReducedBasis, m_raw_values[d], Global_c[basisIndex][d], Global_cov[basisIndex][d], &chisq, mw);
		//gsl_multifit_linear(m_ReducedBasis, m_raw_values[d], m_c[d], m_cov[d], &chisq, mw);
		//gsl_vector_fprintf(stdout, m_c[d], "%f ");
		//printf("====\n");
	}

}

void Gesture::plotSpline(double *outY, double *outY, int *outLen ){
		double ti,xi, yi, yerr;
		size_t j;
		
		outX = new double[NUM_EVALUATION_POINTS]; 
		outY = new double[NUM_EVALUATION_POINTS]; 
		*outLen = NUM_EVALUATION_POINTS;

		const size_t basisIndex = m_ncoeffs-NCOEFFS_MIN;
		double t_step = 1/(NUM_EVALUATION_POINTS-1);
		double t_pos = 0.0;

		for ( j=0; j<NUM_EVALUATION_POINTS; ++j, t_pos+=t_step )
		{

			//Nun berechne ich die Basiswerte ja doch in jedem Objekt...
			gsl_bspline_eval(t_pos, B, bw[basisIndex]);
			//gsl_multifit_linear_est(B, c1, cov, &xi, &yerr);
			//gsl_multifit_linear_est(B, c2, cov, &yi, &yerr);
			gsl_multifit_robust_est(B, c1, cov, &xi, &yerr);
			gsl_multifit_robust_est(B, c2, cov, &yi, &yerr);
			in[j].x = xi;
			in[j].y = yi;
			//printf("%f, %f, %f\n",ti,xi,yi);
		}
	}
