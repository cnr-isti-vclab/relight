#ifndef EIGENPCA_H
#define EIGENPCA_H

#include <Eigen/Eigenvalues>
#include <vector>



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
		// rankUpdate: half the work, and Eigen's OpenMP product is ~20x slower here (MSVC and GCC).
		// 16 blocks of rows in parallel, fixed count so the result does not depend on the number of cores.
		const int nblocks = 16;
		int rows = int(records.rows());
		std::vector<Eigen::MatrixXd> partial(nblocks, Eigen::MatrixXd::Zero(records.cols(), records.cols()));
		#pragma omp parallel for
		for(int b = 0; b < nblocks; b++) {
			int begin = int(int64_t(rows)*b/nblocks);
			int end = int(int64_t(rows)*(b + 1)/nblocks);
			partial[b].selfadjointView<Eigen::Lower>().rankUpdate(records.middleRows(begin, end - begin).adjoint());
		}
		Eigen::MatrixXd cov = partial[0];
		for(int b = 1; b < nblocks; b++)
			cov += partial[b];
		cov = cov.selfadjointView<Eigen::Lower>();
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
