// This piece of code was adapted from the pmp-library https://github.com/pmp-library/pmp-library
// Visit https://github.com/pmp-library/pmp-library/blob/main/src/pmp/algorithms/remeshing.h for the current version of the original code
#pragma once

#include "ScreenDifferentialGeometry.h"

#include <memory>
namespace pmp 
{

//! \brief A class for uniform and adaptive surface remeshing.
//! \details The algorithm implemented here performs incremental remeshing based
//! on edge collapse, split, flip, and tangential relaxation.
//! See \cite botsch_2004_remeshing and \cite dunyach_2013_adaptive for a more
//! detailed description.
//! \ingroup algorithms
template <typename Projection = Orthographic>
class ScreenRemeshing
{
public:
	//! \brief Construct with mesh to be remeshed.
	//! \pre Input mesh needs to be a pure triangle mesh.
	//! \throw InvalidInputException if the input precondition is violated.
	ScreenRemeshing(SurfaceMesh& mesh, const Grid<Eigen::Vector3f>& normals, const Grid<unsigned char>& mask = Grid<unsigned char>(), Projection projection = Projection());

	//! \brief Perform uniform remeshing.
	//! \param edge_length the target edge length.
	//! \param iterations the number of iterations
	//! \param use_projection use back-projection to the input surface
	void uniform_remeshing(Scalar edge_length, unsigned int iterations = 10, bool delaunay = true);

	//! \brief Perform adaptive remeshing.
	//! \param min_edge_length the minimum edge length.
	//! \param max_edge_length the maximum edge length.
	//! \param approx_error the maximum approximation error
	//! \param iterations the number of iterations
	//! \param use_projection use back-projection to the input surface
	void adaptive_remeshing(Scalar min_edge_length, Scalar max_edge_length, Scalar approx_error, unsigned int iterations = 10, bool delaunay = true);

private:
	void preprocessing();
	void postprocessing();

	void split_long_edges();
	void collapse_short_edges();
	void valence_flip_edges();
	void delaunay_flip_edges();
	void tangential_smoothing(unsigned int iterations);
	void remove_caps();

	Point minimize_squared_areas(Vertex v);
	Point weighted_centroid(Vertex v);

	bool is_too_long(Edge e) const
	{
		return geo_.length(e) >
				4.0 / 3.0 * std::min(vsizing_[mesh_.vertex(e, 0)], vsizing_[mesh_.vertex(e, 1)]);
	}
	bool is_too_short(Edge e) const
	{
		return geo_.length(e) <
				4.0 / 5.0 * std::min(vsizing_[mesh_.vertex(e, 0)], vsizing_[mesh_.vertex(e, 1)]);
	}

	SurfaceMesh& mesh_;
	ScreenDifferentialGeometry<Projection> geo_;

	bool uniform_;
	Scalar target_edge_length_;
	Scalar min_edge_length_;
	Scalar max_edge_length_;
	Scalar approx_error_;
	Scalar alpha_ = 0.5;

	bool has_feature_vertices_{false};
	bool has_feature_edges_{false};
	VertexProperty<Point> points_;
	VertexProperty<bool> vfeature_;
	EdgeProperty<bool> efeature_;
	VertexProperty<bool> vlocked_;
	EdgeProperty<bool> elocked_;
	VertexProperty<Scalar> vsizing_;
	FaceProperty<Matrix<Scalar, 2, 2>> fmetric_;
};
} // namespace pmp
