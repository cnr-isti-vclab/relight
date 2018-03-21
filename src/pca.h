#pragma once
/**
 * @file pca.h
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
#include <vector>
#include <string>
#include <sstream>
#include <armadillo>
/**
 * @brief A namespace for statistical analysis
 */
namespace stats {
/**
 * @brief A class for principal component analysis
 */
class pca {
public:
	/**
	 * @brief Constructor
	 */
	pca();
	/**
	 * @brief Constructor
	 * @param num_vars Number of variables
	 * @throws std::invalid_argument if num_vars is smaller than two
	 */
	explicit pca(long num_vars);
	/**
	 * @brief Destructor
	 */
	virtual ~pca();
	/**
	 * @brief Comparison operator. Two pca instances are considered equal if
	 * 	all private members (minus the added data records) are equal relative
	 * 	to an epsilon of 1e-5
	 * @param other Another instance of pca
	 * @return Whether two pca instances can be considered equal
	 */
	bool operator==(const pca& other);
	/**
	 * @brief Sets the number of variables
	 * @param num_vars Number of variables
	 * @throws std::invalid_argument if num_vars is smaller than two
	 */
	void set_num_variables(long num_vars);
	/**
	 * @brief Returns the number of variables assigned to pca
	 * @return The number of variables
	 */
	long get_num_variables() const;
	/**
	 * @brief Adds a data record to pca
	 * @param record A vector with a size that equals the number
	 *  of variables assigned to pca
	 * @throws std::domain_error if record's size is not equal to the number of variables
	 */
	void add_record(const std::vector<double>& record);
	/**
	 * @brief Returns the previously added record with index record_index
	 * @param record_index The record index
	 * @return The record
	 */
	std::vector<double> get_record(long record_index) const;
	/**
	 * @brief Returns the number of records assigned to pca
	 * @return The number of records
	 */
	long get_num_records() const;
	/**
	 * @brief Sets whether to normalize each variable using the
	 *  temporal standard deviation prior to solving the eigenproblem
	 * @param do_normalize The boolean flag
	 */
	void set_do_normalize(bool do_normalize);
	/**
	 * @brief Returns whether the variables are normalized
	 * @return The boolean flag
	 */
	bool get_do_normalize() const;
	/**
	 * @brief Sets whether to bootstrap the eigenproblem after solving
	 *  the actual eigenproblem. Note that enabling bootstrapping
	 *  increases the computation time by an approximate factor of number
	 * @param do_bootstrap The boolean flag
	 * @param number Number of bootstraps
	 * @param seed The random seed used in the generation of the
	 *  random samples
	 * @throws std::invalid_argument if number is smaller than ten
	 */
	void set_do_bootstrap(bool do_bootstrap, long number=30, long seed=1);
	/**
	 * @brief Returns whether the eigenproblem is bootstrapped
	 * @return The boolean flag
	 */
	bool get_do_bootstrap() const;
	/**
	 * @brief Returns the number of bootstraps used in the optional bootstrapping
	 * @return The number of bootstraps
	 */
	long get_num_bootstraps() const;
	/**
	 * @brief Returns the random seed used in the optional bootstrapping
	 * @return The random seed
	 */
	long get_bootstrap_seed() const;
	/**
	 * @brief Sets the algorithmic solver to be used to solve the eigenproblem
	 * @param solver Available options: 'standard' and 'dc' where dc (divide
	 *  and conquer) is significantly faster but may result in slightly
	 *  different eigenvalues. Default is dc
	 * @throws std::invalid_argument if solver is not equal to 'standard' or 'dc'
	 */
	void set_solver(const std::string& solver);
	/**
	 * @brief Returns the solver to be used
	 * @return The solver
	 */
	std::string get_solver() const;
	/**
	 * @brief Solves the eigenproblem. Call this function after assigning
	 *  the data records. This function also performs mean centering, optional
	 *  normalization and optional bootstrapping
	 * @throws std::invalid_argument if the number of variables is smaller than two
	 * @throws std::logic_error if the number of previously assigned records is smaller than two
	 * @throws std::runtime_error if the variables are to be normalized and one of the variables has zero variance
	 */
	void solve();
	/**
	 * @brief Checks whether the eigenvectors are orthogonal. The closer
	 *  the return value to one the more orthogonal are the eigenvectors
	 * @return A scalar value
	 */
	double check_eigenvectors_orthogonal() const;
	/**
	 * @brief Checks whether the projection defined by the principal component analysis
	 *  is accurate. The closer the return value to one the more accurate is the projection
	 * @return A scalar value
	 */
	double check_projection_accurate() const;
	/**
	 * @brief Saves the resulting pca configuration, matrices and vectors to files
	 * @param basename The name that is used as a base for the different files
	 */
	void save(const std::string& basename) const;
	/**
	 * @brief Loads existing pca configuration, matrices and vectors
	 * @param basename The name that is used as a base for the different files
	 */
	void load(const std::string& basename);
	/**
	 * @brief Sets the number of retained eigenvectors. This affects the
	 *  projection from and to the space of principal components
	 * @param num_retained The number of retained eigenvectors
	 */
	void set_num_retained(long num_retained);
	/**
	 * @brief Returns the number of retained eigenvectors
	 * @return The number of retained eigenvectors
	 */
	long get_num_retained() const;
	/**
	 * @brief Projects a record in the variable space to a vector in the
	 *  space of principal components
	 * @param record A vector with a size that equals the number
	 *  of variables assigned to pca
	 * @return A vector with a size that equals the number
	 *  of variables assigned to pca
	 */
	std::vector<double> to_principal_space(const std::vector<double>& record) const;
	/**
	 * @brief Projects a vector in the space of principal components
	 *  to a record in the variable space
	 * @param data A vector with a size that equals the number
	 *  of variables assigned to pca
	 * @return A vector with a size that equals the number
	 *  of variables assigned to pca
	 */
	std::vector<double> to_variable_space(const std::vector<double>& data) const;
	/**
	 * @brief Returns the energy of the principal component analysis.
	 *  The energy is defined as the sum of the eigenvalues which equals
	 *  the trace of the covariance matrix
	 * @return The energy of the principal component analysis
	 */
	double get_energy() const;
	/**
	 * @brief Returns the vector of the energy bootstraps which is only
	 *  filled if the bootstrap flag is set to true. The vector's size
	 *  equals the number of bootstraps
	 * @return The vector of the energy bootstraps
	 */
	std::vector<double> get_energy_boot() const;
	/**
	 * @brief Returns the eigen_index'th eigenvalue starting at zero. Note that
	 *  the eigenvalues are normalized by their sum which equals the energy
	 *  of the eigenproblem
	 * @param eigen_index The index corresponding to the eigen_index'th
	 *  eigenvalue starting at zero
	 * @return An eigenvalue
	 * @throws std:range_error if eigen_index is out of range
	 */
	double get_eigenvalue(long eigen_index) const;
	/**
	 * @brief Returns the eigenvalues. Note that the eigenvalues are normalized
	 * 	by their sum which equals the energy of the eigenproblem
	 * @return The eigenvalues
	 */
	std::vector<double> get_eigenvalues() const;
	/**
	 * @brief Returns the vector of the eigenvalue bootstraps which is only
	 *  filled if the bootstrap flag is set to true. The vector's size
	 *  equals the number of bootstraps
	 * @param eigen_index The index corresponding to the eigen_index'th
	 *  eigenvalue starting at zero
	 * @return The vector of the eigenvalue bootstraps
	 * @throws std:range_error if eigen_index is out of range
	 */
	std::vector<double> get_eigenvalue_boot(long eigen_index) const;
	/**
	 * @brief Returns the eigen_index'th eigenvector starting at zero.
	 *  The vector's size equals the number of variables
	 * @param eigen_index The index corresponding to the eigen_index'th
	 *  eigenvalue starting at zero
	 * @return The eigenvector
	 * @throws std:range_error if eigen_index is out of range
	 */
	std::vector<double> get_eigenvector(long eigen_index) const;
	/**
	 * @brief Returns the eigen_index'th principal component starting at zero.
	 *  The vector's size equals the number of records
	 * @param eigen_index The index corresponding to the eigen_index'th
	 *  eigenvalue starting at zero
	 * @return The principal component
	 * @throws std:range_error if eigen_index is out of range
	 */
	std::vector<double> get_principal(long eigen_index) const;
	/**
	 * @brief Returns the mean values (average) of the records assigned to pca.
	 *  The vector's size equals the number of variables
	 * @return The mean values
	 * @throws std:range_error if eigen_index is out of range
	 */
	std::vector<double> get_mean_values() const;
	/**
	 * @brief Returns the sigma values (standard deviation) of the records assigned to pca.
	 *  The vector's size equals the number of variables
	 * @return The sigma values
	 */
	std::vector<double> get_sigma_values() const;

