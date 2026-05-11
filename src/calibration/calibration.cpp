#include "calibration.h"

#include <cmath>
#include <fstream>
#include <sstream>

/* -----------------------------------------------------------------------
 * GainMap::evaluate
 * Evaluate the 2D polynomial at normalised coordinates (nx, ny) ∈ [-1,1]².
 * Coefficients are stored in graded lexicographic order up to the chosen
 * polynomial order:  1, x, y, x², xy, y², x³, …
 * --------------------------------------------------------------------- */
double GainMap::evaluate(double nx, double ny) const {
	if (coeffs.empty())
		return 1.0;

	double result = 0.0;
	int idx = 0;
	for (int deg = 0; deg <= order; ++deg) {
		for (int i = deg; i >= 0; --i) {
			int j = deg - i;
			if (idx >= (int)coeffs.size()) break;
			double term = coeffs[idx++];
			for (int p = 0; p < i; ++p) term *= nx;
			for (int q = 0; q < j; ++q) term *= ny;
			result += term;
		}
	}
	return result;
}

/* -----------------------------------------------------------------------
 * LensCalibration  –  JSON I/O  (stubs, to be filled with real parser)
 * --------------------------------------------------------------------- */
bool LensCalibration::loadJson(const std::string &/*path*/) {
	// TODO: parse JSON and populate fx, fy, cx, cy, k1–k3, p1–p2
	return false;
}

bool LensCalibration::saveJson(const std::string &/*path*/) const {
	// TODO: serialise coefficients to JSON
	return false;
}

/* -----------------------------------------------------------------------
 * FlatfieldCalibration  –  JSON I/O stubs
 * --------------------------------------------------------------------- */
bool FlatfieldCalibration::loadJson(const std::string &/*path*/) {
	// TODO: load per-LED GainMap polynomial coefficients from JSON
	return false;
}

bool FlatfieldCalibration::saveJson(const std::string &/*path*/) const {
	// TODO: write per-LED GainMap polynomial coefficients to JSON
	return false;
}

/* -----------------------------------------------------------------------
 * LightCalibration  –  LP + JSON I/O stubs
 * --------------------------------------------------------------------- */
bool LightCalibration::loadLP(const std::string &path) {
	std::ifstream in(path);
	if (!in.is_open())
		return false;

	lights.clear();
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || line[0] == '#')
			continue;
		std::istringstream ss(line);
		std::string name;
		LightDirection d;
		if (ss >> name >> d.x >> d.y >> d.z) {
			double len = std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
			if (len > 0.0) { d.x /= len; d.y /= len; d.z /= len; }
			lights.push_back(d);
		}
	}
	return !lights.empty();
}

bool LightCalibration::saveLP(const std::string &path) const {
	std::ofstream out(path);
	if (!out.is_open())
		return false;

	for (int i = 0; i < (int)lights.size(); ++i) {
		const auto &d = lights[i];
		out << "light_" << i << " " << d.x << " " << d.y << " " << d.z << "\n";
	}
	return true;
}

bool LightCalibration::loadJson(const std::string &/*path*/) {
	// TODO: parse JSON with directions + dome_radius_mm + height_offset_mm
	return false;
}

bool LightCalibration::saveJson(const std::string &/*path*/) const {
	// TODO: serialise directions + geometry to JSON
	return false;
}

/* -----------------------------------------------------------------------
 * DomeCalibration  –  bundle JSON I/O stubs
 * --------------------------------------------------------------------- */
bool DomeCalibration::loadJson(const std::string &/*path*/) {
	// TODO: parse top-level JSON that contains sections for
	//       lens, flatfield and lights
	return false;
}

bool DomeCalibration::saveJson(const std::string &/*path*/) const {
	// TODO: write top-level JSON bundle
	return false;
}
