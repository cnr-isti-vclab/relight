#include "calibration.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include <lensfun/lensfun.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

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
 * LensCalibration  –  JSON I/O and undistortion
 * --------------------------------------------------------------------- */
static const char *modelToString(LensCalibration::Model m) {
	switch (m) {
	case LensCalibration::Model::BrownConrady: return "BrownConrady";
	case LensCalibration::Model::Poly3:        return "Poly3";
	case LensCalibration::Model::Poly5:        return "Poly5";
	case LensCalibration::Model::PTLens:       return "PTLens";
	default:                                   return "None";
	}
}

static LensCalibration::Model modelFromString(const QString &s) {
	if (s == "BrownConrady") return LensCalibration::Model::BrownConrady;
	if (s == "Poly3")        return LensCalibration::Model::Poly3;
	if (s == "Poly5")        return LensCalibration::Model::Poly5;
	if (s == "PTLens")       return LensCalibration::Model::PTLens;
	return LensCalibration::Model::None;
}

bool LensCalibration::saveJson(const std::string &path) const {
	QJsonObject o;
	o["model"]    = modelToString(model);
	o["focal_mm"] = focal_mm;
	o["k1"] = k1;
	o["k2"] = k2;
	o["k3"] = k3;
	o["p1"] = p1;
	o["p2"] = p2;
	// Intrinsics — meaningful for BrownConrady; stored as 0 for Lensfun models
	o["fx"] = fx;
	o["fy"] = fy;
	o["cx"] = cx;
	o["cy"] = cy;

	QFile f(QString::fromStdString(path));
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	f.write(QJsonDocument(o).toJson());
	return true;
}

bool LensCalibration::loadJson(const std::string &path) {
	QFile f(QString::fromStdString(path));
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return false;
	QJsonObject o = doc.object();

	model    = modelFromString(o["model"].toString());
	focal_mm = (float)o["focal_mm"].toDouble();
	k1 = o["k1"].toDouble();
	k2 = o["k2"].toDouble();
	k3 = o["k3"].toDouble();
	p1 = o["p1"].toDouble();
	p2 = o["p2"].toDouble();
	fx = o["fx"].toDouble();
	fy = o["fy"].toDouble();
	cx = o["cx"].toDouble();
	cy = o["cy"].toDouble();
	return model != Model::None;
}

bool LensCalibration::applyTo(cv::Mat &src) const {
	if (!isValid() || src.empty())
		return false;

	if (model == Model::BrownConrady) {
		// Use OpenCV undistort with the stored intrinsic matrix
		cv::Mat K = (cv::Mat_<double>(3, 3)
			<< fx, 0,  cx,
			   0,  fy, cy,
			   0,  0,  1);
		cv::Mat dist = (cv::Mat_<double>(1, 5) << k1, k2, p1, p2, k3);
		cv::Mat dst;
		cv::undistort(src, dst, K, dist);
		src = std::move(dst);
		return true;
	}

	// Lensfun models: build a synthetic lfLens from stored coefficients
	lfLens lens;
	lens.Type     = LF_RECTILINEAR;
	lens.MinFocal = focal_mm;
	lens.MaxFocal = focal_mm;

	lfLensCalibDistortion dc{};
	dc.Focal = focal_mm;
	switch (model) {
	case Model::Poly3:
		dc.Model    = LF_DIST_MODEL_POLY3;
		dc.Terms[0] = (float)k1;
		break;
	case Model::Poly5:
		dc.Model    = LF_DIST_MODEL_POLY5;
		dc.Terms[0] = (float)k1;
		dc.Terms[1] = (float)k2;
		break;
	case Model::PTLens:
		dc.Model    = LF_DIST_MODEL_PTLENS;
		dc.Terms[0] = (float)k1;  // a
		dc.Terms[1] = (float)k2;  // b
		dc.Terms[2] = (float)k3;  // c
		break;
	default:
		return false;
	}
	lens.AddCalibDistortion(&dc);

	int w = src.cols, h = src.rows;
	lfModifier *mod = lfModifier::Create(&lens, 1.0f, w, h);
	if (!mod)
		return false;

	// Aperture and distance only matter for vignetting (not corrected here)
	mod->Initialize(&lens, LF_PF_U8, focal_mm,
	                2.8f, 1000.0f, 1.0f,
	                LF_RECTILINEAR, LF_MODIFY_DISTORTION, false);

	// ApplyGeometryDistortion fills an interleaved (x,y) float array:
	// for each destination pixel (col, row) the source coordinates are at
	// [row * w * 2 + col * 2] and [row * w * 2 + col * 2 + 1].
	std::vector<float> coords(w * h * 2);
	mod->ApplyGeometryDistortion(0.0f, 0.0f, w, h, coords.data());
	mod->Destroy();

	// Wrap as CV_32FC2 and remap
	cv::Mat map(h, w, CV_32FC2, coords.data());
	cv::Mat dst;
	cv::remap(src, dst, map, cv::noArray(),
	          cv::INTER_LANCZOS4, cv::BORDER_REFLECT_101);
	src = std::move(dst);
	return true;
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
