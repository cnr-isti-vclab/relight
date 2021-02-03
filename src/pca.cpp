/**
 * @file pca.cpp
 * @brief Principal Component Analysis

This is the MIT license: http://www.opensource.org/licenses/mit-license.php

Copyright (C) 2012-2015 by Christian Blume.
libpca is a trademark of Christian Blume.

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

 */
#include "pca.h"
#include <stdexcept>
#include <random>

namespace stats {

pca::pca()
	: num_vars_(0),
	  num_records_(0),
	  record_buffer_(1000),
	  solver_("dc"),
	  do_normalize_(false),
	  do_bootstrap_(false),
	  num_bootstraps_(10),
	  bootstrap_seed_(1),
	  num_retained_(1),
	  data_(),
	  energy_(1),
	  energy_boot_(),
	  eigval_(),
	  eigval_boot_(),
	  eigvec_(),
	  proj_eigvec_(),
	  princomp_(),
	  mean_(),
	  sigma_()
{}

pca::pca(long num_vars)
	: num_vars_(num_vars),
	  num_records_(0),
	  record_buffer_(1000),
	  solver_("dc"),
	  do_normalize_(false),
	  do_bootstrap_(false),
	  num_bootstraps_(10),
	  bootstrap_seed_(1),
	  num_retained_(num_vars_),
	  data_(record_buffer_, num_vars_),
	  energy_(1),
	  energy_boot_(num_bootstraps_),
	  eigval_(num_vars_),
	  eigval_boot_(num_bootstraps_, num_vars_),
	  eigvec_(num_vars_, num_vars_),
	  proj_eigvec_(num_vars_, num_vars_),
	  princomp_(record_buffer_, num_vars_),
	  mean_(num_vars_),
	  sigma_(num_vars_)
{
	assert_num_vars_();
	initialize_();
}

pca::~pca()
{}

bool pca::operator==(const pca& other) {
	const double eps = 1e-5;
	if (num_vars_ == other.num_vars_ &&
		num_records_ == other.num_records_ &&
		record_buffer_ == other.record_buffer_ &&
		solver_ == other.solver_ &&
		do_normalize_ == other.do_normalize_ &&
		do_bootstrap_ == other.do_bootstrap_ &&
		num_bootstraps_ == other.num_bootstraps_ &&
		bootstrap_seed_ == other.bootstrap_seed_ &&
		num_retained_ == other.num_retained_ &&
		utils::is_approx_equal_container(eigval_, other.eigval_, eps) &&
		utils::is_approx_equal_container(eigvec_, other.eigvec_, eps) &&
		utils::is_approx_equal_container(princomp_, other.princomp_, eps) &&
		utils::is_approx_equal_container(energy_, other.energy_, eps) &&
		utils::is_approx_equal_container(mean_, other.mean_, eps) &&
		utils::is_approx_equal_container(sigma_, other.sigma_, eps) &&
		utils::is_approx_equal_container(eigval_boot_, other.eigval_boot_, eps) &&
		utils::is_approx_equal_container(energy_boot_, other.energy_boot_, eps) &&
		utils::is_approx_equal_container(proj_eigvec_, other.proj_eigvec_, eps))
		return true;
	else
		return false;
}

void pca::resize_data_if_needed_() {
	if (num_records_ == record_buffer_) {
		record_buffer_ += record_buffer_;
		data_.resize(record_buffer_, num_vars_);
	}
}

void pca::assert_num_vars_() {
	if (num_vars_ < 2)
		throw std::invalid_argument("Number of variables smaller than two.");
}

void pca::initialize_() {
	data_.zeros();
	eigval_.zeros();
	eigvec_.zeros();
	princomp_.zeros();
	mean_.zeros();
	sigma_.zeros();
	eigval_boot_.zeros();
	energy_boot_.zeros();
	energy_.zeros();
}

void pca::set_num_variables(long num_vars) {
	num_vars_ = num_vars;
	assert_num_vars_();
	num_retained_ = num_vars_;
	data_.resize(record_buffer_, num_vars_);
	eigval_.resize(num_vars_);
	eigvec_.resize(num_vars_, num_vars_);
	mean_.resize(num_vars_);
	sigma_.resize(num_vars_);
	eigval_boot_.resize(num_bootstraps_, num_vars_);
	energy_boot_.resize(num_bootstraps_);
	initialize_();
}

void pca::add_record(const std::vector<double>& record) {
	assert_num_vars_();

	if (num_vars_ != long(record.size()))
		throw std::domain_error(utils::join("Record has the wrong size: ", record.size()));

	resize_data_if_needed_();
	arma::Row<double> row(&record.front(), record.size());
	data_.row(num_records_) = std::move(row);
	++num_records_;
}

std::vector<double> pca::get_record(long record_index) const {
	return std::move(utils::extract_row_vector(data_, record_index));
}

void pca::set_do_normalize(bool do_normalize) {
	do_normalize_ = do_normalize;
}

void pca::set_do_bootstrap(bool do_bootstrap, long number, long seed) {
	if (number < 10)
		throw std::invalid_argument("Number of bootstraps smaller than ten.");

	do_bootstrap_ = do_bootstrap;
	num_bootstraps_ = number;
	bootstrap_seed_ = seed;

	eigval_boot_.resize(num_bootstraps_, num_vars_);
	energy_boot_.resize(num_bootstraps_);
}

void pca::set_solver(const std::string& solver) {
	if (solver!="standard" && solver!="dc")
		throw std::invalid_argument(utils::join("No such solver available: ", solver));
	solver_ = solver;
}

void pca::solve() {
	assert_num_vars_();

	if (num_records_ < 2)
		throw std::logic_error("Number of records smaller than two.");

	data_.resize(num_records_, num_vars_);

	mean_ = utils::compute_column_means(data_);
	utils::remove_column_means(data_, mean_);

	sigma_ = utils::compute_column_rms(data_);
	if (do_normalize_) utils::normalize_by_column(data_, sigma_);

	arma::Col<double> eigval(num_vars_);
	arma::Mat<double> eigvec(num_vars_, num_vars_);

	arma::Mat<double> cov_mat = utils::make_covariance_matrix(data_);
	arma::eig_sym(eigval, eigvec, cov_mat, solver_.c_str());
	arma::uvec indices = arma::sort_index(eigval, "descend");

	for (long i=0; i<num_vars_; ++i) {
		eigval_(i) = eigval(indices(i));
		eigvec_.col(i) = eigvec.col(indices(i));
	}

	utils::enforce_positive_sign_by_column(eigvec_);
	proj_eigvec_ = eigvec_;

	princomp_ = data_ * eigvec_;

	energy_(0) = arma::sum(eigval_);
	eigval_ *= 1./energy_(0);

	if (do_bootstrap_) bootstrap_eigenvalues_();
}

void pca::bootstrap_eigenvalues_() {
	std::srand(bootstrap_seed_);

	arma::Col<double> eigval(num_vars_);
	arma::Mat<double> dummy(num_vars_, num_vars_);

	for (long b=0; b<num_bootstraps_; ++b) {
		const arma::Mat<double> shuffle = utils::make_shuffled_matrix(data_);

		const arma::Mat<double> cov_mat = utils::make_covariance_matrix(shuffle);
		arma::eig_sym(eigval, dummy, cov_mat, solver_.c_str());
		eigval = arma::sort(eigval, "descend");

		energy_boot_(b) = arma::sum(eigval);
		eigval *= 1./energy_boot_(b);
		eigval_boot_.row(b) = eigval.t();
	}
}

void pca::set_num_retained(long num_retained) {
	if (num_retained<=0 || num_retained>num_vars_)
		throw std::range_error(utils::join("Value out of range: ", num_retained));

	num_retained_ = num_retained;
	proj_eigvec_ = eigvec_.submat(0, 0, eigvec_.n_rows-1, num_retained_-1);
}

std::vector<double> pca::to_principal_space(const std::vector<double>& data) const {
	arma::Col<double> column(&data.front(), data.size());
	column -= mean_;
	if (do_normalize_) column /= sigma_;
	const arma::Row<double> row(column.t() * proj_eigvec_);
	return std::move(utils::extract_row_vector(row, 0));
}

std::vector<double> pca::to_variable_space(const std::vector<double>& data) const {
	const arma::Row<double> row(&data.front(), data.size());
	arma::Col<double> column(arma::trans(row * proj_eigvec_.t()));
	if (do_normalize_) column %= sigma_;
	column += mean_;
	return std::move(utils::extract_column_vector(column, 0));
}

double pca::get_energy() const {
	return energy_(0);
}

std::vector<double> pca::get_energy_boot() const {
	return std::move(utils::extract_column_vector(energy_boot_, 0));
}

double pca::get_eigenvalue(long eigen_index) const {
	if (eigen_index >= num_vars_)
		throw std::range_error(utils::join("Index out of range: ", eigen_index));
	return eigval_(eigen_index);
}

std::vector<double> pca::get_eigenvalues() const {
	return std::move(utils::extract_column_vector(eigval_, 0));
}

std::vector<double> pca::get_eigenvalue_boot(long eigen_index) const {
	return std::move(utils::extract_column_vector(eigval_boot_, eigen_index));
}

std::vector<double> pca::get_eigenvector(long eigen_index) const {
	return std::move(utils::extract_column_vector(eigvec_, eigen_index));
}

std::vector<double> pca::get_principal(long eigen_index) const {
	return std::move(utils::extract_column_vector(princomp_, eigen_index));
}

double pca::check_eigenvectors_orthogonal() const {
	return std::abs(arma::det(eigvec_));
}

double pca::check_projection_accurate() const {
	if (data_.n_cols!=eigvec_.n_cols || data_.n_rows!=princomp_.n_rows)
		throw std::runtime_error("No proper data matrix present that the projection could be compared with.");
	const arma::Mat<double> diff = (princomp_ * arma::trans(eigvec_)) - data_;
	return 1 - arma::sum(arma::sum( arma::abs(diff) )) / diff.n_elem;
}

bool pca::get_do_normalize() const {
	return do_normalize_;
}

bool pca::get_do_bootstrap() const {
	return do_bootstrap_;
}

long pca::get_num_bootstraps() const {
	return num_bootstraps_;
}

long pca::get_bootstrap_seed() const {
	return bootstrap_seed_;
}

std::string pca::get_solver() const {
	return solver_;
}

std::vector<double> pca::get_mean_values() const {
	return std::move(utils::extract_column_vector(mean_, 0));
}

std::vector<double> pca::get_sigma_values() const {
	return std::move(utils::extract_column_vector(sigma_, 0));
}

long pca::get_num_variables() const {
	return num_vars_;
}

long pca::get_num_records() const {
	return num_records_;
}

long pca::get_num_retained() const {
	return num_retained_;
}

void pca::save(const std::string& basename) const {
	const std::string filename = basename + ".pca";
	std::ofstream file(filename.c_str());
	utils::assert_file_good(file.good(), filename);
	utils::write_property(file, "num_variables", num_vars_);
	utils::write_property(file, "num_records", num_records_);
	utils::write_property(file, "solver", solver_);
	utils::write_property(file, "num_retained", num_retained_);
	utils::write_property(file, "do_normalize", do_normalize_);
	utils::write_property(file, "do_bootstrap", do_bootstrap_);
	utils::write_property(file, "num_bootstraps", num_bootstraps_);
	utils::write_property(file, "bootstrap_seed", bootstrap_seed_);
	file.close();

	utils::write_matrix_object(basename + ".eigval", eigval_);
	utils::write_matrix_object(basename + ".eigvec", eigvec_);
	utils::write_matrix_object(basename + ".princomp", princomp_);
	utils::write_matrix_object(basename + ".energy", energy_);
	utils::write_matrix_object(basename + ".mean", mean_);
	utils::write_matrix_object(basename + ".sigma", sigma_);
	if (do_bootstrap_) {
		utils::write_matrix_object(basename + ".eigvalboot", eigval_boot_);
		utils::write_matrix_object(basename + ".energyboot", energy_boot_);
	}
}

void pca::load(const std::string& basename) {
	const std::string filename = basename + ".pca";
	std::ifstream file(filename.c_str());
	utils::assert_file_good(file.good(), filename);
	utils::read_property(file, "num_variables", num_vars_);
	utils::read_property(file, "num_records", num_records_);
	utils::read_property(file, "solver", solver_);
	utils::read_property(file, "num_retained", num_retained_);
	utils::read_property(file, "do_normalize", do_normalize_);
	utils::read_property(file, "do_bootstrap", do_bootstrap_);
	utils::read_property(file, "num_bootstraps", num_bootstraps_);
	utils::read_property(file, "bootstrap_seed", bootstrap_seed_);
	file.close();

	utils::read_matrix_object(basename + ".eigval", eigval_);
	utils::read_matrix_object(basename + ".eigvec", eigvec_);
	utils::read_matrix_object(basename + ".princomp", princomp_);
	utils::read_matrix_object(basename + ".energy", energy_);
	utils::read_matrix_object(basename + ".mean", mean_);
	utils::read_matrix_object(basename + ".sigma", sigma_);
	if (do_bootstrap_) {
		utils::read_matrix_object(basename + ".eigvalboot", eigval_boot_);
		utils::read_matrix_object(basename + ".energyboot", energy_boot_);
	}

	set_num_retained(num_retained_);
}

} // stats
