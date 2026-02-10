#pragma once

#include "../SurfaceMesh.h"
#include "ScreenDifferentialGeometry.h"
#include <Eigen/Sparse>
#include <Eigen/Core>
#include <QString>
#include <functional>

namespace pmp
{
template <typename Camera>
struct IntegrationPostProcessor
{
	template <typename T>
	constexpr T operator()(const T& x)
	{
		return x;
	}
};

template <>
struct IntegrationPostProcessor<Perspective>
{
	template <typename T>
	constexpr T operator()(const T& x)
	{
		return std::exp(x);
	}
};

template <typename T, typename Camera = Orthographic>
class Integration
{
private:
	SurfaceMesh& mesh_;
	ScreenDifferentialGeometry<Camera> geo_;
	Camera projection_;
	
	T cotan_weight(Halfedge h) const
	{
		return geo_.cotan_weight(h);
	}

	// Ratio between the longest and the shortest edge
	T length_ratio(pmp::Face f)
	{
		T min = std::numeric_limits<T>::max();
		T max = 0;
		T length;

		for (auto h : mesh_.halfedges(f))
		{
			length = static_cast<T>(distance(mesh_.position(mesh_.to_vertex(h)), mesh_.position(mesh_.from_vertex(h))));

			min = std::min(length, min);
			max = std::max(length, max);
		}

		return max / (min + std::numeric_limits<float>::min());
	}

public:
	Integration(SurfaceMesh& mesh, const Grid<Eigen::Vector3f>& normals, const Grid<unsigned char>& mask, Camera camera = Camera()) :
		mesh_(mesh), geo_(mesh, normals, mask, camera), projection_(camera)
	{

	}

