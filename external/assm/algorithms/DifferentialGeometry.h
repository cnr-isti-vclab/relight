// Copyright 2011-2020 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include "../Types.h"
#include "../SurfaceMesh.h"

namespace pmp {

//! \addtogroup algorithms
//! @{

//! clamp cotangent values as if angles are in [3, 177]
inline Scalar clamp_cot(const Scalar v)
{
	const auto bound = Scalar(19.1); // 3 degrees
	return (v < -bound ? -bound : (v > bound ? bound : v));
}

//! clamp cosine values as if angles are in [3, 177]
inline Scalar clamp_cos(const Scalar v)
{
	const auto bound = Scalar(0.9986); // 3 degrees
	return (v < -bound ? -bound : (v > bound ? bound : v));
}

//! compute angle between two (un-normalized) vectors
inline Scalar angle(const Point& v0, const Point& v1)
{
	return atan2(norm(cross(v0, v1)), dot(v0, v1));
}

//! compute sine of angle between two (un-normalized) vectors
inline Scalar sin(const Point& v0, const Point& v1)
{
	return norm(cross(v0, v1)) / (norm(v0) * norm(v1));
}

//! compute cosine of angle between two (un-normalized) vectors
inline Scalar cos(const Point& v0, const Point& v1)
{
	return dot(v0, v1) / (norm(v0) * norm(v1));
}

//! compute cotangent of angle between two (un-normalized) vectors
inline Scalar cotan(const Point& v0, const Point& v1)
{
	return clamp_cot(dot(v0, v1) / norm(cross(v0, v1)));
}

//! compute area of a triangle given by three points
Scalar triangle_area(const Point& p0, const Point& p1, const Point& p2);

//! compute area of triangle f
Scalar triangle_area(const SurfaceMesh& mesh, Face f);

//! Compute the surface area of \p mesh
//! \pre Input \p mesh needs to be a pure triangle mesh.
Scalar surface_area(const SurfaceMesh& mesh);

//! \brief Compute the volume of a mesh
//! \details See \cite zhang_2002_efficient for details.
//! \pre Input mesh needs to be a pure triangle mesh.
//! \throw InvalidInputException if the input precondition is violated.
Scalar volume(const SurfaceMesh& mesh);

//! barycenter/centroid of a face
Point centroid(const SurfaceMesh& mesh, Face f);

//! barycenter/centroid of mesh, computed as area-weighted mean of vertices.
//! assumes triangular faces.
Point centroid(const SurfaceMesh& mesh);

//! \brief Compute dual of a mesh.
//! \warning Changes the mesh in place. All properties are cleared.
void dual(SurfaceMesh& mesh);

//! compute the cotangent weight for edge e
double cotan_weight(const SurfaceMesh& mesh, Edge e);

//! compute (mixed) Voronoi area of vertex v
double voronoi_area(const SurfaceMesh& mesh, Vertex v);

//! compute barycentric Voronoi area of vertex v
double voronoi_area_barycentric(const SurfaceMesh& mesh, Vertex v);

//! compute Laplace vector for vertex v (normalized by Voronoi area)
Point laplace(const SurfaceMesh& mesh, Vertex v);

//! compute the sum of angles around vertex v (used for Gaussian curvature)
Scalar angle_sum(const SurfaceMesh& mesh, Vertex v);

//! discrete curvature information for a vertex. used for vertex_curvature()
struct VertexCurvature
{
	VertexCurvature() = default;

	Scalar mean{0.0};
	Scalar gauss{0.0};
	Scalar max{0.0};
	Scalar min{0.0};
};

//! compute min, max, mean, and Gaussian curvature for vertex v. this will not
//! give reliable values for boundary vertices.
VertexCurvature vertex_curvature(const SurfaceMesh& mesh, Vertex v);

//! @}

} // namespace pmp
