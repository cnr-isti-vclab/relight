#include <QDir>
#include <QImage>
#include <QTextStream>
#include <libraw.h>


#include "../levmar-2.6/levmar.h"
#include "../src/vector.h"

#include "../src/getopt.h"

#include <random>
#include <iostream>

#include <assert.h>

using namespace std;
//from 0 to range-1

void help() {
	cout << "Usage: calibrate <file.lp>\n" << endl;
}


class Histogram: public vector<int> {
public:
	int nbins = 20;
	int step = (1<<16)/nbins +1;
	void compute(libraw_data_t &img) {
		resize(3*nbins, 0);

		auto &image = img.image;

		//compute histogram
		for(int y = 0; y < img.sizes.height; y++) {
			for(int x = 0; x < img.sizes.width; x++) {
				ushort *c = image[x + y*img.sizes.width];
				(*this)[3*(c[0]/step) + 0]++;
				(*this)[3*(c[1]/step) + 1]++;
				(*this)[3*(c[2]/step) + 2]++;
			}
		}
	}
};

struct Range {
	int r[4] = { 0, 0, -1, -1 };
	int &operator[](int i) { return r[i]; }
	Range() {}
	Range(int left, int bottom, int right, int top) {
		r[0] = left;
		r[1] = bottom;
		r[2] = right;
		r[3] = top;
	}
};

Vector3d white(libraw_data_t &img, Range range = Range()) {
	int nbins = 50;
	int step = (1<<16)/nbins +1;
	int width = img.sizes.iwidth;
	int height = img.sizes.iheight;

	std::vector<int> histo(nbins*3, 0);
	if(range[2] == -1) range[2] = width;
	if(range[3] == -1) range[3] = height;

	auto &image = img.image;

	//compute histogram
	for(int x = range[0]; x < range[2]; x++) {
		for(int y = range[1]; y < range[3]; y++) {
			ushort *c = image[x + y*width];
			histo[3*(c[0]/step) + 0]++;
			histo[3*(c[1]/step) + 1]++;
			histo[3*(c[2]/step) + 2]++;
		}
	}
	//pick max (at least 10% readed)
	int maxtarget = (range[2] - range[0])*(range[3] - range[1])*0.1;
	int tot = 0;
	int max = 0;
	uint16_t whitethreshold = nbins -1;
	for(int i = nbins-1; i > 0 ; i--) {
		int &size = histo[i*3+1];

		tot += size;
		if(tot < maxtarget) continue;
		if(size > max) {
			max = histo[i*3+1];
		} else {
			if(size < 0.1*max) {
				whitethreshold = i;
				break;
			}
		}
	}

	Vector3d avg(0, 0, 0);
	int count = 0;
	for(int x = range[0]; x < range[2]; x++) {
		for(int y = range[1]; y < range[3]; y++) {
			ushort *c = image[x + y*width];
			if(c[1] > whitethreshold) {
				count++;
				avg[0] += c[0];
				avg[1] += c[1];
				avg[2] += c[2];
			}
		}
	}
	avg = avg/count;
	return avg;
}

class ImageStats: public vector<Vector3d> {
public:
	int nrows = 11;
	int ncols = 15;

	int width;
	int height;

	//histogram.


	void process(libraw_data_t &img, double minfraction, double maxfraction = 1.0) {
		width = img.sizes.iwidth;
		height = img.sizes.iheight;

		resize(nrows*ncols);
		int wstep = width/ncols + 1;
		int hstep = height/nrows + 1;

		for(int y = 0; y < nrows; y++)
			for(int x = 0; x < ncols; x++)
				(*this)[x + y*ncols] = white(img, Range(x*wstep, y*hstep, (x+1)*wstep, (y+1)*hstep));
	}
	Vector3d middle() {
		return (*this)[nrows/2 + ncols/2*nrows];
	}
};

class Adjust: public vector<Vector3d> {
public:
	int nrows = 11;
	int ncols = 15;
	int width, height;

	Adjust() {}
	void init(int w, int h) {
		width = w;
		height = h;
		resize(nrows*ncols);
	}

