#include "sphere.h"
#include "project.h"
#include "lens.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QRunnable>
#include <QGradient>
#include <QPainter>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include <iostream>
#include <math.h>
#include <assert.h>

using namespace std;
using namespace Eigen;

Sphere::Sphere(int n_lights) {
	lights.resize(n_lights);
	directions.resize(n_lights);
	thumbs.resize(n_lights);
}

void Sphere::resetHighlight(size_t n) {
	lights[n] = QPointF();
	directions[n] = Vector3f();
}

void Sphere::ellipseFit() {
	size_t n = border.size();
	Eigen::MatrixXd D1(n, 3);
	Eigen::MatrixXd D2(n, 3);
	for(size_t k = 0; k < border.size(); k++) {
		double x = border[k].x();
		double y = border[k].y();
		D1(k, 0) = x*x;
		D1(k, 1) = x*y;
		D1(k, 2) = y*y;
		D2(k, 0) = x;
		D2(k, 1) = y;
		D2(k, 2) = 1.0;
	}

	Eigen::MatrixXd S1(3,3);
	Eigen::MatrixXd S2(3,3);
	Eigen::MatrixXd S3(3,3);

	S1 = D1.transpose() * D1;
	S2 = D1.transpose() * D2;
	S3 = D2.transpose() * D2;

	Eigen::MatrixXd C1(3,3);
	C1 << 0,0,2, 0,-1,0, 2,0,0;

	Eigen::MatrixXd M;

	M= C1.inverse()* (S1 - S2*S3.inverse()*S2.transpose());

	Eigen::EigenSolver<Eigen::MatrixXd> s(M);

	Eigen::MatrixXd eigenvector= s.eigenvectors().real();
	Eigen::VectorXd row0 = eigenvector.row(0);
	Eigen::VectorXd row1 = eigenvector.row(1);
	Eigen::VectorXd row2 = eigenvector.row(2);

	Eigen::VectorXd cond = 4 * row0.array() * row2.array() - row1.array().pow(2);


	Eigen::VectorXd min_pos_eig = eigenvector.col(0);
	for(int i = 0; i<3; i++) {
		if(cond(i) > 0){
			min_pos_eig = eigenvector.col(i);
			break;
		}
	}
	Eigen::VectorXd coeffs(6);
	Eigen::VectorXd cont_matrix=  -1*S3.inverse()* S2.transpose() * min_pos_eig;
	coeffs << min_pos_eig, cont_matrix;


	double a = coeffs(0);
	double b = coeffs(1)/2;
	double c = coeffs(2);
	double d = coeffs(3)/2;
	double f = coeffs(4)/2;
	double g = coeffs(5);

	double center_x = (c*d-b*f)/(pow(b,2)-a*c);
	double center_y = (a*f-b*d)/(pow(b,2)-a*c);

	double numerator = 2*(a*f*f+c*d*d+g*b*b-2*b*d*f-a*c*g);
	double denominator1 = (b*b-a*c)*( (c-a)*sqrt(1+4*b*b/((a-c)*(a-c)))-(c+a));
	double denominator2 = (b*b-a*c)*( (a-c)*sqrt(1+4*b*b/((a-c)*(a-c)))-(c+a));

	double width = sqrt(numerator/denominator1);
	double height = sqrt(numerator/denominator2);
	double phi = 0.5*atan((2*b)/(a-c));

	if(width < height) {
		std::swap(width, height);
		phi += M_PI/2;
	}

	this->ellipse = true;
	center = QPointF(center_x, center_y);
	eWidth = width;
	eHeight = height;
	eAngle = phi*180/M_PI;
}

