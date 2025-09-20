#pragma once

#include <queue>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include "DifferentialGeometry.h"
#include "../Grid.h"
#include "Rasterizer.h"

namespace pmp
{
namespace projection
{
// Classes to handle the two types of cameras:
template <typename VectorType = Point>
class Orthographic
{
public:
	// dx/du
	inline VectorType du() const
	{
		return VectorType(1, 0, 0);
	}

	// dx/dv
	inline VectorType dv() const
	{
		return VectorType(0, -1, 0);
	}

	// dx/dz
	inline VectorType dz(const Scalar&, const Scalar&) const
	{
		return VectorType(0, 0, 1);
	}
};

template <typename VectorType = Point>
class Perspective
{
private:
	Scalar invax_, invay_, u0_, v0_;

public:
	Perspective(const Scalar& ax, const Scalar& ay, const Scalar& u0, const Scalar& v0) : invax_(1. / ax), invay_(1. / ay), u0_(u0), v0_(v0)
	{

	}

	// dx/du
	inline VectorType du() const
	{
		return VectorType(invax_, 0, 0);
	}

	// dx/dv
	inline VectorType dv() const
	{
		return VectorType(0, invay_, 0);
	}

	// dx/dz
	inline VectorType dz(const Scalar& u, const Scalar& v) const
	{
		return VectorType(invax_ * (u - u0_), invay_ * (v - v0_), 1);
	}
};
}

using Orthographic = projection::Orthographic<Point>;
using Perspective = projection::Perspective<Point>;

template<typename Camera = Orthographic>
class ScreenDifferentialGeometry
{
public:
	static constexpr Scalar degrees       = 0.017453292; //M_PI / 180.;
	static constexpr Scalar cos_max_angle = 0.087155743; //std::cos(85. * degrees);
	static constexpr Scalar sin_max_angle = 0.996194698; //std::sqrt(1. - cos_max_angle * cos_max_angle);
	static constexpr float sigma_         = 1.414213562; //std::sqrt(2);
private:
	SurfaceMesh& mesh_;
	Camera camera_;
	const Grid<Eigen::Vector3f> &normals_;
	const Grid<unsigned char> &mask_;
	Grid<float> max_abs_curvatures_;
	//cv::Mat max_abs_curvatures_;

	FaceProperty<Normal> fnormal_;
	FaceProperty<Matrix<Scalar, 2, 2>> fmetric_;
	FaceProperty<Scalar> fmax_abs_curvature_;

	int umin_, vmin_, umax_, vmax_;

	// Some mathematical helper functions
	Scalar determinant(const Matrix<Scalar, 2, 2>& mat) const
	{
		return mat(0, 0) * mat(1, 1) - mat(0, 1) * mat(1, 0);
	}

	Scalar trace(const Matrix<Scalar, 2, 2>& mat) const
	{
		return mat(0, 0) + mat(1, 1);
	}

	// Calculate metric, i.e. first fundamental form the surface normal
	Matrix<Scalar, 2, 2> metric(const Point& pos, const Normal& normal) const
	{
		Matrix<Scalar, 2, 2> g;

		auto t = tangents(normal, pos);

		g(0, 0) = dot(t.first, t.first);
		g(0, 1) = dot(t.first, t.second);
		g(1, 1) = dot(t.second, t.second);
		g(1, 0) = g(0, 1);

		return g;
	}

