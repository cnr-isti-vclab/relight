#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/SparseCore>
#include <Eigen/IterativeLinearSolvers>

#include <QFile>
#include <QTextStream>
#include <QImage>
#include "bni_normal_integration.h"
#include <iostream>

#include <tiffio.h>
#include "../relight_threadpool.h"
#include <thread>
#include <mutex>
#include <algorithm>
#include <limits>

/* TODO: try this lib:
https://amgcl.readthedocs.io/en/latest/tutorial/poisson3Db.html
*/

using namespace std;

using ProgressCallback = std::function<bool(QString s, int n)>;
using Triple = Eigen::Triplet<double>;
using RowMatrixXd = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

static Eigen::SparseMatrix<double> BuildDerivativeMatrix(int w, int h, std::function<double(int,int)> nzAt);

bool IntegrateNormalsWithScaffold(
	const RowMatrixXd &global_nx,
	const RowMatrixXd &global_ny,
	const RowMatrixXd &global_nz,
	const RowMatrixXd &upsampled_scaffold, // Pre-calculated, matches global dimensions
	RowMatrixXd &global_depth,
	ProgressCallback progressed);

double sigmoid(const double x, const double k = 1.0) {
	return 1.0 / (1.0 + exp(-x*k));
}

bool saveDepthMap(const QString &filename, size_t w, size_t h, std::vector<float> &z) {
	return false;
}

bool saveNormalMap(const QString &filename, size_t w, size_t h, std::vector<Eigen::Vector3f> &normals) {
	QImage img(w, h, QImage::Format_ARGB32);
	for(size_t y = 0; y < h; y++) {
		uint8_t *line = img.scanLine(y);
		for(size_t x = 0; x < w; x++) {
			auto &n = normals[x + y*w];
			line[4*x + 0] = 255;
			line[4*x + 1] = floor(n[0]*255.0f);
			line[4*x + 2] = floor(n[1]*255.0f);
			line[4*x + 3] = floor(n[2]*255.0f);
		}
	}
	img.save(filename);
	return true;
}