bool Sphere::fit() {
	if(border.size() < 3)
		return false;

	if(border.size() >= 5) {
		ellipseFit();
		if(isnan(eWidth)) {
			ellipse = false;
			eWidth = eHeight = radius;
			eAngle = 0.0f;
		} else {
			radius = eWidth;
			assert(eWidth >= eHeight);
		}
	}
	if(!ellipse) {
		//TODO fitCircle
		double n = border.size();
		double sx = 0, sy = 0, sxy = 0, sx2 = 0, sy2 = 0, sx3 = 0, sy3 = 0, sx2y = 0, sxy2 = 0;
		for(size_t k = 0; k < border.size(); k++) {
			double x = border[k].x();
			double y = border[k].y();
			sx += x;
			sy += y;
			sxy += x*y;
			sx2 += x*x;
			sy2 += y*y;
			sx3 += x*x*x;
			sy3 += y*y*y;
			sx2y += x*x*y;
			sxy2 += x*y*y;
		}

		double d11 = n*sxy - sx*sy;
		double d20 = n*sx2 - sx*sx;
		double d02 = n*sy2 - sy*sy;
		double d30 = n*sx3 - sx2*sx;
		double d03 = n*sy3 - sy2*sy;
		double d21 = n*sx2y - sx2*sy;
		double d12 = n*sxy2 - sx*sy2;

		double a = ((d30 + d12)*d02 - (d03 + d21)*d11)/(2*(d02*d20 - d11*d11));
		double b = ((d03 + d21)*d20 - (d30 + d12)*d11)/(2*(d20*d02 - d11*d11));

		double c = (sx2 +sy2  -2*a*sx - 2*b*sy)/n;
		double r = sqrt(c + a*a + b*b);

		center = QPointF(a, b);
		radius = r;

	}
	float max_angle = (50.0/180.0)*M_PI; //slightly over 45. hoping not to spot reflexes
	smallradius = radius*sin(max_angle);

	int startx = (int)floor(center.x() - smallradius);
	int endx = (int)ceil(center.x() + smallradius+1);

	int starty = (int)floor(center.y() - smallradius);
	int endy = (int)ceil(center.y() + smallradius+1);

	inner = QRect(startx, starty, endx - startx, endy - starty);

	//	sphere =QImage(endx - startx, endy - starty, QImage::Format_ARGB32);
	//	sphere.fill(0);

	/*	if(startx < 0 || starty < 0 || endx >= imgsize.width() || endy >= imgsize.height()) {
		fitted = false;
		return false;
	} */
	sphereImg = QImage(inner.width(), inner.height(), QImage::Format_ARGB32);
	sphereImg.fill(0);

	fitted = true;
	return true;
}
bool inEllipse(double x, double y, double a, double b, double theta) {
	double x_rotated = x * cos(theta) + y * sin(theta);
	double y_rotated = y * cos(theta) - x * sin(theta);

	double value = pow(x_rotated / a, 2) + pow(y_rotated / b, 2);
	return value <= 1.0;
}