	// Pre-calculate curvature
	void calculate_curvature()
	{
		int width = normals_.cols();
		int height = normals_.rows();

		max_abs_curvatures_ = Grid<float>(width, height, 0.0f);

		Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::Matrix<Scalar, 2, 2>> solver;

		Eigen::Matrix<Scalar, 3, 2> dX = Eigen::Matrix<Scalar, 3, 2>::Zero(); dX(0, 0) = 1; dX(1, 1) = 1;
		Eigen::Matrix<Scalar, 3, 2> dN;

		Eigen::Matrix<Scalar, 2, 2> I, II;
		Normal normal;

		Grid<Eigen::Vector3f> blurredNormals = normals_.gaussianBlur(5, sigma_);

		for(auto &n: blurredNormals)
			n /= n.norm();

		int hp, hm, vp, vm;

		umin_ = width;
		vmin_ = height;
		umax_ = 0;
		vmax_ = 0;

		// Take derivatives to calculate curvature
		for (int v = 0; v != height; ++v)
		{
			for (int u = 0; u != width; ++u)
			{
				hp = u + 1 < width ? u + 1 : u;
				hm = u > 0 ? u - 1 : u;

				vp = v + 1 < height ? v + 1 : v;
				vm = v > 0 ? v - 1 : v;

				if (mask_.empty() || ((mask_.at(v, u) > 127) && (mask_.at(vp, hp) > 127) && (mask_.at(vm, hp) > 127) && (mask_.at(vp, hm) > 127) && (mask_.at(vm, hm) > 127)))
				{
					// Let h be width/height of a pixel in the capturing volume. Then, dX ~ h and dN ~ 1/h. Acordingly, dX * dN ~ h/h = 1
					// Therefore, we can simply neglect the resolution in the subsequent calculations.

					// Remember BGR to RGB
					Eigen::Vector3f &n = blurredNormals.at(v, u);
					normal = Normal(n[0], n[1], n[2]);

					auto t = tangents(normal, pmp::Point(u, v, 1));

					dX.col(0) = Eigen::Matrix<Scalar, 3, 1>(t.first);
					dX.col(1) = Eigen::Matrix<Scalar, 3, 1>(t.second);

					for (int i = 0; i < 3; ++i)
					{
						// Remember BGR to RGB!
						dN(i, 0) = (blurredNormals.at(v, hp)[i] - blurredNormals.at(v, hm)[i]) / static_cast<float>(hp - hm);
						dN(i, 1) = (blurredNormals.at(vp, u)[i] - blurredNormals.at(vm, u)[i]) / static_cast<float>(vp - vm);
					}

					// Calculate first and second fundamental form
					I = dX.transpose() * dX;
					II = dX.transpose() * dN;

					// Make symmetric
					II = 0.5 * (II + II.transpose());

					// Solve the generalized eigenvalue problem
					solver.compute(II, I, Eigen::EigenvaluesOnly);

					// Save pixel-wise maximum absolute curvature
					max_abs_curvatures_.at(v, u) = solver.eigenvalues().array().abs().matrix().maxCoeff();

					umin_ = std::min(u, umin_);
					vmin_ = std::min(v, vmin_);
					umax_ = std::max(u + 1, umax_);
					vmax_ = std::max(v + 1, vmax_);
				}
				else
				{
					max_abs_curvatures_.at(v, u) = 0.0f;
				}
			}
		}
	}

public:
	ScreenDifferentialGeometry(SurfaceMesh& mesh, const Grid<Eigen::Vector3f>& normals, const Grid<unsigned char >& mask, Camera camera = Camera()) :
		mesh_(mesh), camera_(camera), normals_(normals), mask_(mask)
	{
		calculate_curvature();

		fnormal_ = mesh_.face_property<Normal>("f:normal", Normal(0));
		fmetric_ = mesh_.face_property<Matrix<Scalar, 2, 2>>("f:metric");
		fmax_abs_curvature_ = mesh_.face_property<Scalar>("f:maxAbsCurvature", -1);
		update();
	}

	~ScreenDifferentialGeometry()
	{
		mesh_.remove_face_property(fnormal_);
		mesh_.remove_face_property(fmetric_);
		mesh_.remove_face_property(fmax_abs_curvature_);
	}