bool savePly(const QString &filename, size_t w, size_t h, std::vector<float> &z, float downsampling, float pixel_size) {
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
		stream << "property float s\n";
		stream << "property float t\n";
		stream << "element face " << 2*(w-1)*(h-1) << "\n";
		stream << "property list uchar int vertex_index\n";
		stream << "end_header\n";
	}

	float scale = (pixel_size > 0) ? downsampling * pixel_size : downsampling;
	float cx = (w - 1) * 0.5f;
	float cy = (h - 1) * 0.5f;

	std::vector<float> vertices(w*h*5);
	for(size_t y = 0; y < h; y++) {
		for(size_t x = 0; x < w; x++) {
			size_t pos = x + y*w;
			float *start = &vertices[5*pos];
			float mesh_y = float(h - 1 - y);
			start[0] = (float(x) - cx) * scale;
			start[1] = (mesh_y - cy) * scale;
			start[2] = -z[pos] * scale;
			start[3] = float(x) / float(w - 1);  // s
			start[4] = mesh_y / float(h - 1);    // t
			assert(!isnan(start[2]));
		}
	}
	std::vector<uint8_t> indices(13*2*(w-1)*(h-1));
	for(size_t y = 0; y < h-1; y++) {
		for(size_t x = 0; x < w-1; x++) {
			size_t pos = x + y*w;
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

bool saveTiff(const QString &filename, size_t w, size_t h, std::vector<float> &depthmap, bool normalize, float pixel_size) {
	float min = 1e20;
	float max = -1e20;
	for(float h: depthmap) {
		min = std::min(h, min);
		max = std::max(h, max);
	}

	uint32_t tileWidth = 256;
	uint32_t tileLength = 256;

	TIFF* outTiff = TIFFOpen(filename.toStdString().c_str(), "w");
	if (!outTiff) {
		return false;
	}

	TIFFSetField(outTiff, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField(outTiff, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(outTiff, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(outTiff, TIFFTAG_BITSPERSAMPLE, 32);
	TIFFSetField(outTiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	TIFFSetField(outTiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(outTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK); // Grayscale
	TIFFSetField(outTiff, TIFFTAG_TILEWIDTH, tileWidth);
	TIFFSetField(outTiff, TIFFTAG_TILELENGTH, tileLength);
	TIFFSetField(outTiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE); // No compression

	if(pixel_size > 0) {
		float pixelsPerCm = 10.0f / pixel_size;
		TIFFSetField(outTiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
		TIFFSetField(outTiff, TIFFTAG_XRESOLUTION, pixelsPerCm);
		TIFFSetField(outTiff, TIFFTAG_YRESOLUTION, pixelsPerCm);
	}

	uint32_t numTilesX = (w + tileWidth - 1) / tileWidth;
	uint32_t numTilesY = (h + tileLength - 1) / tileLength;



	for (uint32_t ty = 0; ty < numTilesY; ty++) {
		for (uint32_t tx = 0; tx < numTilesX; tx++) {
			uint32_t tileIndex = TIFFComputeTile(outTiff, tx * tileWidth, ty * tileLength, 0, 0);

			std::vector<float> data(tileWidth * tileLength, 0.0f);

			for(uint32_t dy = 0; dy < tileLength; dy++) {
				for(uint32_t dx = 0; dx < tileWidth; dx++) {
					size_t x = tx*tileWidth + dx;
					size_t y = ty*tileLength + dy;
					if(x < w && y < h) {
						float d = depthmap[x + y*w];
						if(normalize) {
							d = (d - min)/(max - min);
						}
						data[dx + dy*tileWidth] = d;
					}
				}
			}
			if (TIFFWriteEncodedTile(outTiff, tileIndex, data.data(), data.size() * sizeof(float)) < 0) {
				TIFFClose(outTiff);
				return false;
			}
		}
	}

	TIFFClose(outTiff);
	if(normalize) {
		//if normalize: save range in mm and pixel size (in mm) in a text file.
		QFile file(filename.left(filename.lastIndexOf('.')) + ".txt");
		file.open(QFile::WriteOnly);
		QTextStream stream(&file);
		stream << "min: " << min << "\n";
		stream << "max: " << max << "\n";
		stream << "range: " << max - min << "\n";
	}
	return true;
}
/* old code
template <class T>
void bilinear_interpolation(T *data, uint32_t input_width,
							uint32_t input_height, uint32_t output_width,
							uint32_t output_height, T *output) {
	float x_ratio, y_ratio;

	if (output_width > 1) {
		x_ratio = ((float)input_width - 1.0) / ((float)output_width - 1.0);
	} else {
		x_ratio = 0;
	}

	if (output_height > 1) {
		y_ratio = ((float)input_height - 1.0) / ((float)output_height - 1.0);
	} else {
		y_ratio = 0;
	}

	for (int i = 0; i < output_height; i++) {
		for (int j = 0; j < output_width; j++) {
			float x_l = floor(x_ratio * (float)j);
			float y_l = floor(y_ratio * (float)i);
			float x_h = std::min(input_width-1.0f, ceil(x_ratio * (float)j));
			float y_h = std::min(input_height-1.0f, ceil(y_ratio * (float)i));

			float x_weight = (x_ratio * (float)j) - x_l;
			float y_weight = (y_ratio * (float)i) - y_l;

			T a = data[(int)y_l * input_width + (int)x_l];
			T b = data[(int)y_l * input_width + (int)x_h];
			T c = data[(int)y_h * input_width + (int)x_l];
			T d = data[(int)y_h * input_width + (int)x_h];

			T pixel = a * (1.0 - x_weight) * (1.0 - y_weight) +
					b * x_weight * (1.0 - y_weight) +
					c * y_weight * (1.0 - x_weight) +
					d * x_weight * y_weight;

			output[i * output_width + j] = pixel;
		}
	}
} */

template <class T>
void bilinear_interpolation(T *data, uint32_t input_width,
							uint32_t input_height, uint32_t output_width,
							uint32_t output_height, T *output) {
	float x_ratio = (output_width > 1)  ? ((float)input_width - 1.0f) / ((float)output_width - 1.0f) : 0.0f;
	float y_ratio = (output_height > 1) ? ((float)input_height - 1.0f) / ((float)output_height - 1.0f) : 0.0f;

	for (uint32_t i = 0; i < output_height; i++) {
		float y_pos = y_ratio * (float)i;
		int y_l = (int)y_pos;
		int y_h = std::min(y_l + 1, (int)input_height - 1);
		float y_weight = y_pos - (float)y_l;

		// Edge case safety check for the absolute bottom edge pixel row
		if (y_l >= (int)input_height - 1) {
			y_l = input_height - 1;
			y_h = input_height - 1;
			y_weight = 0.0f;
		}

		for (uint32_t j = 0; j < output_width; j++) {
			float x_pos = x_ratio * (float)j;
			int x_l = (int)x_pos;
			int x_h = std::min(x_l + 1, (int)input_width - 1);
			float x_weight = x_pos - (float)x_l;

			// Edge case safety check for the absolute right edge pixel column
			if (x_l >= (int)input_width - 1) {
				x_l = input_width - 1;
				x_h = input_width - 1;
				x_weight = 0.0f;
			}

			T a = data[y_l * input_width + x_l];
			T b = data[y_l * input_width + x_h];
			T c = data[y_h * input_width + x_l];
			T d = data[y_h * input_width + x_h];

			T pixel = a * (1.0f - x_weight) * (1.0f - y_weight) +
					  b * x_weight * (1.0f - y_weight) +
					  c * y_weight * (1.0f - x_weight) +
					  d * x_weight * y_weight;

			output[i * output_width + j] = pixel;
		}
	}
}

void bilinear_interpolation3f(Eigen::Vector3f *data, uint32_t input_width,
							  uint32_t input_height, uint32_t output_width,
							  uint32_t output_height, Eigen::Vector3f *output) {
	return bilinear_interpolation<Eigen::Vector3f>(data, input_width, input_height,
												   output_width, output_height, output);
}


class NormalMap {
public:
	vector<Eigen::Vector3f> normals;
	vector<float> heights;
	int w;
	int h;

	NormalMap up() { //halve resolution
		NormalMap scaled;
		scaled.w = w/2;
		scaled.h = h/2;

		scaled.normals.resize(scaled.w*scaled.h);
		for(int y = 0; y < scaled.h; y++) {
			for(int x = 0; x < scaled.w; x++) {
				Eigen::Vector3f p(0,0,0);
				for(int k = 0; k < 3; k++) {
					p[k] = (normals[(2*x + 2*y*h)][k] +
							normals[(2*x + 1 + 2*y*h)][k] +
							normals[(2*x + (2*y + 1)*h)][k] +
							normals[(2*x + 1 + (2*y +1)*h)][k])/4.0f;
				}
				//normalize:
				float length = sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
				for(int k = 0; k < 3; k++)
					p[k] /= length;
				scaled.normals[x + y*scaled.w] = p;

			}
		}
		return scaled;
	}

	void pull(NormalMap &small) { //update normals
		heights.resize(w*h, 0);
		bilinear_interpolation(small.heights.data(), small.w, small.h, w, h, heights.data());
	}
};

std::vector<float> bni_pyramid(std::function<bool(QString s, int n)> progressed, int &w, int &h, std::vector<Eigen::Vector3f> &normalmap,
							   double k,
							   double tolerance,
							   double solver_tolerance,
							   int max_iterations,
							   int max_solver_iterations,
							   int scale) {
	vector<NormalMap> pyramid;
	vector<int> widths;
	vector<int> heights;

	NormalMap m;
	m.w = w;
	m.h = h;
	m.normals = normalmap;
	pyramid.push_back(m);

	int min_size = 32;
	while(pyramid.back().w > min_size && pyramid.back().h > min_size) {
		pyramid.push_back(pyramid.back().up());
	}
	NormalMap &top = pyramid.back();
	top.heights.resize(top.w*top.h, 0.0f);

	cout << "Scale: " << scale << endl;
	for(int i = pyramid.size()-1; i >= scale; i--) {
		cout << "Level: " << i << endl;
		NormalMap &p = pyramid[i];
		saveNormalMap("testN_" + QString::number(i) + ".png", p.w, p.h, p.normals);
		if(i + 1 < pyramid.size())
			p.pull(pyramid[i+1]);
		bni_integrate(progressed, p.w, p.h, p.normals, p.heights, k, tolerance, solver_tolerance, max_iterations, max_solver_iterations);
		savePly("test_" + QString::number(i) + ".ply", p.w, p.h, p.heights);

	}
	NormalMap &result = pyramid[scale];
	w = result.w;
	h = result.h;
	return result.heights;
}
#include <Eigen/SparseCholesky>


Eigen::VectorXd bni_integrate_iterative(std::function<bool(QString s, int n)> progressed, Eigen::SparseMatrix<double> &A, Eigen::VectorXd &b, Eigen::SparseMatrix<double> W,
										double k,
										double tolerance, double solver_tolerance,
										int max_iterations, int max_solver_iterations) {

	int n = b.size()/4;

	Eigen::VectorXd z = Eigen::VectorXd::Zero(n);

	Eigen::MatrixXd tmp = A*z - b;
	double energy = (tmp.transpose() * W * tmp)(0);
	double start_energy = energy;
	if(isnan(energy)) {
		throw "Computational problems.";
	}

	cout << "Energy : " << energy << endl;

	vector<Triple> triples;
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
		}

		// Get the number of iterations
		int numIterations = solver.iterations();
		std::cout << "Number of iterations: " << numIterations << " error: " << solver.error() << " tolerance: " << solver_tolerance << std::endl;


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
		if(progressed) {
			bool proceed = progressed("Integrating normals...", 100*(1 - (log(relative_energy) - log(tolerance))/(log(total_progress) - log(tolerance))));
			if(!proceed) break;
		}
		if(relative_energy < tolerance)
			break;
	}
	return z;
}


bool bni_integrate_direct(std::function<bool(QString s, int n)> progressed, Eigen::SparseMatrix<double> &A, Eigen::VectorXd &b, Eigen::VectorXd &z) {
	Eigen::SparseMatrix<double> A_mat = A.transpose() * A;
	Eigen::VectorXd b_vec = A.transpose() * b;

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
	solver.compute(A_mat);
	z = solver.solve(b_vec);
	return true;
}


bool bni_integrate(std::function<bool(QString s, int n)> progressed, int w, int h, std::vector<Eigen::Vector3f> &normalmap, std::vector<float> &heights,
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
			nx(pos) = normalmap[pos][1];
			ny(pos) = normalmap[pos][0];
			nz(y, x) = -normalmap[pos][2];
		}
	}

	/* If the image is large, compute a reduced "scaffold" depth map and
	   use the scaffold-aware tiled integrator. For small images, fall
	// through to the original direct-integrator path below which builds
	// the full A/b system and solves it.
	*/
	if (w >= 512 || h >= 512) {
		int maxDim = std::max(w, h);
		int scale = std::max(1, maxDim / 512);
		int sw = std::max(1, w / scale);
		int sh = std::max(1, h / scale);

		// Downsample normals into a small scaffold normal map
		std::vector<Eigen::Vector3f> smallNormals(sw * sh);
		bilinear_interpolation3f(normalmap.data(), w, h, sw, sh, smallNormals.data());

		// Build local A/b for the small problem and solve directly to get coarse depths
		int n_small = sw * sh;
		Eigen::VectorXd nx_s(n_small);
		Eigen::VectorXd ny_s(n_small);
		Eigen::MatrixXd nz_s(sh, sw);
		for (int y = 0; y < sh; ++y) {
			for (int x = 0; x < sw; ++x) {
				int pos = x + y * sw;
				nx_s(pos) = smallNormals[pos][1];
				ny_s(pos) = smallNormals[pos][0];
				nz_s(y, x) = -smallNormals[pos][2];
			}
		}

		Eigen::SparseMatrix<double> A_small = BuildDerivativeMatrix(sw, sh, [&](int yy, int xx){ return nz_s(yy, xx); });

		Eigen::VectorXd b_small(n_small * 4);
		b_small << -nx_s, -nx_s, -ny_s, -ny_s;



		Eigen::VectorXd z_small;
		bool proceed = bni_integrate_direct(progressed, A_small, b_small, z_small);
		if (!proceed) return false;

		

		// Upsample coarse depth to full resolution to create the scaffold
		std::vector<float> smallDepth(n_small);
		for (int i = 0; i < n_small; ++i) smallDepth[i] = static_cast<float>(z_small(i));
		std::vector<float> upDepth(w * h);
		bilinear_interpolation<float>(smallDepth.data(), sw, sh, w, h, upDepth.data());

		// Scale depths to account for the coarser grid spacing used when solving
		// on the downsampled scaffold. Each coarse pixel represents 'scale'
		// fine pixels, so multiply depth values by that factor.
		for (size_t i = 0; i < upDepth.size(); ++i)
			upDepth[i] *= static_cast<float>(scale);

		
		RowMatrixXd upsampled_scaffold(h, w);
		for (int y = 0; y < h; ++y)
			for (int x = 0; x < w; ++x)
				upsampled_scaffold(y, x) = upDepth[x + y * w];
		
		// Prepare full-size normal matrices for the scaffold-aware integrator
		RowMatrixXd global_nx(h, w), global_ny(h, w), global_nz(h, w);
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				int pos = x + y * w;
				global_nx(y, x) = nx(pos);
				global_ny(y, x) = ny(pos);
				global_nz(y, x) = nz(y, x);
			}
		}

		RowMatrixXd depth;
		if (!IntegrateNormalsWithScaffold(global_nx, global_ny, global_nz, upsampled_scaffold, depth, progressed))
			return false;

		heights.resize(w * h);
		for (int y = 0; y < h; ++y)
			for (int x = 0; x < w; ++x)
				heights[x + y * w] = depth(y, x);

		return true;
	}

	// Build derivative-only system matrix A
	Eigen::SparseMatrix<double> A = BuildDerivativeMatrix(w, h, [&](int yy, int xx){ return nz(yy, xx); });

	Eigen::VectorXd b(n*4);
	b << -nx, -nx, -ny, -ny;

	Eigen::SparseMatrix<double> W(n*4, n*4);
	std::vector<Triple> triples;
	for(int i = 0; i < n*4; i++)
		triples.push_back(Triple(i, i, 0.5));
	W.setFromTriplets(triples.begin(), triples.end());

	Eigen::VectorXd z;

	if(k == 0.0) {
		bool proceed = bni_integrate_direct(progressed, A, b, z);
		if(!proceed)
			return false;
	} else
		z = bni_integrate_iterative(progressed, A, b, W, k, tolerance, solver_tolerance, max_iterations, max_solver_iterations);

	heights.resize(w*h);
	for(int i = 0; i < w*h; i++)
		heights[i] = z(i);
	return true;
}


// Helper: append derivative triples (4 blocks) using a value accessor nzAt(y,x).
static void FillDerivativeTriples(std::vector<Triple> &triples, int w, int h, std::function<double(int,int)> nzAt, int offset = 0) {
	// Block 1: Backward Y-difference
	for (int y = 1; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int pos = x + y * w;
			double v = nzAt(y, x);
			triples.push_back(Triple(offset + pos, pos, -v));
			triples.push_back(Triple(offset + pos, pos - w, v));
		}
	}
	offset += w * h;

	// Block 2: Forward Y-difference
	for (int y = 0; y < h - 1; y++) {
		for (int x = 0; x < w; x++) {
			int pos = x + y * w;
			double v = nzAt(y, x);
			triples.push_back(Triple(offset + pos, pos, v));
			triples.push_back(Triple(offset + pos, pos + w, -v));
		}
	}
	offset += w * h;

	// Block 3: Forward X-difference
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w - 1; x++) {
			int pos = x + y * w;
			double v = nzAt(y, x);
			triples.push_back(Triple(offset + pos, pos, -v));
			triples.push_back(Triple(offset + pos, pos + 1, v));
		}
	}
	offset += w * h;

	// Block 4: Backward X-difference
	for (int y = 0; y < h; y++) {
		for (int x = 1; x < w; x++) {
			int pos = x + y * w;
			double v = nzAt(y, x);
			triples.push_back(Triple(offset + pos, pos, v));
			triples.push_back(Triple(offset + pos, pos - 1, -v));
		}
	}
}

