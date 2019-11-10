/**
 * @ file LinearFE1D.cc
 * @ brief NPDE homework LinearFE1D code
 * @ author Christian Mitsch
 * @ date 01.03.2019
 * @ copyright Developed at ETH Zurich
 */

#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>
#include <cmath>
#include <iostream>
#include <vector>

typedef Eigen::SparseMatrix<double> SparseMatrix;
typedef Eigen::Triplet<double> Triplet;
typedef Eigen::VectorXd Vector;

namespace LinearFE1D {

// Calculate the matrix entries corresponding to the integral containg alpha using composite midpoint rule
/* SAM_LISTING_BEGIN_1 */
template <typename FUNCTOR1>
std::vector<Triplet> mat_alpha(const Eigen::VectorXd &mesh, FUNCTOR1&& alpha) {
  
  unsigned M = mesh.size() - 1;

  std::vector<Triplet> triplets;
  triplets.reserve(3 * M + 1);

  // BEGIN-SOLUTION
  double tmp_diag, tmp_offd;
  double dx_left, dx_right;

  // diagonal entries
  // first and last entry need to be calculated separately
  dx_right = mesh(1) - mesh(0);
  tmp_diag = alpha((mesh(1) + mesh(0)) / 2.) / dx_right;
  triplets.push_back(Triplet(0, 0, tmp_diag));

  dx_left = mesh(M) - mesh(M - 1);
  tmp_diag = alpha((mesh(M) + mesh(M - 1)) / 2.) / dx_left;
  triplets.push_back(Triplet(M, M, tmp_diag));

  // other diagonal entries
  for (unsigned i = 1; i < M; ++i) {
    dx_left = mesh(i) - mesh(i - 1);
    dx_right = mesh(i + 1) - mesh(i);
    tmp_diag = alpha((mesh(i - 1) + mesh(i)) / 2.) / dx_left +
               alpha((mesh(i) + mesh(i + 1)) / 2.) / dx_right;
    triplets.push_back(Triplet(i, i, tmp_diag));
  }

  // off-diagonal entries
  for (unsigned i = 0; i < M; ++i) {
    dx_right = mesh(i + 1) - mesh(i);
    tmp_offd = -alpha((mesh(i) + mesh(i + 1)) / 2.) / dx_right;
    triplets.push_back(Triplet(i+1, i, tmp_offd));
    triplets.push_back(Triplet(i, i+1, tmp_offd));
  }
  // END-SOLUTION
  return triplets;
}
/* SAM_LISTING_END_1 */

// Calculate the matrix entries corresponding to the integral containg gamma
// trapezoidal rule
/* SAM_LISTING_BEGIN_2 */
template <typename FUNCTOR1>
std::vector<Triplet> mat_gamma(const Eigen::VectorXd &mesh, FUNCTOR1&& gamma) {
  
  unsigned M = mesh.size() - 1;

  std::vector<Triplet> triplets;
  triplets.reserve(2*M + 1);

  // BEGIN-SOLUTION
  double tmp_diag, tmp_offd;
  double dx;

  // diagonal entries
  // first and last entry need to be calculated separately
  dx = mesh(1) - mesh(0);
  tmp_diag = gamma(mesh(0)) * 0.5 * dx;
  triplets.push_back(Triplet(0, 0, tmp_diag));

  dx = mesh(M) - mesh(M - 1);
  tmp_diag = gamma(mesh(M)) * 0.5 * dx;
  triplets.push_back(Triplet(M, M, tmp_diag));

  // other diagonal entries
  for (unsigned i = 1; i < M; ++i) {
    dx = mesh(i + 1) - mesh(i - 1);
    tmp_diag = gamma(mesh(i)) * 0.5 * dx;
    triplets.push_back(Triplet(i, i, tmp_diag));
  }

  // as we are using the trapezoidal rule we do not get any off-diagonal entries

  // END-SOLUTION
  return triplets;
}
/* SAM_LISTING_END_2 */

// Calculate the rhs vector corresponding to the integral containg f*v using
// composite trapezoidal rule
/* SAM_LISTING_BEGIN_3 */
template <typename FUNCTOR1>
Vector rhs_f(const Vector &mesh, FUNCTOR1&& f) {
  double dx;
  unsigned M = mesh.size() - 1;
  Vector b = Vector::Zero(M + 1);
  // BEGIN-SOLUTION

  // first and last entry need to be calculated separately
  b(0) = f(mesh(0)) * (mesh(1) - mesh(0)) * 0.5;
  b(M) = f(mesh(M)) * (mesh(M) - mesh(M - 1)) * 0.5;

  // other entries
  for (unsigned i = 1; i < M; ++i) {
    dx = mesh(i + 1) - mesh(i - 1);
    b(i) = f(mesh(i)) * 0.5 * dx;
  }

  // END-SOLUTION
  return b;
}

// clang-format on

// Calculate the rhs vector corresponding to the integral containg just v
/* SAM_LISTING_BEGIN_4 */
Vector rhs_constant(const Vector &mesh) {
  double dx;
  unsigned M = mesh.size() - 1;
  Vector b = Vector::Zero(M + 1);
  // BEGIN-SOLUTION

  // first and last entry need to be calculated separately
  b(0) = (mesh(1) - mesh(0)) / 2.;
  b(M) = (mesh(M) - mesh(M - 1)) / 2.;
  // other entries
  for (unsigned i = 1; i < M; ++i) {
    dx = mesh(i+1) - mesh(i-1);
    b(i) = dx;
  }

  // END-SOLUTION
  return b;
}
/* SAM_LISTING_END_4 */

// Build and solve the LSE corresponding to (A)
/* SAM_LISTING_BEGIN_A */
template <typename FUNCTOR1, typename FUNCTOR2>
Vector solveA(const Vector &mesh, FUNCTOR1&& gamma,
                       FUNCTOR2&& f) {

  unsigned M = mesh.size() - 1;
  
  Eigen::VectorXd u(M+1);
  Vector b = Eigen::VectorXd::Zero(M+1);
  // Matrix corresponding to integral with alpha
  Eigen::SparseMatrix<double> A_alpha(M+1, M+1);
  // Matrix corresponding to integral with gamma
  Eigen::SparseMatrix<double> A_gamma(M+1, M+1);
  // complete Galerkin Matrix
  Eigen::SparseMatrix<double> A(M+1, M+1);
  
  std::vector<Triplet> triplets_mat_alpha;
  std::vector<Triplet> triplets_mat_gamma;

  /// STEP1: Build the Galerkin matrix A
  /* SOLUTION_BEGIN */
  
  auto alpha = [](double x) { return 1; };
  
  triplets_mat_alpha = mat_alpha(mesh, alpha);
  triplets_mat_gamma = mat_gamma(mesh, gamma);
  
  A_alpha.setFromTriplets(triplets_mat_alpha.begin(), triplets_mat_alpha.end());
  A_gamma.setFromTriplets(triplets_mat_gamma.begin(), triplets_mat_gamma.end());
  
  A = A_alpha + A_gamma;
  
  /* SOLUTION_END */

  /// STEP2: Build the RHS vector b
  /* SOLUTION_BEGIN */
   
  b = rhs_f(mesh, f);
  
  /* SOLUTION_END */

  /// STEP3: Enforce dirichlet boundary conditions
  /* SOLUTION_BEGIN */
  
  // u(0) = u(M) = 0 
  // Thus, we drop first row and column, and last row and column of A and b
  Eigen::SparseMatrix<double> A_tilde(M-2, M-2);
  A_tilde = A.block(1,1,M-1,M-1);
  
  Eigen::VectorXd b_tilde(M-2);
  b_tilde = b.segment(1,M-1);
  
  /* SOLUTION_END */

  /// STEP4: Solve the system Au = b
  /* SOLUTION_BEGIN */
  Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>, Eigen::Lower> solver;
  solver.compute(A_tilde);

  if (solver.info() != Eigen::Success) {
    throw std::runtime_error("Could not decompose the matrix");
  }
  
  u(0) = 0;
  u(M) = 0;
  u.segment(1,M-1) = solver.solve(b_tilde);
  
  /* SOlUTION_END */

  return u;
}
/* SAM_LISTING_END_A */

// Build an solve the LSE corresponding to (B)
/* SAM_LISTING_BEGIN_B */
template <typename FUNCTOR1, typename FUNCTOR2>
Eigen::VectorXd solveB(const Eigen::VectorXd &mesh, FUNCTOR1&& alpha, FUNCTOR2&& f,
                       double u0, double u1) {
  unsigned M = mesh.size() - 1;
  Eigen::VectorXd u(M + 1);

  double dx_left, dx_right;

  Eigen::VectorXd b = Eigen::VectorXd::Zero(M+1);
  Eigen::SparseMatrix<double> A(M+1, M+1);
  std::vector<Triplet> triplets;

  /// STEP1: Build the Galerkin matrix A
  /* SOLUTION_BEGIN */
  
  triplets = mat_alpha(mesh, alpha);
  A.setFromTriplets(triplets.begin(), triplets.end());
  
  /* SOLUTION_END */

  /// STEP2: Build the RHS vector b
  /* SOLUTION_BEGIN */
  
  b = rhs_f(mesh, f);
  
  /* SOLUTION_END */

  /// STEP3: Enforce dirichlet boundary conditions
  /* SOLUTION_BEGIN */
  
  // Again, we drop first row and column, and last row and column of A and b 
  Eigen::SparseMatrix<double> A_tilde(M-2, M-2);
  A_tilde = A.block(1,1,M-1,M-1);

  Eigen::VectorXd b_tilde(M-2);
  b_tilde = b.segment(1,M-1);
  
  // Change A and b to enforce non-homogeneous dirichlet boundary contions using
  // the offset function technique
  dx_left = mesh(1) - mesh(0);
  dx_right = mesh(M) - mesh(M - 1);
  b_tilde(0) += u0 * alpha((mesh(0) + mesh(1)) / 2.) / dx_left;
  b_tilde(M-2) += u1 * alpha((mesh(M - 1) + mesh(M)) / 2.) / dx_right;

  /* SOLUTION_END */

  /// STEP4: Solve the system Au = b
  /* SOLUTION_BEGIN */
  Eigen::SimplicialLDLT<SparseMatrix> solver;
  solver.compute(A_tilde);

  if (solver.info() != Eigen::Success) {
    throw std::runtime_error("Could not decompose the matrix");
  }

  u(0) = u0;
  u(M) = u1;
  u.segment(1, M-1) = solver.solve(b_tilde);
  
  /* SOLUTION_END */

  return u;
}
/* SAM_LISTING_END_B */

// Build an solve the LSE corresponding to (C)
/* SAM_LISTING_BEGIN_C */
template <typename FUNCTOR1, typename FUNCTOR2>
Vector solveC(const Vector &mesh, FUNCTOR1 alpha,
                       FUNCTOR2 gamma) {
  unsigned M = mesh.size() - 1;
  Vector u(M + 1);

  Vector b = Vector::Zero(M + 1);
  SparseMatrix A(M + 1, M + 1);
  std::vector<Triplet> triplets;
  triplets.reserve(2 * M + 1);

  /// STEP1: Build the Galerkin matrix A
  /* SOLUTION_BEGIN */

  // calculate the contributions from the summand with the alpha function
  double tmp_diag, tmp_offd;
  double dx_left, dx_right;

  // diagonal entries
  // first and last entry need to be calculated separately
  dx_right = mesh(1) - mesh(0);
  tmp_diag = alpha((mesh(1) + mesh(0)) / 2.) / dx_right;
  triplets.push_back(Triplet(0, 0, tmp_diag));

  dx_left = mesh(M) - mesh(M - 1);
  tmp_diag = alpha((mesh(M) + mesh(M - 1)) / 2.) / dx_left;
  triplets.push_back(Triplet(M, M, tmp_diag));

  // other diagonal entries
  for (unsigned i = 1; i < M; ++i) {
    dx_left = mesh(i) - mesh(i - 1);
    dx_right = mesh(i + 1) - mesh(i);
    tmp_diag = alpha((mesh(i - 1) + mesh(i)) / 2.) / dx_left +
               alpha((mesh(i) + mesh(i + 1)) / 2.) / dx_right;
    triplets.push_back(Triplet(i, i, tmp_diag));
  }

  // off-diagonal entries
  for (unsigned i = 0; i < M; ++i) {
    dx_right = mesh(i + 1) - mesh(i);
    tmp_offd = -alpha((mesh(i) + mesh(i + 1)) / 2.) / dx_right;
    triplets.push_back(Triplet(i + 1, i, tmp_offd));
    triplets.push_back(Triplet(i, i + 1, tmp_offd));
  }

  // calculate the contributions from the summand with the gamma function
  double dx;

  // diagonal entries
  // first and last entry need to be calculated separately
  dx = mesh(1) - mesh(0);
  tmp_diag = gamma(mesh(0)) * 0.5 * dx;
  triplets.push_back(Triplet(0, 0, tmp_diag));

  dx = mesh(M) - mesh(M - 1);
  tmp_diag = gamma(mesh(M)) * 0.5 * dx;
  triplets.push_back(Triplet(M, M, tmp_diag));

  // other diagonal entries
  for (unsigned i = 1; i < M; ++i) {
    dx = mesh(i + 1) - mesh(i - 1);
    tmp_diag = gamma(mesh(i)) * 0.5 * dx;
    triplets.push_back(Triplet(i, i, tmp_diag));
  }

  // as we are using the trapezoidal rule we do not get any off-diagonal entries

  A.setFromTriplets(triplets.begin(), triplets.end());
  /* SOLUTION_END */

  /// STEP2: Build the RHS vector b
  /* SOLUTION_BEGIN */

  // calculate the rhs entries resulting from the function f
  // first and last entry need to be calculated separately
  b(0) = (mesh(1) - mesh(0)) / 2.;
  b(M) = (mesh(M) - mesh(M - 1)) / 2.;
  // other entries
  for (unsigned i = 1; i < M; ++i) {
    dx_left = mesh(i) - mesh(i - 1);
    dx_right = mesh(i + 1) - mesh(i);
    b(i) = dx_left / 2. + dx_right / 2.;
  }

  // Since we have newmann boundary conditions we don't need to adapt the LSE to
  // enforce boundary conditions
  /* SOLUTION_END */

  /// STEP3: Solve the system Au = b
  /* SOLUTION_BEGIN */
  Eigen::SimplicialLDLT<SparseMatrix> solver;
  solver.compute(A);

  if (solver.info() != Eigen::Success) {
    throw std::runtime_error("Could not decompose the matrix");
  }

  u = solver.solve(b);

  /* SOLUTION_END */

  return u;
}
/* SAM_LISTING_END_C */

}  // namespace LinearFE1D