	arma::Col<double> &mean() { return mean_; }
	arma::Col<double> &eigval() { return eigval_; }
	arma::Mat<double> &eigvec() { return eigvec_; }
	arma::Mat<double> &proj() { return proj_eigvec_; }

protected:

	long num_vars_;
	long num_records_;
	long record_buffer_;
	std::string solver_;
	bool do_normalize_;
	bool do_bootstrap_;
	long num_bootstraps_;
	long bootstrap_seed_;
	long num_retained_;
	arma::Mat<double> data_;
	arma::Col<double> energy_;
	arma::Col<double> energy_boot_;
	arma::Col<double> eigval_;
	arma::Mat<double> eigval_boot_;
	arma::Mat<double> eigvec_;
	arma::Mat<double> proj_eigvec_;
	arma::Mat<double> princomp_;
	arma::Col<double> mean_;
	arma::Col<double> sigma_;
	void initialize_();
	void assert_num_vars_();
	void resize_data_if_needed_();
	void bootstrap_eigenvalues_();
};
/**
 * @brief Utilities
 */
namespace utils {
/**
 * @brief Computes the covariance matrix of its input matrix
 * @param data The input matrix
 * @return The covariance matrix
 */
arma::Mat<double> make_covariance_matrix(const arma::Mat<double>& data);
/**
 * @brief Computes a shuffled matrix from the input matrix. The resulting matrix
 * 	has the same dimensions as the input matrix. Shuffeling is done along
 * 	the rows and with replacement.
 * @param data The input matrix
 * @return The shuffled matrix
 */
arma::Mat<double> make_shuffled_matrix(const arma::Mat<double>& data);
/**
 * @brief Computes the column means of the input matrix
 * @param data The input matrix
 * @return The column means
 */
arma::Col<double> compute_column_means(const arma::Mat<double>& data);
/**
 * @brief Removes the column means from the input matrix
 * @param data The input matrix to be altered
 * @param means The column means
 * @throws std::range_error if number of columns of data is not equal to
 * 	number of elements of means
 */
void remove_column_means(arma::Mat<double>& data, const arma::Col<double>& means);
/**
 * @brief Computes the column root mean squared (rms) of the input matrix
 * @param data The input matrix
 * @return The column root mean squared
 */
arma::Col<double> compute_column_rms(const arma::Mat<double>& data);
/**
 * @brief Normalizes data by dividing each column with the corresponding
 * 	entry in sigmas
 * @param data The input matrix to be altered
 * @param rms The column vector used to normalize each column in data
 * @throws std::runtime_error if any of the entries in rms equals to zero
 * @throws std::range_error if number of columns of data is not equal to
 * 	number of elements of rms
 */
void normalize_by_column(arma::Mat<double>& data, const arma::Col<double>& rms);
/**
 * @brief Enforces a positive sign on the maximum value of each column and
 * 	then also scales the remaining values of each column
 * @param data The input matrix to be altered
 */
void enforce_positive_sign_by_column(arma::Mat<double>& data);
/**
 * @brief Extracts a column vector from the input matrix
 * @param data The input matrix
 * @param index The column index
 * @return The extracted column vector
 * @throws std:range_error if index is out of range
 */
std::vector<double> extract_column_vector(const arma::Mat<double>& data, long index);
/**
 * @brief Extracts a row vector from the input matrix
 * @param data The input matrix
 * @param index The row index
 * @return The extracted row vector
 * @throws std:range_error if index is out of range
 */
std::vector<double> extract_row_vector(const arma::Mat<double>& data, long index);
/**
 * @brief Asserts the boolean result of a file check
 * @param is_file_good The boolean result of a file check
 * @param filename The name of the file that was checked
 * @throws std::ios_base::failure if assertion failed
 */
void assert_file_good(const bool& is_file_good, const std::string& filename);
/**
 * @brief Write an Armadillo matrix to disk
 * @param filename The name of the file
 * @param matrix The Armadillo matrix
 * @throws std::ios_base::failure if cannot write to file
 */
template<typename T>
void write_matrix_object(const std::string& filename, const T& matrix) {
	assert_file_good(matrix.quiet_save(filename, arma::arma_ascii), filename);
}
/**
 * @brief Read an Armadillo matrix from disk
 * @param filename The name of the file
 * @param matrix The Armadillo matrix to be filled
 * @throws std::ios_base::failure if cannot read from file
 */
template<typename T>
void read_matrix_object(const std::string& filename, T& matrix) {
	assert_file_good(matrix.quiet_load(filename), filename);
}
/**
 * @brief Checks if two values are approx. equal relative to an epsilon
 * @param value1 Some scalar value
 * @param value2 Another scalar value
 * @param eps The epsilon
 * @returns Whether the two values are approx. equal
 */
template<typename T, typename U, typename V>
bool is_approx_equal(const T& value1, const U& value2, const V& eps) {
	return std::abs(value1-value2)<eps ? true : false;
}
/**
 * @brief Checks if two containers are approx. equal relative to an epsilon
 * @param container1 Some container
 * @param container2 Another container
 * @param eps The epsilon
 * @returns Whether the two containers are approx. equal
 */
template<typename T, typename U, typename V>
bool is_approx_equal_container(const T& container1, const U& container2, const V& eps) {
	if (container1.size()==container2.size()) {
		bool equal = true;
		for (size_t i=0; i<container1.size(); ++i) {
			equal = is_approx_equal(container1[i], container2[i], eps);
			if (!equal) break;
		}
		return equal;
	} else {
		return false;
	}
}
/**
 * @brief Computes the mean of a vector
 * @param iter The vector
 * @returns The mean
 */
double get_mean(const std::vector<double>& iter);
/**
 * @brief Computes the standard deviation of a vector
 * @param iter The vector
 * @returns The standard deviation
 */
double get_sigma(const std::vector<double>& iter);
/**
 * @brief A helper class for the join function
 */
struct join_helper {
	static void add_to_stream(std::ostream&) {}

