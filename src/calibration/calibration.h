#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <vector>
#include <string>
#include <array>

/* -----------------------------------------------------------------------
 * LensCalibration
 * Brown-Conrady radial + tangential distortion model.
 * Intrinsic camera matrix: fx, fy (focal lengths), cx, cy (principal point).
 * --------------------------------------------------------------------- */
struct LensCalibration {
	// intrinsic matrix
	double fx = 0.0, fy = 0.0;
	double cx = 0.0, cy = 0.0;

	// distortion coefficients
	double k1 = 0.0, k2 = 0.0, k3 = 0.0;  // radial
	double p1 = 0.0, p2 = 0.0;             // tangential

	bool isValid() const { return fx > 0.0 && fy > 0.0; }
	void reset() { *this = LensCalibration{}; }

	bool loadJson(const std::string &path);
	bool saveJson(const std::string &path) const;
};


/* -----------------------------------------------------------------------
 * GainMap
 * Per-LED vignetting + illumination cone correction map.
 * Stored as coefficients of a 2D polynomial (order 2, 4, or 6) fitted
 * over normalised image coordinates [-1, +1]².
 * --------------------------------------------------------------------- */
struct GainMap {
	int     order      = 4;     // polynomial order (2 / 4 / 6)
	int     led_index  = -1;    // which LED this map belongs to
	std::vector<double> coeffs; // polynomial coefficients (row-major)

	bool isValid() const { return !coeffs.empty() && led_index >= 0; }
	double evaluate(double nx, double ny) const; // evaluate at normalised coords
};


/* -----------------------------------------------------------------------
 * FlatfieldCalibration
 * Collection of per-LED gain maps.
 * --------------------------------------------------------------------- */
struct FlatfieldCalibration {
	std::vector<GainMap> maps;      // one entry per LED
	double global_max = 1.0;        // luminance normalisation factor

	bool isValid() const { return !maps.empty(); }
	void reset() { maps.clear(); global_max = 1.0; }

	bool loadJson(const std::string &path);
	bool saveJson(const std::string &path) const;
};


/* -----------------------------------------------------------------------
 * LightDirection
 * Unit vector for a single LED.
 * --------------------------------------------------------------------- */
struct LightDirection {
	double x = 0.0, y = 0.0, z = 1.0;  // unit vector (z up = overhead)
};


/* -----------------------------------------------------------------------
 * LightCalibration
 * Full set of LED directions plus dome geometry parameters.
 * --------------------------------------------------------------------- */
struct LightCalibration {
	std::vector<LightDirection> lights;   // one per LED, same order as images
	double dome_radius_mm = 300.0;        // distance from centre to LED ring
	double height_offset_mm = 0.0;        // vertical offset of object plane

	bool isValid() const { return !lights.empty(); }
	void reset() { lights.clear(); }

	bool loadLP(const std::string &path);
	bool saveLP(const std::string &path) const;
	bool loadJson(const std::string &path);
	bool saveJson(const std::string &path) const;
};


/* -----------------------------------------------------------------------
 * DomeCalibration
 * Aggregates all three calibration components together with the list of
 * source image paths and the white-point threshold.
 * --------------------------------------------------------------------- */
struct DomeCalibration {
	std::vector<std::string> image_paths;

	double white_point_percentile = 99.0; // images above this are clamped

	LensCalibration     lens;
	FlatfieldCalibration flatfield;
	LightCalibration    lights;

	// Save / load the entire calibration bundle as a JSON file.
	bool loadJson(const std::string &path);
	bool saveJson(const std::string &path) const;
};

#endif // CALIBRATION_H