	void update()
	{
		// Reset properties
		for (auto f : mesh_.faces())
		{
			fmax_abs_curvature_[f] = -1;
			fnormal_[f] = Normal(0);
		}

		Rasterizer rasterizer(umax_ - umin_, vmax_ - vmin_);
		for (auto face : mesh_.faces()) {

			Eigen::Vector3f v[3];
			int count =0 ;
			for (auto vertex : mesh_.vertices(face)) {
				auto p = mesh_.position(vertex);
				v[count++] = p;
			}

			rasterizer.DrawTriangle(
						v[0][2], v[0][0] -umin_, v[0][1]  -vmin_,
					v[1][2], v[1][0] -umin_, v[1][1]  -vmin_,
					v[2][2], v[2][0] -umin_, v[2][1]  -vmin_, face.idx());
		}

		Normal normal;

		// Transfer from pixel-grid to triangle mesh
		for (int v = 0; v != vmax_ - vmin_; ++v)
		{
			for (int u = 0; u != umax_ - umin_; ++u)
			{
				const int& id1 = rasterizer.ids[v*(umax_ - umin_) + u];

				if (id1 >= 0 && (mask_.empty() || mask_.at(v + vmin_, u + umin_) > 127))
				{
					Face f(id1);
					const Eigen::Vector3f &n = normals_.at(v + vmin_, u + umin_);
					normal =
							fnormal_[f] += Normal(n[0], n[1], n[2]);

					fmax_abs_curvature_[f] = std::max<Scalar>(max_abs_curvatures_.at(v + vmin_, u + umin_), fmax_abs_curvature_[f]);
				}
			}
		}

		// A few triangles might not cover a pixel. Identify them ...
		std::queue<Face> undefined;

		for (auto f : mesh_.faces())
		{
			if (fmax_abs_curvature_[f] >= 0)
			{
				fnormal_[f] = normalize(fnormal_[f]);
			}
			else
			{
				undefined.push(f);
			}
		}

		if (undefined.size() == mesh_.n_faces())
		{
			std::cout << "No Triangles within the Image domain\n";
			std::exit(1);
		}

		Scalar w, ww;

		// and ... interpolate normal and max abs curvature from adjacent triangles with valid values
		for (size_t i = 0; i != mesh_.n_faces() && !undefined.empty(); ++i)
		{
			auto f = undefined.front(); undefined.pop();
			ww = 0;

			fnormal_[f] = Normal(0);

			for (auto h : mesh_.halfedges(f))
			{
				if (!mesh_.is_boundary(mesh_.edge(h)))
				{
					auto ff = mesh_.face(mesh_.opposite_halfedge(h));

					if (fmax_abs_curvature_[ff] >= 0.)
					{
						w = 1. / std::max<Scalar>(pmp::cotan_weight(mesh_, mesh_.edge(h)), std::numeric_limits<Scalar>::epsilon());
						fnormal_[f] += w * fnormal_[ff];
						fmax_abs_curvature_[f] = std::max<Scalar>(fmax_abs_curvature_[f], 0) + w * fmax_abs_curvature_[ff];
						ww += w;
					}
				}
			}

			if (fmax_abs_curvature_[f] >= 0.) // Normalize
			{
				fmax_abs_curvature_[f] /= ww;
				fnormal_[f] = normalize(fnormal_[f]);
			}
			else // Mark as undefined and put back in queue;
			{
				undefined.push(f);
			}
		}

		// Calculate face-wise metric and max absolute curvature
		for (auto f : mesh_.faces())
		{
			fmax_abs_curvature_[f] = std::max<Scalar>(fmax_abs_curvature_[f], 0);
			fmetric_[f] = metric(centroid(mesh_, f), fnormal_[f]);
		}
	}

	// Some geometric properties of the mesh that can be derived from the first fundamental form
	inline Scalar area(Face f) const
	{
		return std::sqrt(determinant(fmetric_[f])) * triangle_area(mesh_, f);
	}

	Scalar length(Edge e) const
	{
		auto v0 = mesh_.vertex(e, 0);
		auto v1 = mesh_.vertex(e, 1);

		Vector<Scalar, 2> d = Vector<Scalar, 2>(mesh_.position(v0)[0] - mesh_.position(v1)[0], mesh_.position(v0)[1] - mesh_.position(v1)[1]);

		Matrix<Scalar, 2, 2> metric(0);
		Scalar ww = 0;

		auto f0 = mesh_.face(e, 0);
		auto f1 = mesh_.face(e, 1);

		if (f0.is_valid())
		{
			metric += fmetric_[f0];
			ww += 1;
		}

		if (f1.is_valid())
		{
			metric += fmetric_[f1];
			ww += 1;
		}

		return sqrt(dot(d, metric * d) / ww);
	}

