#pragma once

#include <functional>
#include "Triangulation.h"

#include "ScreenMeshing.h"
#include "ScreenRemeshing.h"
#include "../Grid.h"
#include <QString>
#include <functional>

template <typename Projection = pmp::Orthographic>
class PhotometricRemeshing
{
private:
	pmp::SurfaceMesh mesh_;
	const Grid<Eigen::Vector3f> &normals_;
	const Grid<unsigned char> &mask_;
	Projection projection_;

	int height_, width_;
	int num_pixels_;

public:
	PhotometricRemeshing(const Grid<Eigen::Vector3f>& normals, const Grid<unsigned char>& mask, Projection projection = Projection()) :
		normals_(normals), mask_(mask), projection_(projection)
	{
		assert(mask_.cols() == normals_.cols() && mask_.rows() == normals_.rows() && "Mask and Normals must have the same Dimensions!");

		height_ = normals_.rows();
		width_ = normals_.cols();
	}

	pmp::SurfaceMesh& mesh()
	{
		return mesh_;
	}

	int n_pixels()
	{
		return num_pixels_;
	}

	int n_vertices()
	{
		return mesh_.n_vertices();
	}

	int n_faces()
	{
		return mesh_.n_faces();
	}

	void create_domain()
	{
		// Create Quadmesh
		std::function<bool(int, int)> indicator = [&](int v, int u){ return (u != 0) && (v != 0) && (u != width_ - 1) && (v != height_ - 1) && mask_.at(v, u) > 127; };

		mesh_ = pmp::from_indicator(indicator, height_, width_);

		num_pixels_ = mesh_.n_faces();
	}

	void triangulate()
	{
		// Triangulate Quad-Mesh
		pmp::Triangulation triangulator(mesh_);
		triangulator.triangulate(pmp::Triangulation::Objective::MAX_ANGLE); // Objective doesn't matter in the particular case
	}

	void remesh(pmp::Scalar l_min, pmp::Scalar l_max, pmp::Scalar approx_error, int iterations = 10, bool delaunay = true,
				std::function<bool(QString stage, int percent)> *callback = nullptr)
	{
		// Start remeshing
		pmp::ScreenRemeshing<Projection> remesher(mesh_, normals_, mask_, projection_);

		for (int i = 0; i != iterations; ++i)
		{
			if(callback && !(*callback)("Remeshing", 100*i/iterations))
				return;
			remesher.adaptive_remeshing(l_min, l_max, approx_error, 1, delaunay);
		}
	}

	void run(pmp::Scalar l_min, pmp::Scalar l_max, pmp::Scalar approx_error, int iterations = 10, bool delaunay = true,
			 std::function<bool(QString stage, int percent)> *callback = nullptr)
	{
		// smoothen_normals(l_min);
		create_domain();
		triangulate();
		remesh(l_min, l_max, approx_error, iterations, delaunay, callback);

		std::cout << "Remeshing: " << n_pixels() << " Pixels -> " << n_vertices() << " Vertices\n";
	}
};
