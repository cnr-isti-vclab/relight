#include <QDir>
#include "ballpickerdialog.h"
#include "ui_ballpickerdialog.h"

#include <QGraphicsPixmapItem>
#include <QTextStream>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>

#include <assert.h>

#include <iostream>
using namespace std;

BallPickerDialog::BallPickerDialog(QWidget *parent) :
	QDialog(parent), fitted(false), processed(true), ui(new Ui::BallPickerDialog), circle(NULL) {


	ui->setupUi(this);

	scene = new QGraphicsScene(this);
	ui->graphicsView->setScene(scene);
	ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setInteractive(true);

	connect(ui->openButton, SIGNAL(clicked()), this, SLOT(open()));
	connect(ui->fitButton, SIGNAL(clicked()), this, SLOT(fit()));
	connect(ui->processButton, SIGNAL(clicked()), this, SLOT(process()));
	connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(save()));
}

BallPickerDialog::~BallPickerDialog() {
	delete ui;
}

void BallPickerDialog::open() {
	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder");
	if(dir.isNull()) return;
	init(dir);
}


bool BallPickerDialog::init(QString dirname) {
	dir = QDir(dirname);
	if(!dir.exists()) {
		cerr << "Could not find " << qPrintable(dirname) << " folder.\n";
		return false;
	}
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";
	balls = dir.entryList(img_ext);
	if(!balls.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + dirname);
		return false;
	}
	loadImage(dir.filePath(balls[0]));
	return true;
}

bool BallPickerDialog::loadImage(QString filename) {
	QImage img(filename);
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return false;
	}
	auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	scene->addItem(item);
	imgsize= img.size();
	return true;
}

void BallPickerDialog::mouseDoubleClickEvent(QMouseEvent *event) {
	QPoint posso = ui->graphicsView->mapFromParent(event->pos());
	QPointF pos = ui->graphicsView->mapToScene(posso);
	QBrush blueBrush(Qt::blue);
	QPen outlinePen(Qt::black);
	auto ellipse = scene->addEllipse(pos.x()-3, pos.y()-3, 6, 6, outlinePen, blueBrush);
	ellipse->setFlag(QGraphicsItem::ItemIsMovable);
	ellipse->setData(0, 1);
	ellipse->setCursor(Qt::CrossCursor);
}

void BallPickerDialog::fit() {
	//collect all allises in the scene
	vector<QPointF> centers;
	QList<QGraphicsItem *> items = scene->items();
	for(int i = 0; i  < items.size(); i++) {
		QGraphicsEllipseItem *e = dynamic_cast<QGraphicsEllipseItem *>(items[i]);
		if(!e) continue;
		if(e->data(0).toInt() != 1) continue;

		centers.push_back( e->rect().center() + QPointF(e->x(), e->y()));
	}

	if(centers.size() < 3) {
		QMessageBox::warning(this, "We need your help!", "At least 3 points on the rim of the sphere are needed to find a circle!\nDoubleclick to add them");
		fitted = false;
		return;
	}

	float n = centers.size();
	float sx = 0, sy = 0, sxy = 0, sx2 = 0, sy2 = 0, sx3 = 0, sy3 = 0, sx2y = 0, sxy2 = 0;
	for(size_t k = 0; k < centers.size(); k++) {
		float x = centers[k].x();
		float y = centers[k].y();
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

	float d11 = n*sxy - sx*sy;
	float d20 = n*sx2 - sx*sx;
	float d02 = n*sy2 - sy*sy;
	float d30 = n*sx3 - sx2*sx;
	float d03 = n*sy3 - sy2*sy;
	float d21 = n*sx2y - sx2*sy;
	float d12 = n*sxy2 - sx*sy2;

	float a = ((d30 + d12)*d02 - (d03 + d21)*d11)/(2*(d02*d20 - d11*d11));
	float b = ((d03 + d21)*d20 - (d30 + d12)*d11)/(2*(d20*d02 - d11*d11));

	float c = (sx2 +sy2  -2*a*sx - 2*b*sy)/n;
	float r = sqrt(c + a*a + b*b);
	QPen outlinePen(Qt::blue);
	if(circle)
		delete circle;
	circle = scene->addEllipse(a-r, b-r, 2*r, 2*r, outlinePen);
	circle->setData(0, 0);

	center = QPointF(a, b);
	radius = r;

	float max_angle = (52.0/180.0)*M_PI; //60 deg  respect to the vertical
//	float max_angle = (45.0/180.0)*M_PI;
	smallradius = radius*sin(max_angle);

	int startx = (int)floor(center.x() - smallradius);
	int endx = ceil(center.x() + smallradius+1);

	int starty = (int)floor(center.y() - smallradius);
	int endy = (int)ceil(center.y() + smallradius+1);

	inner = QRect(startx, starty, endx - startx, endy - starty);

	if(startx < 0 || starty < 0 || endx >= imgsize.width() || endy >= imgsize.height()) {
		QMessageBox::critical(this, "OOOOPPPPPSSSS!", "The circle must be inside the image!");
		fitted = false;
		return;
	}
	fitted = true;
}




QPointF BallPickerDialog::findLightDir(QImage &sphere, QString filename) {

	uchar threshold = 220;

	QImage img(filename);
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return QPointF(0, 0);
	}
	if(img.size() != imgsize) {
		QMessageBox::critical(this, "Houston we have a problem!", "All images must be the same size! (" + filename + " doesn't...)");
		return QPointF(0, 0);
	}

	QPointF bari(0, 0); //in image coords
	int count = 0;
	while(count < 10 && threshold > 100) {
		bari = QPointF(0, 0);
		count = 0;
		for(int y = inner.top(); y < inner.bottom(); y++) {
			for(int x = inner.left(); x < inner.right(); x++) {

				float X = x - inner.left(); //coordinates in outer rect
				float Y = y - inner.top();

				float cx = X - smallradius;
				float cy = Y - smallradius;
				float d = sqrt(cx*cx + cy*cy);
				if(d > smallradius) continue;

				QRgb c = img.pixel(x, y);
				int g = qGray(c);

				int mg = qGray(sphere.pixel(X, Y));
				assert(X < sphere.width());
				assert(Y < sphere.height());
				if(g > mg) sphere.setPixel(X, Y, qRgb(g, g, g));

				if(g < threshold) continue;

				bari += QPointF(x, y);
				count++;

			}
		}
		if(count > 0) {
			bari.rx() /= count;
			bari.ry() /= count;
		}
		threshold -= 10;
	}

	return bari;
}