// Build derivative-only sparse matrix A (4 blocks)
static Eigen::SparseMatrix<double> BuildDerivativeMatrix(int w, int h, std::function<double(int,int)> nzAt) {
	int n = w * h;
	std::vector<Triple> triples;
	triples.reserve(n * 8);
	FillDerivativeTriples(triples, w, h, nzAt, 0);
	Eigen::SparseMatrix<double> A(n * 4, n);
	A.setFromTriplets(triples.begin(), triples.end());
	A.makeCompressed();
	return A;
}

// Build derivative matrix with appended lambda identity block (5th block)
static Eigen::SparseMatrix<double> BuildDerivativeMatrixWithLambda(const RowMatrixXd &tile_nz, int w, int h, double lambda) {
	int n = w * h;
	std::vector<Triple> triples;
	triples.reserve(n * 9);
	FillDerivativeTriples(triples, w, h, [&](int y, int x) { return tile_nz(y, x); }, 0);
	int offset = 4 * n;
	for (int i = 0; i < n; ++i) {
		triples.push_back(Triple(offset + i, i, lambda));
	}
	Eigen::SparseMatrix<double> A(n * 5, n);
	A.setFromTriplets(triples.begin(), triples.end());
	A.makeCompressed();
	return A;
}

Eigen::SparseMatrix<double> BuildTileMatrixWithScaffold(const RowMatrixXd& tile_nz, int w, int h, double lambda) {
	return BuildDerivativeMatrixWithLambda(tile_nz, w, h, lambda);
}
/**
 * Tiled integration that uses a pre-calculated global low-res depth scaffold
 * alongside overlapping alpha-feathered blending to ensure flawless continuity.
 */
