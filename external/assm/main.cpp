
#include "ArgumentParser.h"
#include <assm/Grid.h>
#include <assm/algorithms/PhotometricRemeshing.h>
#include <assm/algorithms/Integration.h>

#include <Eigen/Dense>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <chrono>

using namespace std::chrono;
using namespace std;

void print_help_message()
{
	std::cout << "Options\n";
	std::cout << "  -n <path-to-normal-map>      = Path to the normal maps (as .exr file)\n";
	std::cout << "  -e <approximation-error>     = Desired approximation error (orthographic: pixels, perspective: mm)\n";
	std::cout << "  -t <path-to-triangle-mesh>   = Path to write the output mesh to (we recommend using .obj files)\n";
	std::cout << "\n";
	std::cout << "  -m <path-to-foreground-mask> = Path to foreground mask (as b/w .png file, optional) \n";
	std::cout << "  -l <lower-limit>             = Minimal allowed edge length (optional, default: 1px)\n";
	std::cout << "  -h <higher-limit>            = Maximal allowed edge length (optional, default: 100px)\n";

	std::cout << "\nYou can switch from orthographic to perspective projection by supplying intrinsics\n";
	std::cout << "  | -x  0 -u |\n";
	std::cout << "  |  0 -y -v |\n";
	std::cout << "  |  0  0  1 |\n";
	std::cout << "\nPlease Note: In orthographic mode, all lengths (approximation error, minimal and maximal lengths) are in pixels.\nIn perspective mode, we use millimeters instead.\n";
}

bool saveObj(const char *filename, pmp::SurfaceMesh &mesh) {
	FILE *fp = fopen(filename, "wb");
	if(!fp) {
		cerr << "Could not open file: " << filename << endl;
		return false;
	}
	for(auto vertex: mesh.vertices()) {
		auto p = mesh.position(vertex);
		fprintf(fp, "v %f %f %f\n", p[0], p[1], p[2]);
	}
	for (auto face : mesh.faces()) {
		int indexes[3];
		int count =0 ;
		for (auto vertex : mesh.vertices(face)) {
			auto p = mesh.position(vertex);
			indexes[count++] = vertex.idx() + 1;
		}
		fprintf(fp, "f %d %d %d\n", indexes[0], indexes[1], indexes[2]);
	}
	fclose(fp);
	return true;
}

Grid<Eigen::Vector3f> loadNormalMap(const char *filename) {
	cv::Mat normals_char = cv::imread(filename, cv::IMREAD_UNCHANGED);
	cv::Mat normals_float;
	normals_char.convertTo(normals_float, CV_32FC3);

	// Discard alpha if neccessary
	if (normals_float.channels() == 4) {
		cv::cvtColor(normals_float, normals_float, cv::COLOR_BGRA2BGR);

	} else if (normals_float.channels() != 3) {
		std::cout << "Error: Normal Map must have three/four channels.\n";
		exit(0);
	}


	Grid<Eigen::Vector3f> normals(normals_float.rows, normals_float.cols, Eigen::Vector3f::Zero());

	for (int v = 0; v != normals_float.rows; ++v) {
		for (int u = 0; u != normals_float.cols; ++u) {
			cv::Vec3f n = normals_float.at<cv::Vec3f>(v, u);
			n[0] = -n[0] + 127.0f;
			n[1] = -n[1] + 127.0f;
			n[2] = -n[2] + 127.0f;
			n /= sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);

			normals.at(v, u) = Eigen::Vector3f(n[2], n[1], n[0]); //z y x because of opencv loads images as bgr
		}
	}

	return normals;
}

Grid<unsigned char> loadMask(const std::string &filename) {
	cv::Mat maskcv = cv::imread(filename, cv::IMREAD_GRAYSCALE);
	if (maskcv.empty()) {
		std::cout << "Error: Could not read provided foreground mask!\n";
		std::cout << "The foreground mask must be a black-and-white .png image.\n";
		exit(0);
	}

	Grid<unsigned char> mask(maskcv.rows, maskcv.cols, 0);
	for (int v = 0; v != maskcv.rows; ++v) {
		for (int u = 0; u != maskcv.cols; ++u) {
			mask.at(v, u) = maskcv.at<uchar>(v, u);
		}
	}
	return mask;
}