void Sphere::findHighlight(QImage img, int n, bool skip, bool update_positions) {
	if(sphereImg.isNull()) {
		sphereImg = QImage(inner.width(), inner.height(), QImage::Format_ARGB32);
		sphereImg.fill(0);
	}


	thumbs[n] =img.copy(inner);

	if(skip)
		return;

	uchar threshold = 240;
	//0.5% of the area allocated to the reflection.
	int highlight_area = (inner.width()*inner.height())/200;
	vector<int> histo;

	//lower threshold until we find something.
	int max_luma_pixel = 0;
	QPointF bari(0, 0); //in image coords
	int count = 0;
	int iter = 0;
	while(count < highlight_area && threshold > 100) {
		QPointF new_bari = QPointF(0, 0);
		count = 0;
		for(int y = inner.top(); y < inner.bottom(); y++) {
			for(int x = inner.left(); x < inner.right(); x++) {

				float X = x - inner.left(); //coordinates in outer rect
				float Y = y - inner.top();

				float cx = X - smallradius;
				float cy = Y - smallradius;
				if(ellipse) {
					if(!inEllipse(cx, cy, eWidth, eHeight, eAngle))
						continue;
				} else {
					float d = sqrt(cx*cx + cy*cy);
					if(d > smallradius) continue;
				}
				QRgb c = img.pixel(x, y);
				int g = qGray(c);

				assert(X >= 0 && X < sphereImg.width());
				assert(Y >= 0 && Y < sphereImg.height());
				int thumb_g = qGray(sphereImg.pixel(X, Y));
				if(g > thumb_g) sphereImg.setPixel(X, Y, qRgb(g, g, g));

				max_luma_pixel = std::max(max_luma_pixel, g);
				if(g < threshold) continue;

				new_bari += QPointF(x, y);
				count++;

			}
		}
		histo.push_back(count);

		if(count > 0) {
			new_bari.rx() /= count;
			new_bari.ry() /= count;
			bari = new_bari;
		}

		threshold -= 10;
		iter++;
	}
	//threshold now is 10 lower so we get more points.
	threshold += 10;

	if(bari.isNull()) {
		cout << "Bari null! " << n << endl;
	}

	if(!update_positions)
		return;
	

	if(max_luma_pixel < 127) {
		//highlight in the mid greys? probably all the sphere is in shadow.
		lights[n] = QPointF(0, 0);
		cout << "Probably in shadow: " << n << endl;
		return;
	}

	iter = 0;
	//find biggest spot by removing outliers.
	double radius = ceil(0.5*inner.width());
	while(radius > ceil(0.02*inner.width())) {
		QPointF newbari(0, 0); //in image coords
		double weight = 0.0;
		int starty = std::max(inner.top(),    int(floor(bari.ry()) - radius));
		int endy   = std::min(inner.bottom(), int( ceil(bari.ry()) + radius));
		int startx = std::max(inner.left(),   int(floor(bari.rx()) - radius));
		int endx   = std::min(inner.right(),  int( ceil(bari.rx()) + radius));
		for(int y = starty; y < endy; y++) {
			for(int x = startx; x < endx; x++) {

				float X = x - inner.left(); //coordinates in outer rect
				float Y = y - inner.top();

				float cx = X - smallradius;
				float cy = Y - smallradius;
				float d = sqrt(cx*cx + cy*cy);
				if(d > smallradius) continue;

				QRgb c = img.pixel(x, y);
				int g = qGray(c);
				if(g < threshold) continue;
				
				newbari += QPointF(x*double(g), y*double(g));
				weight += g;
			}
		}
		iter++;
		if(weight == 0.0) break;
		bari = newbari/weight;
		radius *= 0.5;
	}

	lights[n] = bari;
}

void Sphere::computeDirections(Lens &lens) {

	Eigen::Vector2f radial;
	if(ellipse) {
		//check large axis:
		Eigen::Vector2f major = { cos(eAngle*M_PI/180), sin(eAngle*M_PI/180) }; //this should be major axis.
		major.normalize();
		radial = { center.x() - lens.width/2.0f, center.y() - lens.height/2.0f};
		radial.normalize();
		float deviation = 180*acos(fabs(major.dot(radial)))/M_PI;
	}


	directions.resize(lights.size());
	Vector3f viewDir = lens.viewDirection(center.x(), center.y());
	for(size_t i = 0; i < lights.size(); i++) {
		if(lights[i].isNull()) {
			directions[i] = Vector3f(0, 0, 0);
			continue;
		}


		float x = lights[i].x();
		float y = lights[i].y();
		Vector3f dir = lens.viewDirection(x, y);

		if(ellipse) {
			Eigen::Vector2f diff = { x - center.x(), y - center.y() };
			Eigen::Vector2f cradial = radial*diff.dot(radial); //find radial component;
			diff -= cradial; //orthogonal component;
			cradial *= eHeight/eWidth;
			diff += cradial;
			diff /= eWidth;
			x = diff.x();
			y = -diff.y();
		} else {
			x = (x - inner.left() - smallradius)/radius;
			y = -(y - inner.top() - smallradius)/radius; //inverted y  coords
		}

		float d = sqrt(x*x + y*y);
		float a = asin(d)*2;

		//this takes into account large spheres
		float delta = acos((viewDir.dot(dir))/(dir.norm() * viewDir.norm()));
		a += delta;
		float r = sin(a);
		x *= r/d;
		y *= r/d;

		float z2 = std::min(1.0, std::max(0.0, (1.0 - x*x - y*y)));
		float z = sqrt(z2);

		directions[i] = Vector3f(x, y, z);
	}

}

