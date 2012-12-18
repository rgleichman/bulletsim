#include "scp_solver.h"
#include "gurobi_solver.h"
#include "timer.h"
#include "trajectory_util.h"
#include <algorithm>
#include "eigen_io_util.h"

void scp_solver(Robot &r, const vector<VectorXd>& X_bar,
		const vector<VectorXd>& U_bar, const vector<MatrixXd>& W_bar,
		const double rho_x, const double rho_u, Robot::GoalFunc g,
		Robot::GoalFuncJacboian dg, const int N_iter, vector<VectorXd>& opt_X,
		vector<VectorXd>& opt_U, MatrixXd& opt_K, VectorXd& opt_u0) {

  int NX = X_bar[0].rows();
  int NU = U_bar[0].rows();
  int T = U_bar.size();
  int NS = W_bar[0].cols();
  assert(T+1 == X_bar.size());
  assert(g != NULL);

  cout << "NX = " << NX << endl;
  cout << "NU = " << NU << endl;
  cout << "T  = " << T  << endl;
  cout << "NS = " << NS << endl;


  vector<VectorXd> X_scp(T+1), U_scp(T); 
  for (int i = 0; i < T+1; i++) 
    X_scp[i] = X_bar[i];
  for (int i = 0; i < T; i++)
    U_scp[i] = U_bar[i]; 


  TrajectoryInfo nominal = TrajectoryInfo(X_scp, U_scp, g, dg, rho_x, rho_u);
  vector<vector<VectorXd> > N_s_bar;
  index_by_sample(W_bar, N_s_bar);
  vector<TrajectoryInfo> samples;

  vector<MatrixXd> rt_W_dist(T);
  vector<vector<VectorXd> > W_s_bar(NS);

  if (NX == r._NB) {
	  rt_W_dist = nominal.rt_belief_noise(r);
  } else {
	  for (int t = 0; t < T; t++) {
		  rt_W_dist[t] = MatrixXd::Identity(NX,NX);
	  }
  }

  for(int s = 0; s < NS; s++) {
	  W_s_bar[s] = vector<VectorXd>(T);
	  for (int t = 0; t < T; t++) {
		  W_s_bar[s][t] = rt_W_dist[t]*N_s_bar[s][t];
	  }
  }

  for (int i = 0; i < NS; i++) {
	  samples.push_back(TrajectoryInfo(X_scp, U_scp, W_s_bar[i], g, dg, rho_x, rho_u));
	  samples[i].integrate(r);
  }


  Timer timer = Timer();
  for (int iter = 0; iter < N_iter; iter++) 
  {

	 if (NX == r._NB) {
		 rt_W_dist = nominal.rt_belief_noise(r);
	 } else {
		 for (int t = 0; t < T; t++) {
			 rt_W_dist[t] = MatrixXd::Identity(NX,NX);
		 }
	 }

	 for(int s = 0; s < NS; s++) {
		 W_s_bar[s] = vector<VectorXd>(T);
		 for (int t = 0; t < T; t++) {
			 W_s_bar[s][t] = rt_W_dist[t]*N_s_bar[s][t];
		 }
	 }


	nominal.update(r); // updates jacobians and bounds
	for (int s = 0; s < NS; s++) {
		samples[s].set(samples[s]._X, samples[s]._U, W_s_bar[s]);
		samples[s].update_sample(r);
	}

    // Setup variables for sending to convex solver
    vector<VectorXd> opt_X, opt_U;
    vector<vector<VectorXd> > opt_sample_X, opt_sample_U;
    //Send to convex solver
    convex_gurobi_solver(nominal, samples, opt_X, opt_U, opt_K, opt_u0,
    		opt_sample_X, opt_sample_U, false);//iter == (N_iter - 1));
//    cout << "X: " << opt_X[0].transpose() << endl;
//    for (int i = 0; i < opt_U.size(); i++) {
//    	cout << "U: " << opt_U[i].transpose() << endl;
//    	cout << "X: " << opt_X[i+1].transpose() << endl;
//
//    }
    nominal.set(opt_X, opt_U);
    nominal.integrate(r); //shooting

    for (int s = 0; s < NS; s++) {
    	//samples[s].set(opt_X, opt_U); //hack for now
    	samples[s].set(opt_X, opt_sample_U[s]);
    	samples[s].integrate(r);
    }

    cout << "Iter " << iter << " time: " << timer.elapsed() << endl; 
  }


  opt_X = nominal._X;
  opt_U = nominal._U;


}

