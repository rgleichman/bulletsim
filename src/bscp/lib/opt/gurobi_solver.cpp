#include "gurobi_solver.h"

void A_prod(const vector<MatrixXd> &As, int tau, int t, MatrixXd& result) {
  int NX = As[0].cols();
  result = MatrixXd::Identity(NX,NX);
  for (int i = tau; i < t; i++) {
    result = As[i] * result;
  }
  //cout << "Aprod tau, t = " << tau << ", " << t << endl;
  //cout << result << endl; ;
}

void build_G(const vector<MatrixXd> &As, SparseMatrix<double> &G) {
  int T = As.size();
  int NX = As[0].cols();

  //cout << "building G" << endl;
  G = SparseMatrix<double>(NX*(T+1),0);
  for (int col = 0; col < T; col++) {
    SparseMatrix<double,RowMajor> column(0,NX);
    SparseMatrix<double> block(NX,NX);
    for (int row = 0; row < T+1; row++) {
      if (row > col) {
        if (row == col+1)
          block = speye(NX);
        else
          block = toSparse(As[row-1]) * block;

      }
      SparseMatrix<double,RowMajor> tmp;
      vcat(column, block, tmp);
      column = tmp;
    }
    SparseMatrix<double> tmp2;
    hcat(G, column, tmp2);
    G = tmp2;
  }
  //cout << "building complete" << endl;
}

void build_G_T1(const vector<MatrixXd> &As, SparseMatrix<double> &G_T1) {
  int T = As.size();
  int NX = As[0].cols();

  G_T1 = SparseMatrix<double>(NX,0);
  SparseMatrix<double> block_A;
  for (int i = T; i > 0; i--) {
    if (i == T) block_A = speye(NX);
    else block_A = block_A * toSparse(As[i]);
    G_T1 = hcat(block_A, G_T1);
  }

}

void build_H(const vector<MatrixXd> &As, const vector<MatrixXd> &Bs, SparseMatrix<double> &H) {
  int T = As.size();
  int NX = As[0].cols();
  int NU = Bs[0].cols();

  //cout << "building H" << endl;
  H = SparseMatrix<double>(NX*(T+1),0);
  for (int col = 0; col < T; col++) {
    SparseMatrix<double,RowMajor> column(0,NU);
    SparseMatrix<double> block_A(NX,NU);
    for (int row = 0; row < T+1; row++) {
      SparseMatrix<double> block(NX,NU);
      if (row > col) {
        if (row == col+1)
          block_A = speye(NX);
        else
          block_A = toSparse(As[row-1]) * block_A;
        block = block_A * toSparse(Bs[col]);
      }
      SparseMatrix<double,RowMajor> tmp;
      vcat(column, block, tmp);
      column = tmp;
    }
    SparseMatrix<double> tmp2;
    hcat(H, column, tmp2);
    H = tmp2;
  }
  //cout << "building complete" << endl;
}

void build_H_T1(const vector<MatrixXd> &As, const vector<MatrixXd> &Bs, SparseMatrix<double> &H_T1) {
  int T = As.size();
  int NX = As[0].cols();

  H_T1 = SparseMatrix<double>(NX,0);
  SparseMatrix<double> block_A;
  for (int i = T; i > 0; i--) {
    if (i == T) block_A = speye(NX);
    else block_A = block_A * toSparse(As[i]);
    H_T1 = hcat(block_A * toSparse(Bs[i-1]), H_T1);
  }

}
void build_x0(const vector<MatrixXd> &As, const vector<VectorXd> &Cs, const VectorXd &x_start, const SparseMatrix<double> &G, VectorXd &x0) {
  int T = As.size();
  int NX = As[0].cols();
  x0 = VectorXd(NX*(T+1));

  VectorXd c_vec(NX*T);
  for (int t = 0; t < T; t++) {
    c_vec.segment(t*NX, NX) = Cs[t];
  }

  for (int t = 0; t < T+1; t++) {
    MatrixXd A_tau_t;
    A_prod(As, 0, t, A_tau_t);
    x0.segment(t*NX, NX) = A_tau_t * x_start;
  }

  x0 = x0 + G*c_vec;

}