	//bilinear interpolation and fix
	Vector3us fix(int x, int y, Vector3us v) {
		float dx = ncols*x/(float)width;
		float dy = nrows*y/(float)height;
		int sx = floor(dx);
		int sy = floor(dy);
		dx -= sx;
		dy -= sy;
		if(sx < 0)
			sx = 0, dx = 0.0f;
		if(sy < 0)
			sy = 0, dy = 0.0f;

		if(sx >= ncols-1)
			sx = ncols-2, dx = 1.0f;
		if(sy >= nrows-1)
			sy = nrows-2, dy = 1.0f;
		int o = sx + ncols*sy;
		Vector3d &s00 = (*this)[o];
		Vector3d &s01 = (*this)[o+1];
		Vector3d &s10 = (*this)[o+ncols];
		Vector3d &s11 = (*this)[o+ncols+1];

		Vector3d f = s00*(1-dx)*(1-dy) + s01*dx*(1-dy) + s10*(1-dx)*dy + s11*dx*dy;
		//Vector3d f = s00;
		//Vector3d f = (*this)[nrows/2 + ncols/2*nrows];

		//bilinear interpolation:
		Vector3us r;
		r[0] = (uint16_t)(f[0]*v[0]);
		r[1] = (uint16_t)(f[1]*v[1]);
		r[2] = (uint16_t)(f[2]*v[2]);
		return r;
	}
};