Line Sphere::toLine(Vector3f dir, Lens &lens) {
	Line line;
	line.origin[0] = (center.x() - lens.width/2.0f)/lens.width;
	line.origin[1] = (center.y() - lens.height/2.0f)/lens.width;
	line.direction = dir;
	return line;
}

//find the intersection of the lines using least squares approximation.
Vector3f intersection(std::vector<Line> &lines) {
	Eigen::MatrixXd A(lines.size(), 3);
	Eigen::VectorXd B(lines.size());
	for(size_t i = 0; i < lines.size(); i++) {
		A(i, 0) = lines[i].direction[0];
		A(i, 1) = lines[i].direction[1];
		A(i, 2) = lines[i].direction[2];
		B(i) = lines[i].origin.dot(lines[i].direction);
	}
	Eigen::VectorXd X = A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(B);
	return Vector3f(X[0], X[1], X[2]);
}

//estimate light directions relative to the center of the image.
void computeDirections(std::vector<Image> &images, std::vector<Sphere *> &spheres, Lens &lens, std::vector<Vector3f> &directions) {

	if(spheres.size() == 0)
		return;

	directions.clear();

	//when a light is not in the center of the image we get a bias in the distribution of the lights on the sphere
	//if more than one light from this we can estimate an approximate radius, if not just get the directions.
	for(Sphere *sphere: spheres) {
		sphere->computeDirections(lens);
	}
	//compute just average direction
	for(size_t i = 0; i < spheres[0]->lights.size(); i++) {
		if(images[i].skip)
			continue;

		Vector3f dir(0, 0, 0);
		for(Sphere *sphere: spheres) {
			if(sphere->directions[i].isZero()) continue;
			dir += sphere->directions[i];
		}
		dir.normalize();
		directions.push_back(dir);
	}
}

//estimate light positions using parallax (image width is the unit).
void computeParallaxPositions(std::vector<Image> &images, std::vector<Sphere *> &spheres, Lens &lens, std::vector<Vector3f> &positions) {
	positions.clear();

	for(Sphere *sphere: spheres)
		sphere->computeDirections(lens);
	
	if(spheres.size() == 1) {
		positions = spheres[0]->directions;
		return;
	}
	//for each reflection, compute the lines and the best intersection, estimate the radiuus of the positions vertices
	float radius = 0;
	for(size_t i = 0; i < spheres[0]->directions.size(); i++) {
		if(images[i].skip)
			continue;

		std::vector<Line> lines;
		for(Sphere *sphere: spheres) {
			if(sphere->directions[i].isZero()) continue;
			lines.push_back(sphere->toLine(sphere->directions[i], lens));
		}
		Vector3f position = intersection(lines);
		radius += position.norm();
		positions.push_back(position);
	}
	radius /= spheres[0]->directions.size();
	//if some directions is too different from the average radius, we bring the direction closer to the average.
	float threshold = 0.1;
	for(Vector3f &dir: positions) {
		float d = dir.norm();
		if(fabs(d - radius) > threshold*radius) {
			dir *= radius/d;
		}
	}
}

//estimate light positions assuming they live on a sphere (parameters provided by dome
void computeSphericalPositions(std::vector<Image> &images, std::vector<Sphere *> &spheres, Dome &dome, Lens &lens, std::vector<Vector3f> &positions) {
	positions.clear();
	computeDirections(images, spheres, lens, positions);
	assert(dome.imageWidth > 0 && dome.domeDiameter > 0);

	for(Vector3f &p: positions) {
		p *= dome.domeDiameter/2.0;
		p[2] += dome.verticalOffset;
	}
}