	// Interior angle of the face adjacent to h at the vertex h is pointing to
	Scalar angle(Halfedge h)
	{
		assert (!mesh_.is_boundary(h) && "Angles are not well-defined for boundary halfedges");

		auto hh = mesh_.next_halfedge(h);

		auto vp = mesh_.from_vertex(h);
		auto v = mesh_.to_vertex(h);
		auto vn = mesh_.to_vertex(hh);

		// Get edge vectors
		Vector<Scalar, 2> d0 = Vector<Scalar, 2>(mesh_.position(vp)[0] - mesh_.position(v)[0], mesh_.position(vp)[1] - mesh_.position(v)[1]);
		Vector<Scalar, 2> d1 = Vector<Scalar, 2>(mesh_.position(vn)[0] - mesh_.position(v)[0], mesh_.position(vn)[1] - mesh_.position(v)[1]);

		// Normalize
		d0 /= std::sqrt(dot(d0, fmetric_[mesh_.face(h)] * d0));
		d1 /= std::sqrt(dot(d1, fmetric_[mesh_.face(h)] * d1));

		return std::acos(dot(d0, fmetric_[mesh_.face(h)] * d1));
	}

	Scalar cotan_weight(Edge e) const
	{
		Scalar weight = 0.0;

		const Halfedge h0 = mesh_.halfedge(e, 0);
		const Halfedge h1 = mesh_.halfedge(e, 1);

		const auto& p0 = mesh_.position(mesh_.to_vertex(h0));
		const auto& p1 = mesh_.position(mesh_.to_vertex(h1));
		Point d0, d1;
		Scalar A, cot;
		Face f;

		if (!mesh_.is_boundary(h0))
		{
			const auto& p2 = mesh_.position(mesh_.to_vertex(mesh_.next_halfedge(h0)));
			d0 = p0 - p2;
			d1 = p1 - p2;
			f = mesh_.face(h0);
			A = area(f);

			if (A > std::numeric_limits<Scalar>::min())
			{
				cot = (fmetric_[f](0, 0) * d0[0] * d1[0] + fmetric_[f](1, 1) * d0[1] * d1[1] + fmetric_[f](0, 1) * d0[0] * d1[1] + fmetric_[f](1, 0) * d0[1] * d1[0]) / A;
				weight += cot;
			}
		}

		if (!mesh_.is_boundary(h1))
		{
			const auto& p2 = mesh_.position(mesh_.to_vertex(mesh_.next_halfedge(h1)));
			d0 = p0 - p2;
			d1 = p1 - p2;
			f = mesh_.face(h1);
			A = area(f);

			if (A > std::numeric_limits<Scalar>::min())
			{
				cot = (fmetric_[f](0, 0) * d0[0] * d1[0] + fmetric_[f](1, 1) * d0[1] * d1[1] + fmetric_[f](0, 1) * d0[0] * d1[1] + fmetric_[f](1, 0) * d0[1] * d1[0]) / A;
				weight += cot;
			}
		}

		return weight;
	}

	Scalar cotan_weight(Halfedge h) const
	{
		Scalar weight = 0.0;

		if (!mesh_.is_boundary(h))
		{
			const auto& p0 = mesh_.position(mesh_.to_vertex(h));
			const auto& p1 = mesh_.position(mesh_.from_vertex(h));
			const auto& p2 = mesh_.position(mesh_.to_vertex(mesh_.next_halfedge(h)));

			Scalar A, cot;
			Face f;

			Point d0 = p0 - p2;
			Point d1 = p1 - p2;
			f = mesh_.face(h);
			A = area(f);

			if (A > std::numeric_limits<Scalar>::min())
			{
				cot = (fmetric_[f](0, 0) * d0[0] * d1[0] + fmetric_[f](1, 1) * d0[1] * d1[1] + fmetric_[f](0, 1) * d0[0] * d1[1] + fmetric_[f](1, 0) * d0[1] * d1[0]) / A;
				weight += cot;
			}
		}

		return weight;
	}

