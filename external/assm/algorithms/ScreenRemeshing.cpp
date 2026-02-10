// This piece of code was adapted from the pmp-library https://github.com/pmp-library/pmp-library
// Visit https://github.com/pmp-library/pmp-library/blob/main/src/pmp/algorithms/remeshing.cpp for the current version of the original code

#include "ScreenRemeshing.h"


#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "DifferentialGeometry.h"
#include <functional>
#include <QString>

namespace pmp 
{
template <typename Projection>
ScreenRemeshing<Projection>::ScreenRemeshing(SurfaceMesh& mesh, const Grid<Eigen::Vector3f>& normals, const Grid<unsigned char>& mask, Projection projection) :
	mesh_(mesh), geo_(mesh_, normals, mask, projection)
{
	if (!mesh_.is_triangle_mesh())
		throw InvalidInputException("Input is not a pure triangle mesh!");

	points_ = mesh_.vertex_property<Point>("v:point");

	geo_.update();

	has_feature_vertices_ = mesh_.has_vertex_property("v:feature");
	has_feature_edges_ = mesh_.has_edge_property("e:feature");
}

template <typename Projection>
void ScreenRemeshing<Projection>::uniform_remeshing(Scalar edge_length, unsigned int iterations, bool delaunay)
{
	uniform_ = true;
	target_edge_length_ = edge_length;

	preprocessing();

	for (unsigned int i = 0; i < iterations; ++i)
	{
		split_long_edges();

		collapse_short_edges();

		if (delaunay)
			delaunay_flip_edges();
		else
			valence_flip_edges();

		geo_.update();
		tangential_smoothing(5);

		geo_.update();
	}

	remove_caps();

	postprocessing();
}

template <typename Projection>
void ScreenRemeshing<Projection>::adaptive_remeshing(Scalar min_edge_length, Scalar max_edge_length, Scalar approx_error, unsigned int iterations, bool delaunay)
{
	uniform_ = false;
	min_edge_length_ = min_edge_length;
	max_edge_length_ = max_edge_length;
	approx_error_ = approx_error;

	preprocessing();

	for (unsigned int i = 0; i < iterations; ++i)
	{
		split_long_edges();
		// std::cout << "After Split: " << mesh_.n_vertices() << " Vertices\n";

		geo_.update();

		collapse_short_edges();
		// std::cout << "After Collapse: " << mesh_.n_vertices() << " Vertices\n";

		if (delaunay)
			delaunay_flip_edges();
		else
			valence_flip_edges();

		geo_.update();
		tangential_smoothing(5);

		geo_.update();
	}

	remove_caps();

	postprocessing();
}

template <typename Projection>
void ScreenRemeshing<Projection>::preprocessing()
{
	// properties
	vfeature_ = mesh_.vertex_property<bool>("v:feature", false);
	efeature_ = mesh_.edge_property<bool>("e:feature", false);
	vlocked_ = mesh_.add_vertex_property<bool>("v:locked", false);
	elocked_ = mesh_.add_edge_property<bool>("e:locked", false);
	vsizing_ = mesh_.add_vertex_property<Scalar>("v:sizing");
	fmetric_ = mesh_.get_face_property<Matrix<Scalar, 2, 2>>("f:metric");

	// lock unselected vertices if some vertices are selected
	auto vselected = mesh_.get_vertex_property<bool>("v:selected");
	if (vselected)
	{
		bool has_selection = false;
		for (auto v : mesh_.vertices())
		{
			if (vselected[v])
			{
				has_selection = true;
				break;
			}
		}

		if (has_selection)
		{
			for (auto v : mesh_.vertices())
			{
				vlocked_[v] = !vselected[v];
			}

			// lock an edge if one of its vertices is locked
			for (auto e : mesh_.edges())
			{
				elocked_[e] = (vlocked_[mesh_.vertex(e, 0)] ||
						vlocked_[mesh_.vertex(e, 1)]);
			}
		}
	}

	// lock feature corners
	for (auto v : mesh_.vertices())
	{
		if (vfeature_[v])
		{
			int c = 0;
			for (auto h : mesh_.halfedges(v))
				if (efeature_[mesh_.edge(h)])
					++c;

			if (c != 2)
				vlocked_[v] = true;
		}
	}

	// compute sizing field
	if (uniform_)
	{
		for (auto v : mesh_.vertices())
		{
			vsizing_[v] = target_edge_length_;
		}
	}
	else
	{
		geo_.update();

		// use vsizing_ to store/smooth curvatures to avoid another vertex property

		// curvature values for feature vertices and boundary vertices
		// are not meaningful. mark them as negative values.
		for (auto v : mesh_.vertices())
		{
			if (mesh_.is_boundary(v) || (vfeature_ && vfeature_[v]))
				vsizing_[v] = -1.0;
			else
				vsizing_[v] = geo_.max_abs_curvature(v);
		}

		// curvature values might be noisy. smooth them.
		// don't consider feature vertices' curvatures.
		// don't consider boundary vertices' curvatures.
		// do this for two iterations, to propagate curvatures
		// from non-feature regions to feature vertices.
		for (int iters = 0; iters < 2; ++iters)
		{
			for (auto v : mesh_.vertices())
			{
				Scalar w, ww = 0.0;
				Scalar c, cc = 0.0;

				for (auto h : mesh_.halfedges(v))
				{
					c = vsizing_[mesh_.to_vertex(h)];
					if (c >= 0.0)
					{
						w = std::max<Scalar>(0.0, geo_.cotan_weight(mesh_.edge(h)));
						ww += w;
						cc += w * c;
					}
				}

				if (ww)
					cc /= ww;
				vsizing_[v] = cc;
			}
		}

		// now convert per-vertex curvature into target edge length
		for (auto v : mesh_.vertices())
		{
			Scalar c = vsizing_[v];

			// Check for division by zero
			if (c > std::numeric_limits<Scalar>::min())
			{
				// get edge length from curvature
				const Scalar r = 1.0 / c;
				const Scalar e = approx_error_;
				Scalar h;
				if (e < r)
				{
					// see mathworld: "circle segment" and "equilateral triangle"
					//h = sqrt(2.0*r*e-e*e) * 3.0 / sqrt(3.0);
					h = sqrt(6.0 * e * r - 3.0 * e * e); // simplified...
				}
				else
				{
					// this does not really make sense
					h = e * 3.0 / sqrt(3.0);
				}

				// clamp to min. and max. edge length
				if (h < min_edge_length_)
					h = min_edge_length_;
				else if (h > max_edge_length_)
					h = max_edge_length_;

				// store target edge length
				vsizing_[v] = h;
			}
			else
			{
				vsizing_[v] = max_edge_length_;
			}
		}
	}
}

template <typename Projection>
void ScreenRemeshing<Projection>::postprocessing()
{
	// remove properties
	mesh_.remove_vertex_property(vlocked_);
	mesh_.remove_edge_property(elocked_);
	mesh_.remove_vertex_property(vsizing_);

	if (!has_feature_vertices_)
	{
		mesh_.remove_vertex_property(vfeature_);
	}
	if (!has_feature_edges_)
	{
		mesh_.remove_edge_property(efeature_);
	}
}

template <typename Projection>
void ScreenRemeshing<Projection>::split_long_edges()
{
	Vertex vnew, v0, v1;
	Halfedge h0, h1;
	Edge enew; //, e0, e1;
	Face f0, f1; //, f2, f3;
	bool ok, is_feature, is_boundary;
	int i;

	for (ok = false, i = 0; !ok && i < 10; ++i)
	{
		ok = true;

		for (auto e : mesh_.edges())
		{
			v0 = mesh_.vertex(e, 0);
			v1 = mesh_.vertex(e, 1);

			if (!elocked_[e] && is_too_long(e))
			{
				const Point& p0 = points_[v0];
				const Point& p1 = points_[v1];

				// Get faces on both sides of the edge
				f0 = mesh_.face(e, 0);
				f1 = mesh_.face(e, 1);

				is_feature = efeature_[e];
				is_boundary = mesh_.is_boundary(e);

				vnew = mesh_.add_vertex((p0 + p1) * 0.5f);
				auto h0 = mesh_.split(e, vnew);
				h1 = mesh_.opposite_halfedge(h0);

				// need normal or sizing for adaptive refinement
				vsizing_[vnew] = 0.5f * (vsizing_[v0] + vsizing_[v1]);

				// To keep track of edge lengths, we need to update the metric tensor on the (potentially two) newly created faces
				if (f0.is_valid())
				{
					fmetric_[mesh_.face(h0)] = fmetric_[f0];
				}

				if (f1.is_valid())
				{
					fmetric_[mesh_.face(h1)] = fmetric_[f1];
				}

				if (is_feature)
				{
					enew = is_boundary ? Edge(mesh_.n_edges() - 2)
									   : Edge(mesh_.n_edges() - 3);
					efeature_[enew] = true;
					vfeature_[vnew] = true;
				}

				ok = false;
			}
		}
	}
}

template <typename Projection>
void ScreenRemeshing<Projection>::collapse_short_edges()
{
	Vertex v0, v1;
	Halfedge h0, h1, h01, h10;
	bool ok, b0, b1, l0, l1, f0, f1;
	int i;
	bool hcol01, hcol10;

	for (ok = false, i = 0; !ok && i < 10; ++i)
	{
		ok = true;

		for (auto e : mesh_.edges())
		{
			if (!mesh_.is_deleted(e) && !elocked_[e])
			{
				h10 = mesh_.halfedge(e, 0);
				h01 = mesh_.halfedge(e, 1);
				v0 = mesh_.to_vertex(h10);
				v1 = mesh_.to_vertex(h01);

				if (is_too_short(e))
				{
					// get status
					b0 = mesh_.is_boundary(v0);
					b1 = mesh_.is_boundary(v1);
					l0 = vlocked_[v0];
					l1 = vlocked_[v1];
					f0 = vfeature_[v0];
					f1 = vfeature_[v1];
					hcol01 = hcol10 = true;

					// boundary rules
					if (b0 && b1)
					{
						if (!mesh_.is_boundary(e))
							continue;
					}
					else if (b0)
						hcol01 = false;
					else if (b1)
						hcol10 = false;

					// locked rules
					if (l0 && l1)
						continue;
					else if (l0)
						hcol01 = false;
					else if (l1)
						hcol10 = false;

					// feature rules
					if (f0 && f1)
					{
						// edge must be feature
						if (!efeature_[e])
							continue;

						// the other two edges removed by collapse must not be features
						h0 = mesh_.prev_halfedge(h01);
						h1 = mesh_.next_halfedge(h10);
						if (efeature_[mesh_.edge(h0)] ||
								efeature_[mesh_.edge(h1)])
							hcol01 = false;
						// the other two edges removed by collapse must not be features
						h0 = mesh_.prev_halfedge(h10);
						h1 = mesh_.next_halfedge(h01);
						if (efeature_[mesh_.edge(h0)] ||
								efeature_[mesh_.edge(h1)])
							hcol10 = false;
					}
					else if (f0)
						hcol01 = false;
					else if (f1)
						hcol10 = false;

					// topological rules
					bool collapse_ok = mesh_.is_collapse_ok(h01);

					if (hcol01)
						hcol01 = collapse_ok;
					if (hcol10)
						hcol10 = collapse_ok;

					// both collapses possible: collapse into vertex w/ higher valence
					if (hcol01 && hcol10)
					{
						if (mesh_.valence(v0) < mesh_.valence(v1))
							hcol10 = false;
						else
							hcol01 = false;
					}

					// try v1 -> v0
					if (hcol10)
					{
						if (hcol10)
						{
							mesh_.collapse(h10);
							ok = false;
						}
					}

					// try v0 -> v1
					else if (hcol01)
					{
						if (hcol01)
						{
							mesh_.collapse(h01);
							ok = false;
						}
					}
				}
			}
		}
	}

	mesh_.garbage_collection();
}

template <typename Projection>
void ScreenRemeshing<Projection>::valence_flip_edges()
{
	Vertex v0, v1, v2, v3;
	Halfedge h;
	int val0, val1, val2, val3;
	int val_opt0, val_opt1, val_opt2, val_opt3;
	int ve0, ve1, ve2, ve3, ve_before, ve_after;
	bool ok;
	int i;

	// precompute valences
	auto valence = mesh_.add_vertex_property<int>("valence");
	for (auto v : mesh_.vertices())
	{
		valence[v] = mesh_.valence(v);
	}

	for (ok = false, i = 0; !ok && i < 10; ++i)
	{
		ok = true;

		for (auto e : mesh_.edges())
		{
			if (!elocked_[e] && !efeature_[e])
			{
				h = mesh_.halfedge(e, 0);
				v0 = mesh_.to_vertex(h);
				v2 = mesh_.to_vertex(mesh_.next_halfedge(h));
				h = mesh_.halfedge(e, 1);
				v1 = mesh_.to_vertex(h);
				v3 = mesh_.to_vertex(mesh_.next_halfedge(h));

				if (!vlocked_[v0] && !vlocked_[v1] && !vlocked_[v2] &&
						!vlocked_[v3])
				{
					val0 = valence[v0];
					val1 = valence[v1];
					val2 = valence[v2];
					val3 = valence[v3];

					val_opt0 = (mesh_.is_boundary(v0) ? 4 : 6);
					val_opt1 = (mesh_.is_boundary(v1) ? 4 : 6);
					val_opt2 = (mesh_.is_boundary(v2) ? 4 : 6);
					val_opt3 = (mesh_.is_boundary(v3) ? 4 : 6);

					ve0 = (val0 - val_opt0);
					ve1 = (val1 - val_opt1);
					ve2 = (val2 - val_opt2);
					ve3 = (val3 - val_opt3);

					ve0 *= ve0;
					ve1 *= ve1;
					ve2 *= ve2;
					ve3 *= ve3;

					ve_before = ve0 + ve1 + ve2 + ve3;

					--val0;
					--val1;
					++val2;
					++val3;

					ve0 = (val0 - val_opt0);
					ve1 = (val1 - val_opt1);
					ve2 = (val2 - val_opt2);
					ve3 = (val3 - val_opt3);

					ve0 *= ve0;
					ve1 *= ve1;
					ve2 *= ve2;
					ve3 *= ve3;

					ve_after = ve0 + ve1 + ve2 + ve3;

					if (ve_before > ve_after && mesh_.is_flip_ok(e))
					{
						mesh_.flip(e);
						--valence[v0];
						--valence[v1];
						++valence[v2];
						++valence[v3];
						ok = false;
					}
				}
			}
		}
	}

	mesh_.remove_vertex_property(valence);
}

template <typename Projection>
void ScreenRemeshing<Projection>::delaunay_flip_edges()
{
	using PriorityEdge = std::pair<Edge, Scalar>;

	Vertex v0, v1, v2, v3;
	Halfedge h0, h1;
	Face f0, f1;

	auto hangle = mesh_.add_halfedge_property<Scalar>("h:angle");

	auto cmp = [](const PriorityEdge& rhs, const PriorityEdge& lhs){ return rhs.first < lhs.first; };
	std::priority_queue<PriorityEdge, std::vector<PriorityEdge>, decltype(cmp)> nld(cmp);

	// Precompute angles
	for (auto h : mesh_.halfedges())
	{
		if (!mesh_.is_boundary(h))
		{
			hangle[h] = geo_.angle(h);
		}
	}

	// Find non-Delaunay edges
	for (auto e : mesh_.edges())
	{
		if (!mesh_.is_boundary(e) && !elocked_[e] && !efeature_[e])
		{
			h0 = mesh_.next_halfedge(mesh_.halfedge(e, 0));
			h1 = mesh_.next_halfedge(mesh_.halfedge(e, 1));

			if (hangle[h0] + hangle[h1] > M_PI)
			{
				nld.emplace(e, hangle[h0] + hangle[h1]);
			}
		}
	}

	for (size_t i = 0; !nld.empty() && i < mesh_.n_edges();) // Basically: Flip each edge at most once.
	{
		auto e = nld.top().first; nld.pop();

		h0 = mesh_.halfedge(e, 0);
		h1 = mesh_.halfedge(e, 1);

		if ((hangle[mesh_.next_halfedge(h0)] + hangle[mesh_.next_halfedge(h1)] > M_PI) && geo_.is_convex(e) && mesh_.is_flip_ok(e))
		{
			++i;
			mesh_.flip(e);

			// Update angles
			f0 = mesh_.face(h0);
			f1 = mesh_.face(h1);

			for (auto h : mesh_.halfedges(f0))
			{
				h0 = mesh_.next_halfedge(h0);
				hangle[h0] = geo_.angle(h0);
			}

			for (auto h : mesh_.halfedges(f1))
			{
				h1 = mesh_.next_halfedge(h1);
				hangle[h1] = geo_.angle(h1);
			}

			// Queue if necessary
			for (auto h : mesh_.halfedges(f0))
			{
				e = mesh_.edge(h);

				if (!mesh_.is_boundary(e) && !elocked_[e] && !efeature_[e])
				{
					h0 = mesh_.next_halfedge(mesh_.halfedge(e, 0));
					h1 = mesh_.next_halfedge(mesh_.halfedge(e, 1));

					if (hangle[h0] + hangle[h1] > M_PI)
					{
						nld.emplace(e, hangle[h0] + hangle[h1]);
					}
				}
			}

			for (auto h : mesh_.halfedges(f1))
			{
				e = mesh_.edge(h);

				if (!mesh_.is_boundary(e) && !elocked_[e] && !efeature_[e])
				{
					h0 = mesh_.next_halfedge(mesh_.halfedge(e, 0));
					h1 = mesh_.next_halfedge(mesh_.halfedge(e, 1));

					if (hangle[h0] + hangle[h1] > M_PI)
					{
						nld.emplace(e, hangle[h0] + hangle[h1]);
					}
				}
			}
		}
	}

	geo_.update();
	mesh_.remove_halfedge_property(hangle);
}

template <typename Projection>
void ScreenRemeshing<Projection>::tangential_smoothing(unsigned int iterations)
{
	Vertex v1, v2, v3, vv;
	Edge e;
	Scalar w, ww;
	Point u, n, t, b;

	// add property
	auto update = mesh_.add_vertex_property<Point>("v:update");

	for (unsigned int iters = 0; iters < iterations; ++iters)
	{
#pragma omp parallel for private(v1, v2, v3, vv, e, w, ww, u, n, t, b) shared(update)
		for (int i = 0; i < int(mesh_.n_vertices()); ++i)
		{
			Vertex v(i);

			if (!mesh_.is_boundary(v) && !vlocked_[v])
			{
				if (vfeature_[v])
				{
					u = Point(0.0);
					t = Point(0.0);
					ww = 0;
					int c = 0;

					for (auto h : mesh_.halfedges(v))
					{
						if (efeature_[mesh_.edge(h)])
						{
							vv = mesh_.to_vertex(h);

							b = points_[v];
							b += points_[vv];
							b *= 0.5;

							w = geo_.length(mesh_.edge(h)) /
									(0.5 * (vsizing_[v] + vsizing_[vv]));
							ww += w;
							u += w * b;

							if (c == 0)
							{
								t += normalize(points_[vv] - points_[v]);
								++c;
							}
							else
							{
								++c;
								t -= normalize(points_[vv] - points_[v]);
							}
						}
					}

					assert(c == 2);

					u *= (1.0 / ww);
					u -= points_[v];
					t = normalize(t);
					u = t * dot(u, t);

					update[v] = u;
				}
				else
				{
					Point p(0);
					// p = weighted_centroid(v);
					// u = p - mesh_.position(v);
					// update[v] = u;
					update[v] = weighted_centroid(v);
				}
			}
		}

		// update vertex positions
#pragma omp parallel for private(v1, v2, v3, vv, e, w, ww, u, n, t, b) shared(update)
		for (int i = 0; i < int(mesh_.n_vertices()); ++i)
		{
			Vertex v(i);
			if (!mesh_.is_boundary(v) && !vlocked_[v])
			{
				points_[v] = (1. - alpha_) * points_[v] + alpha_ * update[v];
			}
		}

		geo_.update();
	}

	// remove property
	mesh_.remove_vertex_property(update);
}

template <typename Projection>
void ScreenRemeshing<Projection>::remove_caps()
{
	Halfedge h;
	Vertex v, vb, vd;
	Face fb, fd;
	Scalar a0, a1, amin, aa(::cos(170.0 * M_PI / 180.0));
	Point a, b, c, d;

	for (auto e : mesh_.edges())
	{
		if (!elocked_[e] && mesh_.is_flip_ok(e))
		{
			h = mesh_.halfedge(e, 0);
			a = points_[mesh_.to_vertex(h)];

			h = mesh_.next_halfedge(h);
			b = points_[vb = mesh_.to_vertex(h)];

					h = mesh_.halfedge(e, 1);
					c = points_[mesh_.to_vertex(h)];

					h = mesh_.next_halfedge(h);
					d = points_[vd = mesh_.to_vertex(h)];

					a0 = dot(normalize(a - b), normalize(c - b));
					a1 = dot(normalize(a - d), normalize(c - d));

					if (a0 < a1)
			{
				amin = a0;
				v = vb;
			}
			else
			{
			amin = a1;
			v = vd;
		}

			// is it a cap?
			if (amin < aa)
			{
			// feature edge and feature vertex -> seems to be intended
			if (efeature_[e] && vfeature_[v])
			continue;

			// project v onto feature edge
			if (efeature_[e])
			points_[v] = (a + c) * 0.5f;

			// flip
			mesh_.flip(e);
		}
		}
	}
}

template <typename Projection>
Point ScreenRemeshing<Projection>::minimize_squared_areas(Vertex v)
{
	dmat3 A(0);
	dvec3 b(0), x;

	for (auto h : mesh_.halfedges(v))
	{
		assert(!mesh_.is_boundary(h));

		// get edge opposite to vertex v
		auto v0 = mesh_.to_vertex(h);
		auto v1 = mesh_.to_vertex(mesh_.next_halfedge(h));
		auto p = (dvec3)points_[v0];
		auto q = (dvec3)points_[v1];
		auto d = q - p;
		auto w = 1.0 / norm(d);

		// build squared cross-product-with-d matrix
		dmat3 D;
		D(0, 0) = d[1] * d[1] + d[2] * d[2];
		D(1, 1) = d[0] * d[0] + d[2] * d[2];
		D(2, 2) = d[0] * d[0] + d[1] * d[1];
		D(1, 0) = D(0, 1) = -d[0] * d[1];
		D(2, 0) = D(0, 2) = -d[0] * d[2];
		D(1, 2) = D(2, 1) = -d[1] * d[2];
		A += w * D;

		// build right-hand side
		b += w * D * p;
	}

	// compute minimizer
	try
	{
		x = inverse(A) * b;
	}
	catch (...)
	{
		auto what = "Remeshing: Matrix not invertible.";
		throw SolverException(what);
	}

	return Point(x);
}

template <typename Projection>
Point ScreenRemeshing<Projection>::weighted_centroid(Vertex v)
{
	Vector<float, 2> p(mesh_.position(v)[0], mesh_.position(v)[1]);
	Matrix<float, 2, 2> ww(0);
	Matrix<float, 2, 2> w;

	// Avoid inversion of a 0 matrix
	ww(0, 0) = 1;
	ww(1, 1) = 1;

	for (auto h : mesh_.halfedges(v))
	{
		auto v1 = v;
		auto v2 = mesh_.to_vertex(h);
		auto v3 = mesh_.to_vertex(mesh_.next_halfedge(h));
		auto f = mesh_.face(h);

		auto b = points_[v1];
		b += points_[v2];
		b += points_[v3];
		b *= (1.0 / 3.0);

		// d = Vector<Scalar, 2>(p[0] - b[0], p[1] - b[1]);

		float area = geo_.area(f);
		// norm(cross(points_[v2] - points_[v1], points_[v3] - points_[v1]));

		// take care of degenerate faces to avoid all zero weights and division
		// by zero later on
		if (area == 0)
			area = 1.0;

		w = area / pow((vsizing_[v1] + vsizing_[v2] + vsizing_[v3]) / 3.0, 2.0) * fmetric_[f];

		p += w * Vector<float, 2>(b[0], b[1]);
		ww += w;
	}

	float det = ww(0, 0) * ww(1, 1) - ww(0, 1) * ww(1, 0);

	// Invert ww
	ww = ww / det;
	std::swap(ww(0, 0), ww(1, 1));
	ww(1, 0) *= -1.;
	ww(0, 1) *= -1.;

	p = ww * p;

	return Point(p[0], p[1], mesh_.position(v)[2]);
}

template class ScreenRemeshing<Orthographic>;
template class ScreenRemeshing<Perspective>;
} // namespace pmp