QJsonObject Sphere::toJson() {
	QJsonObject sphere;
	QJsonArray jcenter = { center.x(), center.y() };
	sphere["center"] = jcenter;
	sphere["radius"] = radius;
	sphere["smallradius"] = smallradius;

	QJsonObject jinner;
	jinner.insert("left", inner.left());
	jinner.insert("top", inner.top());
	jinner.insert("width", inner.width());
	jinner.insert("height", inner.height());
	sphere["inner"] = jinner;

	QJsonArray jlights;
	for(QPointF l: lights) {
		QJsonArray jlight = { l.x(), l.y() };
		jlights.append(jlight);
	}
	sphere["lights"] = jlights;

	QJsonArray jdirections;
	for(Vector3f l: directions) {
		QJsonArray jdir = { l[0], l[1], l[2] };
		jdirections.append(jdir);
	}
	sphere["directions"] = jdirections;

	QJsonArray jborder;
	for(QPointF p: border) {
		QJsonArray b = { p.x(), p.y() };
		jborder.push_back(b);
	}
	sphere["border"] = jborder;
	return sphere;
}

void Sphere::fromJson(QJsonObject obj) {
	auto jcenter = obj["center"].toArray();
	center.setX(jcenter[0].toDouble());
	center.setY(jcenter[1].toDouble());
	fitted = !center.isNull();

	radius = obj["radius"].toDouble();
	smallradius = obj["smallradius"].toDouble();

	auto jinner = obj["inner"].toObject();
	inner.setLeft(jinner["left"].toInt());
	inner.setTop(jinner["top"].toInt());
	inner.setWidth(jinner["width"].toInt());
	inner.setHeight(jinner["height"].toInt());

	lights.clear();
	for(auto jlight: obj["lights"].toArray()) {
		auto j = jlight.toArray();
		lights.push_back(QPointF(j[0].toDouble(), j[1].toDouble()));
	}
	thumbs.resize(lights.size());

	directions.clear();
	for(auto jdir: obj["directions"].toArray()) {
		auto j = jdir.toArray();
		directions.push_back(Vector3f(j[0].toDouble(), j[1].toDouble(), j[2].toDouble()));
	}
	border.clear();
	for(auto jborder: obj["border"].toArray()) {
		auto b = jborder.toArray();
		//TODO cleanp this code replicated in mainwindow.
		border.push_back(QPointF(b[0].toDouble(), b[1].toDouble()));
	}
	fit();
}


void Sphere::readCacheThumbs(QImage img) {
	assert(thumbs.size() > 0);
	int w = 20*inner.width();
	int h = (1 + (thumbs.size() + 1)/20)*inner.height();
	assert(img.width() == w);
	assert(img.height() == h);
	sphereImg = img.copy(0, 0, inner.width(), inner.height());
	for(int i = 0; i < int(thumbs.size()); i++) {
		int j = i+1;
		int x = (j%20)*inner.width();
		int y = (j/20)*inner.height();
		thumbs[i] = img.copy(x, y, inner.width(), inner.height());
	}


}

void Sphere::saveCacheThumbs(QString filename) {
	//set quality 95

	int w = 20*inner.width();
	int h = (1 + (thumbs.size()+1)/20)*inner.height();
	QImage img(w, h, QImage::Format_ARGB32);
	img.fill(0);
	QPainter painter(&img);
	painter.drawImage(0, 0, sphereImg);
	for(int i = 0; i < int(thumbs.size()); i++) {
		if(thumbs[i].isNull())
			continue;
		int j = i+1;
		int x = (j%20)*inner.width();
		int y = (j/20)*inner.height();

		painter.drawImage(x, y, thumbs[i]);
	}
	img.save(filename, "jpg", 95);
}