	Scalar is_delaunay(Edge e) const
	{
		if (mesh_.is_boundary(e))
		{
			return true;
		}

		Scalar total_angle = 0;

		auto h0 = mesh_.halfedge(e, 0);
		auto h1 = mesh_.halfedge(e, 1);

		auto f0 = mesh_.face(h0);
		auto f1 = mesh_.face(h1);

		auto v0 = mesh_.to_vertex(h0);
		auto v1 = mesh_.to_vertex(h1);
		auto v2 = mesh_.to_vertex(mesh_.next_halfedge(h0));
		auto v3 = mesh_.to_vertex(mesh_.next_halfedge(h1));

		auto g = metric(0.25 * (mesh_.position(v0) + mesh_.position(v1) + mesh_.position(v2) + mesh_.position(v3)), (area(f0) * normal(f0) + area(f1) * normal(f1)));

		Vector<Scalar, 2> d0(mesh_.position(v0)[0] - mesh_.position(v2)[0], mesh_.position(v0)[1] - mesh_.position(v2)[1]);
		Vector<Scalar, 2> d1(mesh_.position(v1)[0] - mesh_.position(v2)[0], mesh_.position(v1)[1] - mesh_.position(v2)[1]);

		d0 /= std::sqrt(pmp::dot(d0, g * d0));
		d1 /= std::sqrt(pmp::dot(d1, g * d1));

		total_angle += std::acos(pmp::dot(d0, d1));

		d0[0] = mesh_.position(v0)[0] - mesh_.position(v3)[0];
		d0[1] = mesh_.position(v0)[1] - mesh_.position(v3)[1];

		d1[0] = mesh_.position(v1)[0] - mesh_.position(v3)[0];
		d1[1] = mesh_.position(v1)[1] - mesh_.position(v3)[1];

		d0 /= std::sqrt(pmp::dot(d0, g * d0));
		d1 /= std::sqrt(pmp::dot(d1, g * d1));

		total_angle += std::acos(pmp::dot(d0, d1));

		return M_PI - total_angle;
	}

	// Check whether the quad (whose diagonal is e) is convex
	bool is_convex(Edge e) const
	{
		auto h0 = mesh_.halfedge(e, 0);
		auto h1 = mesh_.halfedge(e, 1);

		auto v0 = mesh_.to_vertex(h0);
		auto v1 = mesh_.to_vertex(h1);
		Vertex v2, v3;

		if (!mesh_.is_boundary(h0))
		{
			v2 = mesh_.to_vertex(mesh_.next_halfedge(h0));

			if (pmp::angle(mesh_.position(v0) - mesh_.position(v2), mesh_.position(v1) - mesh_.position(v2)) >= M_PI)
			{
				return false;
			}
		}

		if (!mesh_.is_boundary(h1))
		{
			v3 = mesh_.to_vertex(mesh_.next_halfedge(h1));

			if (pmp::angle(mesh_.position(v0) - mesh_.position(v3), mesh_.position(v1) - mesh_.position(v3)) >= M_PI)
			{
				return false;
			}
		}

		if (!mesh_.is_boundary(e))
		{
			if (pmp::angle(mesh_.position(v2) - mesh_.position(v0), mesh_.position(v3) - mesh_.position(v0)) >= M_PI)
			{
				return false;
			}

			if (pmp::angle(mesh_.position(v2) - mesh_.position(v1), mesh_.position(v3) - mesh_.position(v1)) >= M_PI)
			{
				return false;
			}
		}

		return true;
	}

	// Getters for curvature
	inline Scalar max_abs_curvature(Face f) const
	{
		return fmax_abs_curvature_[f];
	}

	Scalar max_abs_curvature(Vertex v) const
	{
		Scalar c = 0;

		for (auto f : mesh_.faces(v))
		{
			c = std::max(c, max_abs_curvature(f));
		}

		return c;
	}

	// Getters for the normals
	const Normal& normal(Face f) const
	{
		return fnormal_[f];
	}

	Normal normal(Face f)
	{
		return fnormal_[f];
	}

	std::pair<Point, Point> tangents(const Normal& normal, const Point& point) const
	{
		auto du = camera_.du();
		auto dv = camera_.dv();
		auto dz = camera_.dz(point[0], point[1]);

		Scalar n3 = std::min(dot(dz, normal), -cos_max_angle);

		pmp::Point dx_du = du - (dot(du, normal) / n3) * dz;
		pmp::Point dx_dv = dv - (dot(dv, normal) / n3) * dz;

		return std::make_pair(dx_du, dx_dv);
	}

	std::pair<Point, Point> tangents(Face f) const
	{
		return tangents(fnormal_[f], centroid(mesh_, f));
	}
};
}