bool IntegrateNormalsWithScaffold(
	const RowMatrixXd &global_nx,
	const RowMatrixXd &global_ny,
	const RowMatrixXd &global_nz,
	const RowMatrixXd &upsampled_scaffold,
	RowMatrixXd &global_depth,
	ProgressCallback progressed) {

	const int img_w = global_nx.cols();
	const int img_h = global_nx.rows();

	// Initialize the depth map and a matching matrix to track weight denominators
	global_depth = RowMatrixXd::Zero(img_h, img_w);
	RowMatrixXd global_weights = RowMatrixXd::Zero(img_h, img_w);

	const int TILE_SIZE = 512;
	const int PADDING = 32;                          // Bring padding back as an active overlap buffer
	const int STEP_SIZE = TILE_SIZE - (2 * PADDING); // 448 pixel step size

	// Soft regularization parameter guiding low-frequency structure without erasing micro-texture
	const double LAMBDA = 0.1;

	int total_tiles = std::ceil((double)img_h / STEP_SIZE) * std::ceil((double)img_w / STEP_SIZE);

	RelightThreadPool pool;
	unsigned int n_threads = std::thread::hardware_concurrency();
	if (n_threads == 0) n_threads = 1;
	pool.start(n_threads);

	std::vector<std::future<void>> futures;
	std::mutex write_mutex;

	for (int ty = 0; ty < img_h; ty += STEP_SIZE) {
		for (int tx = 0; tx < img_w; tx += STEP_SIZE) {

			int x_start = std::max(0, tx - PADDING);
			int y_start = std::max(0, ty - PADDING);
			int x_end = std::min(img_w, tx - PADDING + TILE_SIZE);
			int y_end = std::min(img_h, ty - PADDING + TILE_SIZE);

			int actual_tile_w = x_end - x_start;
			int actual_tile_h = y_end - y_start;

			if (actual_tile_w <= 0 || actual_tile_h <= 0) continue;

			futures.push_back(pool.queue([=, &global_nx, &global_ny, &global_nz, &upsampled_scaffold, &global_depth, &global_weights, &write_mutex]() {
				// Use dynamic sizes matching the slice bounds to prevent zero-bleed edge artifacts
				RowMatrixXd tile_nx = global_nx.block(y_start, x_start, actual_tile_h, actual_tile_w);
				RowMatrixXd tile_ny = global_ny.block(y_start, x_start, actual_tile_h, actual_tile_w);
				RowMatrixXd tile_nz = global_nz.block(y_start, x_start, actual_tile_h, actual_tile_w);
				RowMatrixXd tile_scaffold = upsampled_scaffold.block(y_start, x_start, actual_tile_h, actual_tile_w);

				int n = actual_tile_w * actual_tile_h;

				// 1. Build the local 5-block system matrix
				Eigen::SparseMatrix<double> A_tile = BuildTileMatrixWithScaffold(tile_nz, actual_tile_w, actual_tile_h, LAMBDA);

				// 2. Build local right hand side 'b' safely maps layout indexes sequentially
				Eigen::VectorXd b_tile(n * 5);
				for (int y = 0; y < actual_tile_h; ++y) {
					for (int x = 0; x < actual_tile_w; ++x) {
						int idx = x + y * actual_tile_w;
						b_tile(idx)         = -tile_nx(y, x);
						b_tile(n + idx)     = -tile_nx(y, x);
						b_tile(2 * n + idx) = -tile_ny(y, x);
						b_tile(3 * n + idx) = -tile_ny(y, x);
						b_tile(4 * n + idx) = LAMBDA * tile_scaffold(y, x);
					}
				}

				// 3. Formulate the normal equations
				Eigen::VectorXd W_diag = Eigen::VectorXd::Ones(n * 5);
				W_diag.head(n * 4).array() = 0.5;
				W_diag.tail(n).array() = 1.0;

				Eigen::SparseMatrix<double> At = A_tile.transpose();
				Eigen::SparseMatrix<double> AtW = At * W_diag.asDiagonal();
				Eigen::SparseMatrix<double> AtWA = AtW * A_tile;
				Eigen::VectorXd AtWb = AtW * b_tile;

				AtWA.diagonal().array() += 1e-9;

				// 4. Solve system analytically
				Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> tile_solver;
				tile_solver.compute(AtWA);
				if (tile_solver.info() != Eigen::Success) return;

				Eigen::VectorXd z_tile_flat = tile_solver.solve(AtWb);

				// 5. Accumulate results into global canvas using a linear feathering blend profile
				std::lock_guard<std::mutex> lg(write_mutex);
				for (int y = 0; y < actual_tile_h; ++y) {
					for (int x = 0; x < actual_tile_w; ++x) {
						int global_x = x_start + x;
						int global_y = y_start + y;

						// Horizontal ramp calculation
						double weight_x = 1.0;
						if (x_start > 0 && x < PADDING) {
							weight_x = (double)x / PADDING;
						} else if (x_end < img_w && x >= actual_tile_w - PADDING) {
							weight_x = (double)(actual_tile_w - 1 - x) / PADDING;
						}

						// Vertical ramp calculation
						double weight_y = 1.0;
						if (y_start > 0 && y < PADDING) {
							weight_y = (double)y / PADDING;
						} else if (y_end < img_h && y >= actual_tile_h - PADDING) {
							weight_y = (double)(actual_tile_h - 1 - y) / PADDING;
						}

						// Combine ramps into 2D product factor
						double blend_weight = weight_x * weight_y;

						double z_val = z_tile_flat(x + y * actual_tile_w);
						global_depth(global_y, global_x)   += z_val * blend_weight;
						global_weights(global_y, global_x) += blend_weight;
					}
				}
			}));
		}
	}

	// Wait for tasks and report progress
	int completed = 0;
	for (auto &f : futures) {
		f.get();
		++completed;
		if (progressed) {
			if (!progressed("Injecting details into scaffold...", (100 * completed) / total_tiles)) {
				pool.abort();
				return false;
			}
		}
	}

	pool.finish();

	// =================================================================
	// 6. NORMALIZATION PASS: Resolve overlapping weights uniformly
	// =================================================================
	for (int y = 0; y < img_h; ++y) {
		for (int x = 0; x < img_w; ++x) {
			double w = global_weights(y, x);
			if (w > 1e-5) {
				global_depth(y, x) /= w;
			}
		}
	}

	if (progressed) progressed("Surface fully integrated via multi-res scaffolding.", 100);
	return true;
}
