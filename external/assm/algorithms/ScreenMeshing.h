#pragma once

#include <queue>
#include "DifferentialGeometry.h"

namespace pmp
{
enum class ScreenMeshing { PixelToQuad, PixelToVertex };

template <typename Indicator>
class ScreenMesher
{
private:
	SurfaceMesh& mesh_;
	Scalar z_ = 1.;

public:
	ScreenMesher(SurfaceMesh& mesh) : mesh_(mesh)
	{

	}


	void run(Indicator indicator, int height, int width, ScreenMeshing strategy = ScreenMeshing::PixelToQuad)
	{
		if (strategy == ScreenMeshing::PixelToQuad)
		{
			run_pixel_to_quad(indicator, height, width);
		}
		else if (strategy == ScreenMeshing::PixelToVertex)
		{
			run_pixel_to_vertex(indicator, height, width);
		}
	}

	void run_pixel_to_vertex(Indicator indicator, int height, int width)
	{
		// Add Vertices
		for (int v = 0; v != height; ++v)
		{
			for (int u = 0; u != width; ++u)
			{
				mesh_.add_vertex(Point(u, v, z_));
			}
		}

		// Add faces
		for (int v = 0; v != height - 1; ++v)
		{
			for (int u = 0; u != width - 1; ++u)
			{
				mesh_.add_quad(Vertex(v * width + u), Vertex(v * width + u + 1), Vertex((v + 1) * width + u + 1), Vertex((v + 1) * width + u));
			}
		}

		// Remove superfluous vertices
		for (int v = 0; v != height; ++v)
		{
			for (int u = 0; u != width; ++u)
			{
				if (!indicator(v, u))
				{
					mesh_.delete_vertex(Vertex(v * width + u));
				}
			}
		}

		mesh_.garbage_collection();
	}

	void run_pixel_to_quad(Indicator indicator, int height, int width)
	{
		constexpr int du[] { 0, -1, 1, 0 };
		constexpr int dv[] { -1, 0, 0, 1 };

		std::vector<Vertex> vertices((height - 1) * (width - 1), Vertex());
		std::vector<Face> faces(height * width, Face());

		// Add the appropriate number of vertices for each face
		for (int v = 0; v != height - 1; ++v)
		{
			for (int u = 0; u != width - 1; ++u)
			{
				if (indicator(v, u) || indicator(v, u + 1) || indicator(v + 1, u) || indicator(v + 1, u + 1))
				{
					// Avoid the following configuration that can lead to topology errors.
					// 0 1
					// 1 0
					if ((indicator(v, u) == indicator(v + 1, u + 1)) && (indicator(v + 1, u) == indicator(v, u + 1)) && (indicator(v, u) != indicator(v, u + 1)))
					{
						// Skip as it would cause a topology error
					}
					else
					{
						vertices[(width - 1) * v + u] = mesh_.add_vertex(pmp::Point(static_cast<Scalar>(u) + 0.5, static_cast<Scalar>(v) + 0.5, z_));
					}
				}
			}
		}

		Vertex tl, tr, bl, br;
		std::queue<std::pair<int, int>> unresolved;

		for (int V = 1; V != height - 1; ++V)
		{
			for (int U = 1; U != width - 1; ++U)
			{
				if (indicator(V, U) && !faces[width * V + U].is_valid())
				{
					unresolved.emplace(V, U);

					while(!unresolved.empty())
					{
						int v = unresolved.front().first;
						int u = unresolved.front().second;
						unresolved.pop();

						if (!faces[width * v + u].is_valid())
						{
							tl = vertices[(width - 1) * (v - 1) + u - 1];
							tr = vertices[(width - 1) * (v - 1) + u];
							br = vertices[(width - 1) * v + u];
							bl = vertices[(width - 1) * v + u - 1];

							if (mesh_.is_valid(tl) && mesh_.is_valid(tr) && mesh_.is_valid(bl) && mesh_.is_valid(br))
							{
								// std::cout << mesh_.add_quad(tl, tr, br, bl) << "\n";
								faces[width * v + u] = mesh_.add_quad(tl, tr, br, bl);
								for (int i = 0; i != 4; ++i)
								{
									if (indicator(v + dv[i], u + du[i]) && !faces[width * (v + dv[i]) + (u + du[i])].is_valid())
									{
										unresolved.emplace(v + dv[i], u + du[i]);
									}
								}
							}
						}
					}
				}
			}
		}

		// Remove disconnected vertices
		for (int i = 0; i != mesh_.n_vertices(); ++i)
		{
			Vertex v(i);
			if (!mesh_.is_deleted(v) && mesh_.valence(v) == 0)
			{
				mesh_.delete_vertex(v);
			}
		}

		mesh_.garbage_collection();
	}
};

template <typename Indicator>
static SurfaceMesh from_indicator(Indicator indicator, int height, int width, ScreenMeshing strategy = ScreenMeshing::PixelToQuad)
{
	SurfaceMesh mesh;
	ScreenMesher<Indicator>mesher(mesh);
	mesher.run(indicator, height, width, strategy);

	return mesh;
}
}