int main(int argc, char *argv[]) {
	ArgumentParser parser(argc, argv);

	if (!parser.has_arguments()) {
		print_help_message();
		return 0;
	}

	if (!parser.has_argument('n')) {
		std::cout << "Error: No Normal Map provided!\n\n";
		print_help_message();
		return 1;
	}

	if (!parser.has_argument('t')) {
		std::cout << "Warning: No output Mesh provided!\n\n";
	}


	Grid<Eigen::Vector3f> normals = loadNormalMap(parser.get_argument('n').c_str());

	int height = normals.rows();
	int width = normals.cols();

	Grid<unsigned char> mask(0, 0, 0); //null mask.

	if (parser.has_argument('m'))
		mask = loadMask(parser.get_argument('m'));

	if(mask.rows() == 0) {
		mask = Grid<unsigned char>(normals.rows(), normals.cols(), 0);
		mask.fill(255);
	}

	// Perspective Mode
	if (parser.has_argument('x'))
	{
		float ax = std::stod(parser.get_argument('x'));
		float ay = parser.has_argument('y') ? std::stod(parser.get_argument('y')) : ax;

		float u0 = parser.has_argument('u') ? std::stod(parser.get_argument('u')) : static_cast<float>(width) / 2.;
		float v0 = parser.has_argument('v') ? std::stod(parser.get_argument('v')) : static_cast<float>(height) / 2.;

		float scale = 1. / std::sqrt(std::abs(ax * ay)); // Approximately the size of one pixel

		// Load remeshing values
		float l_min = parser.has_argument('l') ? std::stof(parser.get_argument('l')) : scale;
		float l_max = parser.has_argument('h') ? std::stof(parser.get_argument('h')) : 100. * scale;
		float approx_error = parser.has_argument('e') ? (std::stof(parser.get_argument('e')) / 1000.) : 1. * scale;

		// Run remeshing
		PhotometricRemeshing<pmp::Perspective> remesher(normals, mask, pmp::Perspective(ax, -ay, u0, v0));
		remesher.run(l_min, l_max, approx_error);

		// Perform integration
		pmp::Integration<double, pmp::Perspective> integrator(remesher.mesh(), normals, mask, pmp::Perspective(ax, -ay, u0, v0));
		integrator.run();

		// Transform mesh to world-coordinates
		for (auto v : remesher.mesh().vertices())
		{
			auto pos = remesher.mesh().position(v);

			pos[0] -= u0;
			pos[0] /= ax;

			pos[1] -= v0;
			pos[1] /= ay;

			remesher.mesh().position(v) = pos;
		}

		// Remove slivers
		int slivers = integrator.remove_slivers();

		if (slivers > 0)
		{
			std::cout << "Removed " << slivers << " slivers\n";
		}

		if (parser.has_argument('t'))
		{
			saveObj(parser.get_argument('t').c_str(), remesher.mesh());
		}
	}
	// Orthographic Mode
	else
	{
		// Load remeshing values
		float l_min = parser.has_argument('l') ? std::stof(parser.get_argument('l')) : 1;
		float l_max = parser.has_argument('h') ? std::stof(parser.get_argument('h')) : 100.;
		float approx_error = parser.has_argument('e') ? std::stof(parser.get_argument('e')) : 1.;

		auto now = high_resolution_clock::now();

		cout << "Remeshing" << endl;
		PhotometricRemeshing<pmp::Orthographic> remesher(normals, mask);
		remesher.run(l_min, l_max, approx_error);

		auto end = high_resolution_clock::now();
		auto time = duration_cast<seconds>(end - now);
		cout << "Took: " << time.count() << endl;

		cout << "integrating" << endl;

		// Perform integration
		pmp::Integration<double, pmp::Orthographic> integrator(remesher.mesh(), normals, mask);
		integrator.run();

		auto rend = high_resolution_clock::now();
		auto rtime = duration_cast<seconds>(rend - end);
		cout << "Took: " << rtime.count() << endl;

		// Remove slivers
		int slivers = integrator.remove_slivers();

		if (slivers > 0)
		{
			std::cout << "Removed " << slivers << " slivers\n";
		}

		if (parser.has_argument('t'))
		{
			saveObj(parser.get_argument('t').c_str(), remesher.mesh());
		}
	}
}