	template<typename T, typename... Args>
	static void add_to_stream(std::ostream& stream, const T& arg, const Args&... args) {
		stream << arg;
		add_to_stream(stream, args...);
	}
};
/**
 * @brief Joins an arbitrary number of input arguments to a single string
 * @param arg An argument
 * @param args An arbitrary number of arguments (can be omitted)
 * @returns The joined string
 */
template<typename T, typename... Args>
std::string join(const T& arg, const Args&... args) {
	std::ostringstream stream;
	stream << arg;
	join_helper::add_to_stream(stream, args...);
	return stream.str();
}
/**
 * @brief Writes a property consisting of key and value to file
 * @param file The file
 * @param key The key
 * @param value The value
 */
template<typename T>
void write_property(std::ostream& file, const std::string& key, const T& value) {
	file << key << "\t" << value << std::endl;
}
/**
 * @brief Reads a property consisting of key and value from file
 * @param file The file
 * @param key The key
 * @param value The value
 * @throws std::domain_error if key not found
 */
template<typename T>
void read_property(std::istream& file, const std::string& key, T& value) {
	std::string tmp;
	bool found = false;
	while (file.good()) {
		file >> tmp;
		if (tmp==key) {
			file >> value;
			found = true;
			break;
		}
	}
	if (!found)
		throw std::domain_error(join("No such key available: ", key));
	file.seekg(0);
}

} //utils
} //stats