	T run(std::function<bool(QString stage, int percent)> *callback = nullptr)
	{
		mesh_.garbage_collection();
		geo_.update();

		int n = mesh_.n_vertices();

		Eigen::SparseMatrix<T> L(n, n);
		//Eigen::Vector<T, -1> b = Eigen::Vector<T, -1>::Zero(n);
		Eigen::Matrix<T, -1, 1> b = Eigen::Matrix<T, -1, 1>::Zero(n);

		// Assembly:
		std::vector<Eigen::Triplet<T>> coefficients; // list of non-zeros coefficients
		coefficients.reserve(4 * mesh_.n_edges());

		Vertex v0, v1, v2;
		Halfedge h0, h1;
		Face f0, f1;

		T trace = 0;
		T l00, l01, l11;
		T b0, b1;
		T cot;

		T r0n, r1n, r2n;

		Vector<T, 3> r0, r1, r2, normal, dr;

		if(callback && !(*callback)("Solving linear system", 0))
			return 0;

		size_t ei = 0;
		size_t etot = std::max(size_t(2), mesh_.n_edges());
		size_t estep = std::max(size_t(2), etot/100);

		for (auto e : mesh_.edges())
		{
			if((ei % estep) == 0) {
				if(callback && !(*callback)("Initializing edges", 100*ei/etot))
					return 0;
			}
			h0 = mesh_.halfedge(e, 0);
			h1 = mesh_.halfedge(e, 1);

			v0 = mesh_.to_vertex(h0);
			v1 = mesh_.to_vertex(h1);

			f0 = mesh_.face(h0);
			f1 = mesh_.face(h1);

			r0 = Vector<T, 3>(projection_.dz(mesh_.position(v0)[0], mesh_.position(v0)[1]));
			r1 = Vector<T, 3>(projection_.dz(mesh_.position(v1)[0], mesh_.position(v1)[1]));

			dr = Vector<T, 3>((mesh_.position(v0)[0] - mesh_.position(v1)[0]) * projection_.du() + (mesh_.position(v0)[1] - mesh_.position(v1)[1]) * projection_.dv());

			l00 = std::numeric_limits<T>::epsilon(); // Avoid zeros
			b0 = 0;

			if (f0.is_valid())
			{
				v2 = mesh_.to_vertex(mesh_.next_halfedge(h0));
				r2 = Vector<T, 3>(projection_.dz(mesh_.position(v2)[0], mesh_.position(v2)[1]));
				normal = Vector<T, 3>(geo_.normal(f0));

				r0n = std::min<T>(dot(normal, r0), -ScreenDifferentialGeometry<Camera>::cos_max_angle);
				r1n = std::min<T>(dot(normal, r1), -ScreenDifferentialGeometry<Camera>::cos_max_angle);
				r2n = std::min<T>(dot(normal, r2), -ScreenDifferentialGeometry<Camera>::cos_max_angle);

				cot = cotan_weight(h0);

				l00 += cot / 6. * (r0n * r0n + r1n * r1n + r2n * r2n + r0n * r1n +  r1n * r2n + r2n * r0n);
				b0 += cot / 3. * (r0n + r1n + r2n) * dot(normal, dr);
			}

			if (f1.is_valid())
			{
				v2 = mesh_.to_vertex(mesh_.next_halfedge(h1));
				r2 = Vector<T, 3>(projection_.dz(mesh_.position(v2)[0], mesh_.position(v2)[1]));
				normal = Vector<T, 3>(geo_.normal(f1));

				r0n = std::min<T>(dot(normal, r0), -ScreenDifferentialGeometry<Camera>::cos_max_angle);
				r1n = std::min<T>(dot(normal, r1), -ScreenDifferentialGeometry<Camera>::cos_max_angle);
				r2n = std::min<T>(dot(normal, r2), -ScreenDifferentialGeometry<Camera>::cos_max_angle);

				cot = cotan_weight(h1);

				l00 += cot / 6. * (r0n * r0n + r1n * r1n + r2n * r2n + r0n * r1n +  r1n * r2n + r2n * r0n);
				b0 += cot / 3. * (r0n + r1n + r2n) * dot(normal, dr);
			}

			l01 = -l00;
			l11 = l00;

			b1 = -b0;

			coefficients.emplace_back(v0.idx(), v0.idx(), l00);
			coefficients.emplace_back(v0.idx(), v1.idx(), l01);

			coefficients.emplace_back(v1.idx(), v1.idx(), l11);
			coefficients.emplace_back(v1.idx(), v0.idx(), l01);

			b(v0.idx()) += b0;
			b(v1.idx()) += b1;

			// std::cout << "n = " << normal << "\n";

			trace += l00 + l11;
		}

		// Build Linear System
		L.setFromTriplets(coefficients.begin(), coefficients.end());

		T reg = std::numeric_limits<T>::epsilon(); // / static_cast<T>(n - 1);

		L.diagonal().array() += reg;
		b.array() += reg;
		if(callback && !(*callback)("Initializing linear system", 0))
			return 0;

		Eigen::ConjugateGradient<Eigen::SparseMatrix<T>> cg;
		cg.compute(L);
		//Eigen::Vector<T, -1> depth = cg.solve(b);
		if(callback && !(*callback)("Solving linear system", 0))
			return 0;
		Eigen::Matrix<T, -1, 1> depth = cg.solve(b);

		depth.array() -= depth.mean();

		IntegrationPostProcessor<Camera> processor;
		size_t i = 0;
		size_t tot = std::max(size_t(2), mesh_.n_vertices());
		size_t step = std::max(size_t(2), tot/100);

		for(auto v: mesh_.vertices()) {
			if((i % step) == 0) {
				if(callback && !(*callback)("Ingegrating", 100*i/tot))
					return 0;
			}
			mesh_.position(v)[2] = processor(depth(v.idx()));
			i++;
		}

		//std::cout << "Integration: Residual is " << cg.error() << " after " << cg.iterations() << " iterations\n";

		return cg.error();
	}

	// Remove elongated triangles that can occur around the border during integration. Return how many triangles were removed.
	int remove_slivers(const T& threshold = 20.)
	{
		int count = 0;
		bool removed = true;

		while (removed)
		{
			removed = false;

			for (auto f : mesh_.faces())
			{
				if (mesh_.is_boundary(f))
				{
					T ratio = length_ratio(f);

					if (ratio > threshold)
					{
						mesh_.delete_face(f);
						removed = true;
						count++;
					}
				}
			}
		}

		mesh_.garbage_collection();

		return count;
	}
};
}
