#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/SparseCore>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/IterativeLinearSolvers>

#include <QFile>
#include <QTextStream>
#include "bni_normal_integration.h"
#include <iostream>

typedef Eigen::Triplet<double> Triple;

using namespace std;

double sigmoid(const double x, const double k = 1.0) {
	return 1.0 / (1.0 + exp(-x*k));
}

bool saveDepthMap(const QString &filename, int w, int h, std::vector<float> &z) {
	return false;
}

bool savePly(const QString &filename, int w, int h, std::vector<double> &z) {
	QFile file(filename);
	bool success = file.open(QFile::WriteOnly);
	if(!success)
		return false;
	{
		QTextStream stream(&file);

		stream << "ply\n";
		stream << "format binary_little_endian 1.0\n";
		stream << "element vertex " << w*h << "\n";
		stream << "property float x\n";
		stream << "property float y\n";
		stream << "property float z\n";
		stream << "element face " << 2*(w-1)*(h-1) << "\n";
		stream << "property list uchar int vertex_index\n";
		stream << "end_header\n";
	}

	std::vector<float> vertices(w*h*3);
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int pos = x + y*w;
			float *start = &vertices[3*pos];
			start[0] = x;
			start[1] = y;
			start[2] = z[pos];
		}
	}
	std::vector<uint8_t> indices(13*2*(w-1)*(h-1));
	for(int y = 0; y < h-1; y++) {
		for(int x = 0; x < w-1; x++) {
			int pos = x + y*w;
			uint8_t *start = &indices[26*(x + y*(w-1))];
			start[0] = 3;
			int *face = (int *)(start + 1);
			assert(pos+w+1 < w*h);
			face[0] = pos;
			face[1] = pos+w;
			face[2] = pos+w+1;

			start += 13;
			start[0] = 3;
			face = (int *)(start + 1);
			face[0] = pos;
			face[1] = pos+w+1;
			face[2] = pos+1;
		}
	}
	file.write((const char *)vertices.data(), vertices.size()*4);
	file.write((const char *)indices.data(), indices.size());
	file.close();
	return true;
}

std::vector<double> bni_integrate(std::function<bool(std::string s, int n)> progressed, int w, int h, std::vector<float> &normalmap,
								  double k,
								  double tolerance,
								  double solver_tolerance,
								  int max_iterations,
								  int max_solver_iterations) {

	cout << "Eigen using nthreads: " << Eigen::nbThreads( ) << endl;


	int n = w*h;
	Eigen::VectorXd nx(n);
	Eigen::VectorXd ny(n);
	Eigen::MatrixXd nz(h, w);

	for(int y = 0; y < h; y++) {
		for(int x= 0; x < w; x++) {
			int pos = x + y*w;
			nx(pos) = normalmap[pos*3+1];
			ny(pos) = normalmap[pos*3+0];
			nz(y, x) = -normalmap[pos*3+2];
		}
	}
	//BUILD A maps which encodes half derivatives (positive, negative, dx and dy)

	vector<Triple> triples;
	int offset =0;

	for(int y = 1; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int pos = x + y*w;
			triples.push_back(Triple(offset + pos, pos, -nz(y, x)));
			triples.push_back(Triple(offset + pos, pos-w, nz(y, x)));
		}
	}
	offset += n;

	for(int y = 0; y < h-1; y++) {
		for(int x = 0; x < w; x++) {
			int pos = x + y*w;
			triples.push_back(Triple(offset + pos, pos, nz(y, x)));
			triples.push_back(Triple(offset + pos, pos+w, -nz(y, x)));

		}
	}
	offset += n;

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w-1; x++) {
			int pos = x + y*w;
			triples.push_back(Triple(offset + pos, pos, -nz(y, x)));
			triples.push_back(Triple(offset + pos, pos+1, nz(y, x)));
		}
	}
	offset += n;

	for(int y = 0; y < h; y++) {
		for(int x = 1; x < w; x++) {
			int pos = x + y*w;
			triples.push_back(Triple(offset + pos, pos, nz(y, x)));
			triples.push_back(Triple(offset + pos, pos-1, -nz(y, x)));

		}
	}
	offset += n;

	Eigen::SparseMatrix<double> A(n*4, n);
	A.setFromTriplets(triples.begin(), triples.end());



	Eigen::VectorXd b(n*4);
	b << -nx, -nx, -ny, -ny;

	Eigen::SparseMatrix<double> W(n*4, n*4);
	triples.clear();
	for(int i = 0; i < n*4; i++)
		triples.push_back(Triple(i, i, 0.5));
	W.setFromTriplets(triples.begin(), triples.end());

	Eigen::VectorXd z = Eigen::VectorXd::Zero(n);

	Eigen::MatrixXd tmp = A*z - b;
	double energy = (tmp.transpose() * W * tmp)(0);
	double start_energy = energy;
	if(isnan(energy)) {
		return vector<double>();
	}

	cout << "Energy : " << energy << endl;

	Eigen::VectorXd Z;
	for(int i = 0; i < max_iterations; i++) {

		Eigen::SparseMatrix<double> A_mat = A.transpose()*W*A;
		Eigen::VectorXd b_vec = A.transpose()*W*b;

		Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> solver;
		solver.compute(A_mat);
		solver.setTolerance(solver_tolerance);
		solver.setMaxIterations(max_solver_iterations);

		z = solver.solveWithGuess(b_vec, z);
		if (solver.info() != Eigen::Success) {
			double finalResidual = solver.error();
			cerr << "Max iter reached with error: " << finalResidual << endl;
//			return std::vector<double>();
		}

		// Get the number of iterations
		int numIterations = solver.iterations();
		std::cout << "Number of iterations: " << numIterations << std::endl;


		if(k == 0)
			break;

		Eigen::VectorXd wu = ((A.block(n, 0, n, n)*z).array().pow(2) -
							  (A.block(0, 0, n, n)*z).array().pow(2)).unaryExpr([k](double x) { return sigmoid(x, k); });
		Eigen::VectorXd wv = ((A.block(n*3, 0, n, n)*z).array().pow(2) -
							  (A.block(n*2, 0, n, n)*z).array().pow(2)).unaryExpr([k](double x) { return sigmoid(x, k); });

		triples.clear();
		for(int i = 0; i < n; i++) {
			triples.push_back(Triple(i, i, wu(i)));
			triples.push_back(Triple(i+n, i+n, 1.0 - wu(i)));
			triples.push_back(Triple(i+n*2, i+n*2, wv(i)));
			triples.push_back(Triple(i+n*3, i+n*3, 1 - wv(i)));
		}
		W.setFromTriplets(triples.begin(), triples.end());

		double energy_old = energy;
		tmp = A*z - b;
		energy = (tmp.transpose() * W * tmp)(0);
		cout << "Energy: " << energy << endl;

		double relative_energy = fabs(energy - energy_old) / energy_old;
		double total_progress = fabs(energy - start_energy) / start_energy;
		if(progressed)
			progressed("Integrating normals...", 100*(log(relative_energy) - log(tolerance))/(log(total_progress) - log(tolerance)));
		if(relative_energy < tolerance)
			break;
	}
	std::vector<double> depthmap(n);
	memcpy(&depthmap[0], z.data(), n*8);
	return depthmap;
}