int main(int argc, char *argv[]) {

	if(argc < 2) {
		help();
		return 0;
	}

	opterr = 0;
	char c;
	bool ignore_filenames = true;
	while ((c  = getopt (argc, argv, "h")) != -1)
		switch (c)
		{
		case 'h': help(); return 0;
		default: cerr << "Unknown error" << endl; return 1;
		}

	if(optind == argc) {
		cerr << "Too few arguments" << endl;
		help();
		return 1;
	}

	QString path = argv[optind++];
	QDir dir(path);

	QStringList lps = dir.entryList(QStringList() << "*.lp");
	if(lps.size() == 0) {
		cerr << "Could not find .lp file in folder: " << qPrintable(path) << "\n";
		return false;
	}
	QString sphere_path = dir.filePath(lps[0]);
	QFile sphere(sphere_path);
	if(!sphere.open(QFile::ReadOnly)) {
		cerr << "Could not open: " << qPrintable(sphere_path) << endl;
		return false;
	}

	QTextStream stream(&sphere);
	size_t n;
	stream >> n;


	QStringList img_ext;
	img_ext << "*.nef" << "*.NEF";


	QStringList images = dir.entryList(img_ext);
	std::vector<Vector3d> lights;

	for(size_t i = 0; i < n; i++) {
		QString s;
		Vector3d light;
		stream >> s >> light[0] >> light[1] >> light[2];


		lights.push_back(light);
		QString filepath = dir.filePath(s);

		if(ignore_filenames) {
			if(images.size() != n) {
				cerr << "Lp number of lights (" << n << ") different from the number of images found (" << images.size() << ")\n";
				return false;
			}
			filepath = dir.filePath(images[i]);
		} else {
			//often path are absolute. TODO cleanup HERE!
			QFileInfo info(filepath);
			if(!info.exists()) {
				cerr << "Could not find image: " << qPrintable(s) << endl;
				return false;
			}
		}
	}

	QFile filestats("stats.bin");
	if(!filestats.open(QIODevice::ReadWrite)) {
		cerr << "Failed opening stats.bin" << endl;
		exit(0);
	}

	vector<ImageStats> stats(images.size());

	for(int i = 0; i < images.size(); i++) {
		cout << qPrintable(images[i]) << endl;
		QString &imgpath = images[i];

		//First thing we need the directions
		LibRaw raw;

		QString filepath = dir.filePath(imgpath);

		int ret = raw.open_file(filepath.toStdString().c_str());

		if(ret != LIBRAW_SUCCESS) {
			raw.recycle();
			cerr << "Failed to open: " << qPrintable(imgpath) << endl;
			return 0;
		}

		ret = raw.unpack();
		if(ret != LIBRAW_SUCCESS) {
			cerr << "Failed decoding!" << endl;
			return 0;
		}
		auto &params = raw.imgdata.params;

		params.output_color = 0; //raw9
		params.half_size = 1;
		params.use_camera_wb = 1;
		raw.dcraw_process();

		auto &img = raw.imgdata;

//		Histogram histo;
//		histo.compute(img);
//		for(int v = 1; v < histo.size()*3; v += 3)
//			cout << histo[v] << "\t";
//		cout << endl;


		stats[i].process(img, 0.0, 1.0);
		filestats.write((char *)stats[i].data(), stats[i].size()*sizeof(Vector3d));
		raw.recycle();
	}
	filestats.close();
	return 0;



	for(int i = 0; i < stats.size(); i++) {
		ImageStats &stat = stats[i];

		stat.width = 3689;
		stat.height = 2462;
		stat.resize(stat.ncols*stat.nrows);
		filestats.read((char *)stat.data(), stat.size()*sizeof(Vector3d));
	}


	//graph angle/luminosity. //here we just have the intrinsic luminosity of the leds
	std::vector<double> material;

	QFile mat("mat.csv");
	mat.open(QFile::WriteOnly);
	QTextStream str(&mat);

	for(int i = 0; i < stats.size(); i++) {
		ImageStats &stat = stats[i];
		Vector3d light = lights[i];

		int width = stat.width;
		int height = stat.height;
//#define CENTRAL
#ifdef CENTRAL
		double angle = acos(light[2]);
		double brightness = stat[stat.ncols/2 + (stat.nrows/2)*stat.ncols][1];
		material.push_back(angle);
		material.push_back(brightness);
#else

		double h = 201; //mm //real height
		double w = (width/(double)height)*h; //real width
		double domeradius = 400; //mm
		Vector3d pos = lights[i]*domeradius + Vector3d(0, 0, 70);

		for(int y = 0; y < stat.nrows; y++) {
//			if(y != stat.nrows/2) continue;
			for(int x = 0; x < stat.ncols; x++) {

//				if(x != stat.ncols/2) continue;

				Vector3d p;
				p[0] = (x + 0.5 - stat.ncols/2.0)/stat.ncols * w;
				p[1] = -(y + 0.5 - stat.nrows/2.0)/stat.nrows * h;
				p[2] = 0;
				double d2 = pow((p - pos).norm()/pos.norm(), 2.0);

				p = pos - p;
				p = p/p.norm();

				double angle = acos(p[2]);
				double brightness = (stat[x + y*stat.ncols][1])*d2;

				material.push_back(angle);
				material.push_back(brightness);
				str << angle << "," << brightness << "," << i << "\n";
			}
		}

#endif
	}
	double p[2] = { 15000, 5000 }; //parameters
	double opts[5] = { 1.2, 0.0001, 0.0001, 0.0001, -10 };
	double info[LM_INFO_SZ];

	void *adata = (void *)material.data();

	vector<double> factor(stats[0].ncols, stats[0].nrows);

	dlevmar_dif(
				[](double *p, double *hx, int m, int n, void *adata) {
		double *data = (double *)adata;
		for(int i = 0; i < n; i++) {
			double v = p[0]*cos(data[2*i]) + p[1];
			hx[i] = pow(v - data[2*i+1], 2);
		}
	},
	p,                  /* I/O: initial parameter estimates. On output contains the estimated solution */
	NULL,               /* I: measurement vector. NULL implies a zero vector */
	2,                  /* I: parameter vector dimension (i.e. #unknowns) */
	material.size()/2,  /* I: measurement vector dimension */
	100,                /* I: maximum number of iterations */
	opts,           /* I: opts[0-4] = minim. options [\tau, \epsilon1, \epsilon2, \epsilon3, \delta]. Respectively the
						* scale factor for initial \mu, stopping thresholds for ||J^T e||_inf, ||Dp||_2 and ||e||_2 and the
						* step used in difference approximation to the Jacobian. If \delta<0, the Jacobian is approximated
						* with central differences which are more accurate (but slower!) compared to the forward differences
						* employed by default. Set to NULL for defaults to be used.
						*/
	info,
	/* O: information regarding the minimization. Set to NULL if don't care
						 * info[0]= ||e||_2 at initial p.
						 * info[1-4]=[ ||e||_2, ||J^T e||_inf,  ||Dp||_2, \mu/max[J^T J]_ii ], all computed at estimated p.
						 * info[5]= # iterations,
						 * info[6]=reason for terminating: 1 - stopped by small gradient J^T e
						 *                                 2 - stopped by small Dp
						 *                                 3 - stopped by itmax
						 *                                 4 - singular matrix. Restart from current p with increased \mu
						 *                                 5 - no further error reduction is possible. Restart with increased mu
						 *                                 6 - stopped by small ||e||_2
						 *                                 7 - stopped by invalid (i.e. NaN or Inf) "func" values; a user error
						 * info[7]= # function evaluations
						 * info[8]= # Jacobian evaluations
						 * info[9]= # linear systems solved, i.e. # attempts for reducing error
						 */
	NULL,      /* I: working memory, allocated internally if NULL. If !=NULL, it is assumed to point to
						* a memory chunk at least LM_DIF_WORKSZ(m, n)*sizeof(double) bytes long
						*/
	NULL,     /* O: Covariance matrix corresponding to LS solution; Assumed to point to a mxm matrix.
						* Set to NULL if not needed.
						*/
	adata);       /* I: pointer to possibly needed additional data, passed uninterpreted to func.
						* Set to NULL if not needed
						*/
	cout << "Info E: " << info[0] << ", Reason: " << info[6] << endl;
	cout << "Info: e1: " << info[1] << " e2: " << info[2] << " e3: " << info[3] << endl;
	//cout << "P: " << p[0] << " " << p[1] << endl;
	double diffuse = p[0];
	double ambient = p[1];

	cout << "Diffuse: " << diffuse << " ambient: " << ambient << endl;

	//return 0;
	vector<Adjust> adjust(images.size());

	for(int i = 0; i < stats.size(); i++) {
		ImageStats &stat = stats[i];
		//someone says that we need white balance before demosaicization (but why?).
		int width = stat.width;
		int height = stat.height;

		double h = 201; //mm //real height
		double w = (width/(double)height)*h; //real width
		double domeradius = 400; //mm
		//get position of the light respect to the center of the image
		Vector3d pos = lights[i]*domeradius + Vector3d(0, 0, 0);

		cout << qPrintable(images[i]) << ": " << lights[i][2] << endl;
		/*		for(int y = 0; y < stat.nrows; y++) {
			for(int x = 0; x < stat.ncols; x++) {
				//approx spatial pos
				cout << (int)((stat[x + y*stat.ncols][1]/255)) << "\t";
			}
			cout << endl;
		}
		cout << endl;


		for(int y = 0; y < stat.nrows; y++) {
			for(int x = 0; x < stat.ncols; x++) {
				Vector3d p;
				p[0] = (x + 0.5 - stat.ncols/2.0)/stat.ncols * w;
				p[1] = -(y + 0.5 - stat.nrows/2.0)/stat.nrows * h;
				p[2] = 0;
				double d = pow((p - pos).norm()/pos.norm(), 2.0); //scaled respect the center of the image
				double lambert = ((pos - p)/(pos-p).norm())*Vector3d(0, 0, 1);

				double v = ((diffuse*lambert + ambient)/255);
				cout << (int)v << "\t";
			}
			cout << endl;
		}*/


		Adjust &ad = adjust[i];
		ad.init(width, height);
		for(int y = 0; y < stat.nrows; y++) {
			for(int x = 0; x < stat.ncols; x++) {
				//approx spatial pos
				Vector3d p;
				p[0] = (x + 0.5 - stat.ncols/2.0)/stat.ncols * w;
				p[1] = -(y + 0.5 - stat.nrows/2.0)/stat.nrows * h;
				p[2] = 0;
				double d = pow((p - pos).norm()/pos.norm(), 2.0); //scaled respect the center of the image
				double lambert = ((pos - p)/(pos-p).norm())*Vector3d(0, 0, 1);

				double o = stat[x + y*stat.ncols][1];
				double v = diffuse*lambert + ambient;
				Vector3d m = stat.middle();
				m[0] = m[1]/m[0];
				m[2] = m[1]/m[2];
				m[1] = 1.0;

				m = m * (v/o);

				//diffuse + ambiend should correspond to max.(90%)
				m = m * 0.8*65535.0/(diffuse + ambient);
				ad[x + y*stat.ncols] = m;
				//				cout << v/o << "\t";
			}
		}

	}

	//return 0;
	for(int i = 0; i < images.size(); i++) {

		QString &imgpath = images[i];

		//First thing we need the directions
		LibRaw raw;

		QString filepath = dir.filePath(imgpath);

		int ret = raw.open_file(filepath.toStdString().c_str());

		if(ret != LIBRAW_SUCCESS) {
			raw.recycle();
			cerr << "Failed to open: " << qPrintable(imgpath) << endl;
			return 0;
		}

		ret = raw.unpack();
		if(ret != LIBRAW_SUCCESS) {
			cerr << "Failed decoding!" << endl;
			return 0;
		}
		auto &params = raw.imgdata.params;

		params.output_color = 0; //raw
		params.half_size = 1;
		params.use_camera_wb = 1;
		raw.dcraw_process();


		auto &img = raw.imgdata;
		int width = img.sizes.iwidth;
		int height = img.sizes.iheight;

		QImage image(width, height, QImage::Format_RGB32);

		Adjust &ad = adjust[i];

		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				int i =  x + y*width;
				Vector3us c(img.image[i][0], img.image[i][1], img.image[i][2]);

				Vector3us v = ad.fix(x, y, c);
				if(c[0] < 5000 || c[1] < 5000 || c[2] < 5000) {
					v[0] = 65535;
					v[1] = 0;
					v[2] = 0;
				}
				image.bits()[i*4 + 0] = v[0]/255;
				image.bits()[i*4 + 1] = v[1]/255;
				image.bits()[i*4 + 2] = v[2]/255;
			}
		}
		image.save(QString("%1_test.jpg").arg(images[i]), "jpg", 98);

		raw.recycle();
		//break;

	}

	return 0;
}