void BallPickerDialog::process() {
	cout << "Processing" << endl;
	//TODO check for smallradius circle not going out of image.
	//TODO process disabled until we have a center and radius
	//TODO spherelights shown with lights positions...
	QImage spherelights(inner.width(), inner.height(), QImage::Format_RGB32);
	spherelights.fill(0);

	//iterate over images, find center.
	//TODO warn the user when light findingn failed. (and have him provide manually!
	QStringList failed;
	for(int i = 0; i < balls.size(); i++) {
		QPointF light = findLightDir(spherelights, dir.filePath(balls[i]));
		if(light == QPointF(0, 0))
			failed.push_back(balls[i]);
		cout << "light: " << light.x() << " " << light.y() << endl;

		lights.push_back(light); //still image coordinates
	}

	if(failed.size()) {
		QString str = failed.join('\n');
		QMessageBox::critical(this, "Too bad we miss some light.", "Couldn't find the highlight in the following images:" + str);
	}

	for(size_t i = 0; i < lights.size(); i++) {
		QPointF l = lights[i];
		int x = l.x() -inner.left();
		int y = l.y() - inner.top();
		spherelights.setPixel(x, y, qRgb(255, 0, 0));
	}
	spherelights.save(dir.filePath("sphere.png"));

	auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(spherelights));
	item->setPos(inner.topLeft());
	scene->addItem(item);

	cout << "Done!" << endl;
	processed = failed.size() == 0;
}



void BallPickerDialog::save() {
	//save LP
	//TODO disabled when not processed, fitting requires new processing.
	//TODO use messagebox and return.
	//TODO get position of lights instead.
	QFile sphere(dir.filePath("sphere.lp"));
	if(!sphere.open(QFile::WriteOnly)) {
		cerr << "Couldl not save sphere.lp!\n";
		exit(0);
	}
	if(lights.size() != (size_t)balls.size()) {
		QMessageBox::critical(this, "Unexpected error.", "Highlights and images should have the same number");
		return;

	}
	QTextStream stream(&sphere);
	stream << lights.size() << "\n";
	for(int i = 0; i < balls.size(); i++) {

		float x = lights[i].x();
		float y = lights[i].y();
		x = (x - inner.left() - smallradius)/radius;
		y = (y - inner.top() - smallradius)/radius;

		float d = sqrt(x*x + y*y);
		float a = asin(d)*2;
		float r = sin(a);
		cout << x << " " << y  << " -> ";
		x *= r/d;
		y *= r/d;
		cout << x << " " << y << endl;
		y *= -1; //cooordinates inverted!

		float z = sqrt(1.0 - x*x - y*y);
		stream << balls[i] << " " << x << " " << y << " " << z << "\n";
	}
}