void build_kx(int NU, VectorXd& x0, SparseMatrix<double> &result) {
  SparseMatrix<double> sparse_x0 = toSparse(x0.transpose());
  result = SparseMatrix<double>(0,0);
  for (int i = 0; i < NU; i++) {
    SparseMatrix<double> tmp;
    blkdiag(result, sparse_x0, tmp);
    result = tmp;
  }
}

void build_Qx(VectorXd x0, int NX, int NU, int T, SparseMatrix<double> &result) {

  result = SparseMatrix<double>(0,0);
  for (int t = 0; t < T; t++) {
    SparseMatrix<double> block;
    VectorXd x0_t = x0.segment(NX*t, NX);
    build_kx(NU, x0_t, block);
    SparseMatrix<double> tmp;
    blkdiag(result, block, tmp);
    result = tmp;
  }
}

void build_xbar(const vector<VectorXd>& X_bar, VectorXd& x_bar) {
  int T1 = X_bar.size();
  int NX = X_bar[0].rows();
  x_bar = VectorXd(T1*NX);
  for (int t = 0; t < T1; t++) {
    x_bar.segment(t*NX, NX) = X_bar[t];
  }
}

void build_ubar(const vector<VectorXd>& U_bar, VectorXd& u_bar) {
  int T = U_bar.size();
  int NU = U_bar[0].rows();
  u_bar = VectorXd(T*NU);
  for (int t = 0; t < T; t++) {
    u_bar.segment(t*NU,NU) = U_bar[t];
  }
}

void build_sample_matrix(const vector<MatrixXd> &W_bar, MatrixXd & W_s) {
  int T = W_bar.size();
  int NS = W_bar[0].cols();
  int NX = W_bar[0].rows();

  W_s = MatrixXd(NX*T, NS);
  for (int t = 0; t < T; t++) {
    for (int s = 0; s < NS; s++) {
      W_s.block(t*NX,s,NX,1) = W_bar[t].col(s);
    }
  }
}

void build_P(const vector<MatrixXd>& Ps, SparseMatrix<double>& P) {
	P = SparseMatrix<double>(0,0);
	for (int i = 0; i < Ps.size(); i++) {
		P = blkdiag(P, toSparse(Ps[i]));
	}
}

void build_pbar(const vector<VectorXd>& ps, VectorXd& pbar) {
	int num_entries = 0;
	for (int i = 0; i < ps.size(); i++) num_entries += ps[i].rows();
	pbar = VectorXd(num_entries);
	int index = 0;
	for (int i = 0; i < ps.size(); i++) {
		pbar.segment(index, ps[i].rows()) = ps[i];
		index += ps[i].rows();
	}
}


//void convex_gurobi_solver(const vector<VectorXd>& X_bar, const vector<VectorXd>& U_bar, const vector<MatrixXd>& W_bar,
//		const vector<MatrixXd>& As, const vector<MatrixXd>& Bs, const vector<VectorXd>& Cs,
//		const vector<MatrixXd>& Ps, const vector<VectorXd>& p_offsets,
//		const vector<VectorXd>& X_upper_limit, const vector<VectorXd>& X_lower_limit,
//		const vector<VectorXd>& U_upper_limit, const vector<VectorXd>& U_lower_limit,
//		const VectorXd &x_goal,
//		vector<VectorXd>& opt_X, vector<VectorXd>& opt_U, MatrixXd &opt_K, VectorXd & opt_u0,
//		const bool compute_policy) {


