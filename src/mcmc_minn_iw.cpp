#include "mfbvar.h"
// [[Rcpp::export]]
void mcmc_minn_iw(const arma::mat & y_in_p,
                  arma::cube& Pi, arma::cube& Sigma, arma::cube& Z, arma::cube& Z_fcst,
                  const arma::mat& Lambda_comp, const arma::mat& prior_Pi_Omega,
                  const arma::mat& inv_prior_Pi_Omega,
                  const arma::mat& Omega_Pi, const arma::mat& prior_Pi_mean,
                  const arma::mat & prior_S, const arma::mat& Z_1, arma::uword n_reps,
                  arma::uword n_q, arma::uword T_b, arma::uword n_lags, arma::uword n_vars,
                  arma::uword n_T, arma::uword n_fcst) {
  arma::mat Pi_i = Pi.slice(0);
  arma::mat Sigma_i = Sigma.slice(0);
  arma::mat y_i, X, XX, XX_inv, Pi_sample, post_Pi_Omega, post_Pi;
  arma::mat S, Pi_diff, post_S, Sigma_chol, x;
  arma::mat Z_i = arma::mat(n_lags + y_in_p.n_rows, n_vars, arma::fill::zeros);
  arma::mat Z_fcst_i = arma::mat(n_vars, n_lags + n_fcst);
  Z_i.rows(0, n_lags - 1) = Z_1;
  int post_nu = n_T + n_vars + 2;

  for (arma::uword i = 0; i < n_reps; ++i) {
    y_i = simsm_adaptive_cv(y_in_p, Pi_i, arma::chol(Sigma_i), Lambda_comp, Z_1, n_q, T_b);
    Z_i.rows(n_lags, n_T + n_lags - 1) = y_i;
    Z.slice(i) = Z_i;

    X = create_X(Z_i, n_lags);
    XX = X.t() * X;

    XX_inv = arma::inv_sympd(XX);
    Pi_sample = XX_inv * (X.t() * y_i);
    post_Pi_Omega = arma::inv_sympd(inv_prior_Pi_Omega + XX);
    post_Pi = post_Pi_Omega * (Omega_Pi + X.t() * y_i);
    S = arma::trans((y_i - X * Pi_sample)) * (y_i - X * Pi_sample);
    Pi_diff = prior_Pi_mean - Pi_sample;
    post_S = prior_S + S + Pi_diff.t() * arma::inv_sympd(prior_Pi_Omega + XX_inv) * Pi_diff;
    Sigma_i = rinvwish(post_nu, post_S);

    Sigma.slice(i) = Sigma_i;
    Pi_i = rmatn(post_Pi.t(), post_Pi_Omega, Sigma_i);

    Sigma_chol = arma::chol(Sigma_i, "lower");
    arma::vec errors = arma::vec(n_vars);

    if (n_fcst > 0) {
      Z_fcst_i.head_cols(n_lags) = Z_i.tail_rows(n_lags).t();
      for (arma::uword h = 0; h < n_fcst; ++h) {
        errors.imbue(norm_rand);
        x = create_X_t(Z_fcst_i.cols(0+h, n_lags-1+h).t());
        Z_fcst_i.col(n_lags + h) = Pi_i * x + Sigma_chol * errors;
      }
      Z_fcst.slice(i) = Z_fcst_i.t();
    }
  }

}

// [[Rcpp::export]]
void mcmc_minn_csv(const arma::mat & y_in_p,
                  arma::cube& Pi, arma::cube& Sigma, arma::cube& Z, arma::cube& Z_fcst,
                  arma::vec& phi, arma::vec& sigma, arma::mat& f,
                  const arma::mat& Lambda_comp, const arma::mat& prior_Pi_Omega,
                  const arma::mat& inv_prior_Pi_Omega,
                  const arma::mat& Omega_Pi, const arma::mat& prior_Pi_mean,
                  const arma::mat & prior_S, const arma::mat& Z_1, arma::uword n_reps,
                  arma::uword n_q, arma::uword T_b, arma::uword n_lags, arma::uword n_vars,
                  arma::uword n_T, arma::uword n_fcst) {
  arma::mat Pi_i = Pi.slice(0);
  arma::mat Sigma_i = Sigma.slice(0);
  arma::mat y_i, X, XX, XX_inv, Pi_sample, post_Pi_Omega, post_Pi;
  arma::mat S, Pi_diff, post_S, Sigma_chol, x, y_scaled, X_scaled;
  arma::vec f_i = f.row(0).t(), exp_f;
  arma::vec errors = arma::vec(n_vars);
  double phi_i = phi(0), sigma_i = sigma(0), vol_pred;
  arma::mat Z_i = arma::mat(n_lags + y_in_p.n_rows, n_vars, arma::fill::zeros);
  arma::mat Z_fcst_i = arma::mat(n_vars, n_lags + n_fcst);
  Z_i.rows(0, n_lags - 1) = Z_1;
  int post_nu = n_T + n_vars + 2;

  for (arma::uword i = 0; i < n_reps; ++i) {
    y_i = simsm_adaptive_cv(y_in_p, Pi_i, arma::chol(Sigma_i), Lambda_comp, Z_1, n_q, T_b);
    Z_i.rows(n_lags, n_T + n_lags - 1) = y_i;
    Z.slice(i) = Z_i;

    X = create_X(Z_i, n_lags);
    exp_f = arma::exp(0.5 * f_i);
    y_scaled = y_i;
    y_scaled.each_col() /= exp_f;
    X_scaled = X;
    X_scaled.each_col() /= exp_f;
    XX = X_scaled.t() * X_scaled;

    XX_inv = arma::inv_sympd(XX);
    Pi_sample = XX_inv * (X_scaled.t() * y_scaled);
    post_Pi_Omega = arma::inv_sympd(inv_prior_Pi_Omega + XX);
    post_Pi = post_Pi_Omega * (Omega_Pi + X_scaled.t() * y_scaled);
    S = arma::trans((y_scaled - X_scaled * Pi_sample)) * (y_scaled - X_scaled * Pi_sample);
    Pi_diff = prior_Pi_mean - Pi_sample;
    post_S = prior_S + S + Pi_diff.t() * arma::inv_sympd(prior_Pi_Omega + XX_inv) * Pi_diff;
    Sigma_i = rinvwish(post_nu, post_S);

    Sigma.slice(i) = Sigma_i;
    Sigma_chol = arma::chol(Sigma_i, "lower");
    // Sigma_chol_cube.each_slice() = Sigma_chol;
    // for (arma::uword j = 0; j < n_T; ++j) {
    //   Sigma_chol_cube.slice(j) %= exp_f;
    // }
    Pi_i = rmatn(post_Pi.t(), post_Pi_Omega, Sigma_i);

    // Sample factor and related parameters here

    vol_pred = f(n_T-1);
    if (n_fcst > 0) {
      Z_fcst_i.head_cols(n_lags) = Z_i.tail_rows(n_lags).t();
      for (arma::uword h = 0; h < n_fcst; ++h) {
        vol_pred = phi_i * vol_pred + R::rnorm(0.0, sigma_i);
        errors.imbue(norm_rand);
        errors = errors * std::exp(0.5 * vol_pred);
        x = create_X_t(Z_fcst_i.cols(0+h, n_lags-1+h).t());
        Z_fcst_i.col(n_lags + h) = Pi_i * x + Sigma_chol * errors;
      }
      Z_fcst.slice(i) = Z_fcst_i.t();
    }
  }

}
