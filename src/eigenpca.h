#ifndef EIGENPCA_H
#define EIGENPCA_H

#include <Eigen/Eigenvalues>



class PCA {
public:
	PCA();
	PCA(int num_vars, int n_records) {
		resize(num_vars, n_records);
	}

	void resize(int num_vars, int n_records) {
		records.resize(n_records, num_vars);
	}


	void setRecord(int row, std::vector<double>& record) {
		 Eigen::Map<Eigen::RowVectorXd> v(record.data(), record.size());
		records.row(row) = v;
	}


	void solve(int n) {
		Eigen::MatrixXd cov = records.adjoint() * records;
		cov = cov / (records.rows() - 1);

		Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(cov);
		Eigen::VectorXd normalizedEigenValues =  eig.eigenvalues() / eig.eigenvalues().sum();

		Eigen::MatrixXd eigenVectors = eig.eigenvectors();
		transform = eigenVectors.rightCols(n).rowwise().reverse();
	}

	//TODO: is it bettrer or worse?
	void solveSVD(int n) {
		Eigen::JacobiSVD<Eigen::MatrixXd> svd(records, Eigen::ComputeThinV);

		// this is our basis
		transform = svd.matrixV().leftCols(n);
	}
	Eigen::MatrixXd &proj() {
		return transform;
	}

	Eigen::MatrixXd records;
	Eigen::MatrixXd transform;
};

#endif // EIGENPCA_H