void convex_gurobi_solver(const TrajectoryInfo &nominal,
		const vector<TrajectoryInfo>& samples,
		vector<VectorXd>& opt_X, vector<VectorXd>& opt_U, MatrixXd &opt_K,
		VectorXd & opt_u0, vector<vector<VectorXd> >& opt_sample_X,
		vector<vector<VectorXd> >& opt_sample_U, const bool compute_policy) {

  int NX = nominal._X[0].rows();
  int NU = nominal._U[0].rows();
  int NS = samples.size();
  int T = nominal._U.size();
  int NG = nominal._goal_offset.rows();


  // Construct major matrices
  SparseMatrix<double> G, H, P;
  vector<SparseMatrix<double> > s_G(NS), s_H(NS), s_P(NS);
  VectorXd x0, x_bar, p_bar;
  vector<VectorXd> s_x0(NS), s_pbar(NS);


  Timer timer = Timer();
  //cout << "building matrices" << endl;
  build_G(nominal._A, G);
  build_H(nominal._A, nominal._B, H);
  build_x0(nominal._A, nominal._C, nominal._X[0], G, x0);
  build_xbar(nominal._X, x_bar);
  build_P(nominal._P, P);
  build_pbar(nominal._p_offset, p_bar);

//  for (int s = 0; s < NS; s++) {
//	  build_G(samples[s]._A, s_G[s]);
//	  build_H(samples[s]._A, samples[s]._B, s_H[s]);
//	  build_x0(samples[s]._A, samples[s]._C, samples[s]._X[0], s_G[s], s_x0[s]);
//	  build_P(samples[s]._P, s_P[s]);
//	  build_pbar(samples[s]._p_offset, s_pbar[s]);
//  }
  for (int s = 0; s < NS; s++) {
  	  build_G(nominal._A, s_G[s]);
  	  build_H(nominal._A, nominal._B, s_H[s]);
  	  build_x0(nominal._A, nominal._C, nominal._X[0], s_G[s], s_x0[s]);
  	  build_P(nominal._P, s_P[s]);
  	  build_pbar(nominal._p_offset, s_pbar[s]);
    }

  int NP = p_bar.rows();




//  cout << "NX = " << NX << endl;
//  cout << "NU = " << NU << endl;
//  cout << "NS = " << NS << endl;
//  cout << "NP = " << NP << endl;
//  cout << "T  = " << T  << endl;


  assert(P.rows() == p_bar.rows());
  assert(P.cols() == NX*(T+1));
  //x0_T1 = x0.segment(NX*T, NX);
  //cout << "Time to build:" << timer.elapsed() << endl;
  timer.restart();

  //cout << "Building constraints" << endl;

  /* Formulate as a standard Quadratic Program
   *
   * variables [r; q_vec; x; u; goal_diff; p; sample_x; sample_u; sample_diff]
   * r -> NU*T
   * q_vec -> NX*NU*T
   * x -> NX*(T+1) (represents the difference from a nominal trajectory X_bar)
   * u -> NU*T
   * goal_diff -> NG
   * p -> NP (represents the distance penetration of each collision)
   * sample_x -> NS*NX*(T+1)
   * sample_u -> NU*NS*T
   * sample_diff -> NG*NS
   * sample_p -> NP*NS
   */

  int ALL_VARS = NU*T + NX*NU*T + NX*(T+1) + NU*T + NG + NP + NX*NS*(T+1) + NU*NS*T + NG*NS + NP*NS;


  /* Build constraint matrices */

  /*
   * CONSTRAINT 1
   * x - x_bar = H*u + x0 - x_bar -> x - H*u = x0 - x_bar
   */
  SparseMatrix<double> x_A;
  VectorXd x_b(NX*(T+1));
  hcat(spzeros(NX*(T+1), NU*T + NX*NU*T), speye(NX*(T+1)), x_A); // r, q, x
  x_A = hcat(x_A, -H); // u
  x_A = hcat(x_A, spzeros(NX*(T+1), NG + NP + NX*NS*(T+1) + NU*NS*T + NG*NS + NP*NS));
  //goal_diff, p, sample_x, sample_u, sample_diff, sample_p
  x_b = x0 - x_bar;


  /*
   * CONSTRAINT 2
   * u = Q*x0 + r -> u - r - Q*x0 = 0
   */
  SparseMatrix<double> u_A;
  VectorXd u_b(NU*T);

  SparseMatrix<double> Qx0;
  build_Qx(x0, NX, NU, T, Qx0);
  hcat(-1*speye(NU*T), -Qx0, u_A); // r, q
  u_A = hcat(u_A, spzeros(NU*T, NX*(T+1))); // x
  u_A = hcat(u_A, speye(NU*T)); // u
  u_A = hcat(u_A, spzeros(NU*T, NG +NP + NX*NS*(T+1) + NU*NS*T + NG*NS + NP*NS)); // goal_diff, p, sample_x, sample_u, sample_diff, sample_p
  u_b = VectorXd::Zero(NU*T);


  /*
   * CONSTRAINT 3
   * g = G(x_t + bar_x_t) + g(bar_x_t)
   * g - Gx_t = G*bar_x_t + g(bar_x_t)
   */

  SparseMatrix<double> g_A;
  VectorXd g_b(NG);
  hcat(spzeros(NG, NU*T + NX*NU*T + NX*T), -toSparse(nominal._Goal), g_A); // r, q, X
  g_A = hcat(g_A, spzeros(NG, NU*T)); // U
  g_A = hcat(g_A, speye(NG)); // goal_diff
  g_A = hcat(g_A, spzeros(NG, NP + NX*NS*(T+1) + NU*T*NS + NS*NG + NP*NS)); //p, sample_x, sample_u, sample_diff, sample_p
  g_b = nominal._Goal * nominal._X[T] + nominal._goal_offset;


  /*
   * CONSTRAINT 4
   * sample_x - sample_x_bar = sample_H*sample_u + sample_G*sample_w + sample_x0 - sample_x_bar
   * -> sample_x - sample_H*sample_u = sample_x0 + sample_G*sample_w - sample_x_bar
   */

  SparseMatrix<double> sx_A;
  VectorXd sx_b(NX*NS*(T+1));
  SparseMatrix<double> sx_H;
  for (int s = 0; s < NS; s++) sx_H = blkdiag(sx_H, -s_H[s]);

  hcat(spzeros(NX*NS*(T+1), NU*T + NX*NU*T + NX*(T+1) + NU*T + NG + NP), speye(NX*NS*(T+1)), sx_A); // r, q, x, u, goal_diff, p, sample_x
  sx_A = hcat(sx_A, sx_H); // sample_u (negative sign taken care of)
  sx_A = hcat(sx_A, spzeros(NX*NS*(T+1), NG*NS + NP*NS)); // sample_diff, sample_p
  for (int s = 0; s < NS; s++)
	  sx_b.segment(s*NX*(T+1), NX*(T+1)) = s_x0[s] + s_G[s]*samples[s].W_vec() - samples[s].X_vec();


  /*
   * CONSTRAINT 5
   * sample_u = Q(x0+G*w_s) + r -> sample_u - Q(x0 + G*w_s) - r = 0
   */

  SparseMatrix<double> su_A;
  VectorXd su_b(NU*T*NS);

  SparseMatrix<double> su_A_Q_block(0,NU*NX*T);
  SparseMatrix<double> su_A_r_block(0,NU*T);
  for (int i = 0; i < NS; i++) {
    SparseMatrix<double> Qx0Gws;
    build_Qx(s_x0[i] + G*samples[i].W_vec(), NX, NU, T, Qx0Gws);
    su_A_Q_block = vcat(su_A_Q_block, -Qx0Gws);
    su_A_r_block = vcat(su_A_r_block, -speye(NU*T));
  }
  hcat(su_A_r_block, su_A_Q_block, su_A); // r, q
  su_A = hcat(su_A, spzeros(NU*T*NS, NX*(T+1) + NU*T + NP + NG + NX*NS*(T+1))) ; // x, u, goal_diff, p, sample_x
  su_A = hcat(su_A, speye(NU*T*NS)); // sample_u
  su_A = hcat(su_A, spzeros(NU*T*NS, NG*NS + NP*NS)); //sample_diff, sample_p
  su_b = VectorXd::Zero(NU*T*NS);


  /*
   * CONSTRAINT 6
   * sample_g - Gsample_x_t = G*sample_bar_x_t + g(sample_bar_x_t)
   */

  SparseMatrix<double> sd_A;
  VectorXd sd_b(NG*NS);
  SparseMatrix<double> sd_A_sample_X_block;

  for (int s = 0; s < NS; s++) {
	  SparseMatrix<double> sd_A_sample_X_subblock;
	  hcat(spzeros(NG,NX*T), -toSparse(samples[s]._Goal), sd_A_sample_X_subblock);
	  sd_A_sample_X_block = blkdiag(sd_A_sample_X_block, sd_A_sample_X_subblock);
  }

  hcat(spzeros(NG*NS, NU*T + NX*NU*T + NX*(T+1) + NU*T + NG + NP) , sd_A_sample_X_block,
		  sd_A); // r, q,  x, u, goal_diff, p, sample_x
  sd_A = hcat(sd_A, spzeros(NG*NS, NU*T*NS)); // sample_u
  sd_A = hcat(sd_A, speye(NG*NS)); // sample_diff
  sd_A = hcat(sd_A, spzeros(NG*NS, NP*NS)); //sample_p

  for (int s = 0; s < NS; s++) {
	  sd_b.segment(s*NG, NG) = samples[s]._Goal * samples[s]._X[T] + samples[s]._goal_offset;
  }


  // INEQUALITY CONSTRAINTS
  /*
   * CONSTRAINT 7
   * a is a slope parameter with a > 0
   * eps is a safety parameter
   * p >= a(P(X+X_bar) + p_bar + eps) -> p - aPX >= a(PX_bar + p_bar + eps)
   */

  double a = 10.0;
  double eps = 0.0;
  SparseMatrix<double> p_A;
  VectorXd p_b(NP);
  hcat(spzeros(NP, NU*T + NX*NU*T), a*P, p_A); // r, q, X;
  p_A = hcat(p_A, spzeros(NP, NU*T + NG)); // U, goal_diff
  p_A = hcat(p_A, speye(NP)); // p
  p_A = hcat(p_A, spzeros(NP, NX*NS*(T+1) + NU*T*NS + NG*NS + NP*NS)); // sample_x, sample_u, sample_diff, sample_p
  p_b = -a*(p_bar + P*x_bar + eps*VectorXd::Ones(NP));

  /*
   * CONSTRAINT 8
   * a is a slope parameter with a > 0
   * eps is a safety parameter
   */
  SparseMatrix<double> sp_A;
  VectorXd sp_b(NP*NS);
  SparseMatrix<double> sp_A_sample_X_block;

  for (int s = 0; s < NS; s++) {
	  sp_A_sample_X_block = blkdiag(sp_A_sample_X_block, a*s_P[s]);
  }
  hcat(spzeros(NP*NS, NU*T + NX*NU*T + NX*(T+1) + NU*T + NG + NP), sp_A_sample_X_block, sp_A); //r, q, X, U, goal_diff, p, sample_x
  sp_A = hcat(sp_A, spzeros(NP*NS, NS*NU*T + NG*NS)); // sample_u, sample_diff
  sp_A = hcat(sp_A, speye(NP*NS)); // sample_p

  for (int s = 0; s < NS; s++) {
	  sp_b.segment(s*NP, NP) = -a*(s_pbar[s] + s_P[s]*samples[s].X_vec() + eps*VectorXd::Ones(NP));
  }


  // Concatenate constraints
  SparseMatrix<double,RowMajor> A;
  vcat(x_A, u_A, A); // Constraint 1, 2
  A = vcat(A, g_A); // Constraint 3
  A = vcat(A, sx_A); // Constraint 4
  A = vcat(A, su_A); // Constraint 5
  A = vcat(A, sd_A); // Constraint 6
  A = vcat(A, p_A); // Constraint 7
  A = vcat(A, sp_A); // Constraint 8
  VectorXd b(x_b.rows() + u_b.rows() + g_b.rows() + sx_b.rows() + su_b.rows() + sd_b.rows() + p_b.rows() + sp_b.rows());
  b << x_b, u_b, g_b, sx_b, su_b, sd_b, p_b, sp_b; // Constraint 1,2,3,4,5,6,7,8


  //cout << "constraints complete: " << timer.elapsed() << endl;

  /* Build objective matrices */
  VectorXd diag(ALL_VARS);
  VectorXd Qr_obj  = 0.00000     * VectorXd::Ones(NU*T);     // r prior to prevent overfitting on samples
  VectorXd Qq_obj  = 0.00001     * VectorXd::Ones(NU*NX*T);  // q prior to prevent overfitting on samples
  VectorXd Qx_obj  = 0.0     * VectorXd::Ones(NX*(T+1)); // x
  VectorXd Qu_obj  = 0.001   * VectorXd::Ones(NU*T);     // u
  VectorXd Qg_obj  = 10.0    * VectorXd::Ones(NG);       // goal_diff
  VectorXd Qp_obj  = 0.0     * VectorXd::Ones(NP);       // p
  VectorXd Qsx_obj = 0.0     * VectorXd::Ones(NX*NS*(T+1));
  VectorXd Qsu_obj = 0.001/NS * VectorXd::Ones(NU*NS*T);  // sample_control
  VectorXd Qsg_obj = 1.0/NS * VectorXd::Ones(NG*NS);    // sample_diff
  VectorXd Qsp_obj = 0.0    * VectorXd::Ones(NP*NS);  // sample_p

  diag <<  Qr_obj, Qq_obj, Qx_obj, Qu_obj, Qg_obj, Qp_obj, Qsx_obj, Qsu_obj, Qsg_obj, Qsp_obj;
  SparseMatrix<double> Q_obj = spdiag(diag);

  VectorXd c_dense(ALL_VARS);
  VectorXd Cr_obj  = 0.0     * VectorXd::Ones(NU*T);        // r
  VectorXd Cq_obj  = 0.0     * VectorXd::Ones(NU*NX*T);     // q
  VectorXd Cx_obj  = 0.0     * VectorXd::Ones(NX*(T+1));    // x
  VectorXd Cu_obj  = 0.0     * VectorXd::Ones(NU*T);        // u
  VectorXd Cg_obj  = 0.0     * VectorXd::Ones(NG);          // goal_diff
  VectorXd Cp_obj  = 1.0     * VectorXd::Ones(NP);          // p
  VectorXd Csx_obj = 0.0     * VectorXd::Ones(NX*NS*(T+1)); // sample_x
  VectorXd Csu_obj = 0.0     * VectorXd::Ones(NU*NS*T);     // sample_u
  VectorXd Csg_obj = 0.0     * VectorXd::Ones(NG*NS);       // sample_diff
  VectorXd Csp_obj = 1.0     * VectorXd::Ones(NP*NS);       // sample_p

  c_dense << Cr_obj, Cq_obj, Cx_obj, Cu_obj, Cg_obj, Cp_obj, Csx_obj, Csu_obj, Csg_obj, Csp_obj;
  SparseVector<double> c_obj = toSparseVector(c_dense);

  double lb[ALL_VARS], ub[ALL_VARS];

  // default bounds
  for (int i = 0; i < ALL_VARS; i++) {
    lb[i] = -1e30; // treated as - infinity
    ub[i] =  1e30; // treated as + infinity
  }

  // state and control constraints
  int ind = NU*T + NX*NU*T; // r, q

  for (int t = 0; t < T+1; t++) {
	  for (int i = 0; i < NX; i++) {
		  lb[ind + t*NX + i] = nominal._X_lower_limit[t][i];
		  ub[ind + t*NX + i] = nominal._X_upper_limit[t][i];
	  }
  }


  ind += NX*(T+1); // X

  for (int t = 0; t < T; t++) {
	  for (int i = 0; i < NU; i++) {
		  lb[ind + t*NU + i] = nominal._U_lower_limit[t][i];
		  ub[ind + t*NU + i] = nominal._U_upper_limit[t][i];
	  }
  }
  ind += NU*T; // U

  ind += NG; // goal_diff

  for (int i = 0; i < NP; i++) {
	  lb[ind + i] = 0;
  }

  ind += NP; // p

//
//  for (int s = 0; s < NS; s++) {
//	  for (int t = 0; t < T+1; t++) {
//		  for (int i = 0; i < NX; i++) {
//			  lb[ind + s*(T+1)*NX + t*NX + i] = samples[s]._X_lower_limit[t][i];
//			  ub[ind + s*(T+1)*NX + t*NX + i] = samples[s]._X_upper_limit[t][i];
//		  }
//	  }
//  }

  ind += NX*NS*(T+1); // sample_x

//  for (int s = 0; s < NS; s++) {
//	  for (int t = 0; t < T; t++) {
//		  for (int i = 0; i < NU; i++) {
//			  lb[ind + s*T*NU + t*NU + i] = samples[s]._U_lower_limit[t][i];
//			  ub[ind + s*T*NU + t*NU + i] = samples[s]._U_upper_limit[t][i];
//		  }
//	  }
//  }

  ind += NU*NS*T; // sample_u

  ind += NG*NS; // sample_diff

  for (int i = 0; i < NP*NS; i++) {
	  lb[ind + i] = 0;
  }

  ind += NP*NS; // sample_p



  char constraint_sense[b.rows()];
  for (int i = 0; i < b.rows(); i++) {
    constraint_sense[i] = GRB_EQUAL;
  }
  for (int i = b.rows() - NP - NP*NS; i < b.rows(); i++) {
  //for(int i = b.rows() - NP; i < b.rows(); i++) {
  	constraint_sense[i] = GRB_GREATER_EQUAL;
  }

  VectorXd opt_sol;
  double obj;

  cout << "calling dense" << endl;

  //cout << "start on dense optimize: " << endl;
  timer.restart();
  GRBEnv* env = new GRBEnv();
  dense_optimize(env,ALL_VARS,c_obj,Q_obj,A,constraint_sense,b,lb,ub,NULL,opt_sol,&obj);
  delete env;

  //cout << "dense optimize time: " << timer.elapsed() << endl;
  //parse solution
  ind = 0;
  VectorXd opt_r  = opt_sol.segment(ind, NU*T); ind += NU*T;
  VectorXd opt_q  = opt_sol.segment(ind, NU*NX*T); ind += NU*NX*T;
  VectorXd opt_x  = opt_sol.segment(ind, NX*(T+1)); ind += NX*(T+1);
  VectorXd opt_u  = opt_sol.segment(ind, NU*T); ind += NU*T;
  VectorXd opt_d  = opt_sol.segment(ind, NG); ind += NG;
  VectorXd opt_p  = opt_sol.segment(ind, NP); ind += NP;
  VectorXd opt_sx = opt_sol.segment(ind, NX*NS*(T+1)); ind += NX*NS*(T+1);
  VectorXd opt_su = opt_sol.segment(ind, NU*NS*T); ind += NU*NS*T;
  VectorXd opt_sd = opt_sol.segment(ind, NG*NS); ind += NG*NS;
  VectorXd opt_sp = opt_sol.segment(ind, NP*NS); ind += NP*NS;

  // parse X
  opt_x = opt_x + x_bar;
  opt_X.resize(T+1);
  for (int i = 0; i < T+1; i++) {
    opt_X[i] = opt_x.segment(i*NX, NX);
  }

  // parse U
  opt_U.resize(T);
  for (int i = 0; i < T; i++) {
    opt_U[i] = opt_u.segment(i*NU, NU);
  }

  // parse Q
  SparseMatrix<double> opt_Q(0,0);
  //vector<SparseMatrix<double> > Q_diag(T);
  for (int i = 0; i < T; i++) {
    MatrixXd Q_diag_t(NU, NX);
    VectorXd Q_diag_t_vec = opt_q.segment(NX*NU*i, NX*NU);
    for (int r = 0; r < NU; r++) {
      Q_diag_t.row(r) = Q_diag_t_vec.segment(r*NX, NX).transpose();
    }
    //Q_diag[i] = toSparse(Q_diag_t);
    opt_Q = blkdiag(opt_Q, toSparse(Q_diag_t));
  }
  opt_Q = hcat(opt_Q, spzeros(NU*T, NX));
  //cout << opt_Q << endl;
  //cout << opt_r << endl;

  // parse opt_sample_X
  VectorXd opt_sample_x = opt_sx; // s_x_bar is added in the loop

  opt_sample_X.resize(NS);
  for (int s = 0; s < NS; s++) {
	  vector<VectorXd> opt_sample_X_t(T+1);
	  VectorXd opt_sample_x_t = opt_sample_x.segment(s*NX*(T+1), NX*(T+1)) + samples[s].X_vec();
	  for (int t = 0; t < T+1; t++) {
		  opt_sample_X_t[t] = opt_sample_x_t.segment(t*NX, NX);
	  }
	  opt_sample_X[s] = opt_sample_X_t;
  }

  // parse opt_sample_U

  opt_sample_U.resize(NS);
  for (int s = 0; s < NS; s++) {
	  vector<VectorXd> opt_sample_U_t(T);
	  VectorXd opt_sample_u_t = opt_su.segment(s*NU*T, NU*T);
	  for (int t = 0; t < T; t++) {
		  opt_sample_U_t[t] = opt_sample_u_t.segment(t*NU, NU);
	  }
	  opt_sample_U[s] = opt_sample_U_t;
  }


  if (compute_policy) {
    SparseMatrix<double> F_inv = speye(NU*T) + opt_Q*H;
    // Solve the system F_inv * K = Q
    //MatrixXd K;
    env = new GRBEnv();
    //cout << "solving large system" << endl;
    solve_matrix_linear_system(env, F_inv, opt_Q, opt_K);
    delete env;

    cout << opt_Q << endl;
    cout << opt_K << endl;

    cout << "solving small sys" << endl;
    env = new GRBEnv();
    solve_linear_system(env, F_inv, opt_r, opt_u0);

    cout << opt_r << endl;
    cout << opt_u0 << endl;
    delete env;
  } else {
    opt_K = opt_Q;
    opt_u0 = opt_r;
  }

  //PastixLU<SparseMatrix<double> > solver;
  //solver.compute(F_inv);
  //K = solver.solve(toDense(opt_Q));

  //cout << F_inv*K << endl;
  //ConjugateGradient<SparseMatrix<double> > solver;
  //BiCGSTAB<SparseMatrix<double> > solver(F_inv);
  //SparseMatrix<double> K(NU*T, NX*(T+1));
  //solver.compute(F_inv);
  //K = solver.solve(opt_Q);
  // Solve the system F_inv * u0 = r
  //ConjugateGradient<SparseMatrix<double> > solver;
  //SparseMatrix<double> u0(NU*T, 1);
  //u0 = solver.solve(toSparse(opt_r));


  /*
  cout << "opt_x" << endl;
  cout << opt_x << endl;
  cout << "x_bar" << endl;
  cout << x_bar << endl;
  cout << "opt_Q*x0 + opt_r" << endl;
  cout << opt_Q*x0 + opt_r << endl;
  cout << "K*opt_x + u0" << endl;
  cout << K*opt_x + u0 << endl;
  cout << "opt_u" << endl;
  cout << opt_u << endl;

  */

  /*
  cout << "X" << endl;
  cout << opt_x << endl;

  cout << "X_bar" << endl;
  cout << x_bar << endl;

  cout << "U" << endl;
  cout << opt_u << endl;

  cout << "diff X" << endl;
  cout << opt_x - x_bar << endl;

  cout << "diff U" << endl;
  cout << opt_u << endl;
  */

}

