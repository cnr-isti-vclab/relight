#include <QMessageBox>
#include <QFileDialog>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>

#include "aligndialog.h"
#include "ui_aligndialog.h"
#include "aligninspector.h"

#include <assert.h>

#include <iostream>
using namespace std;

AlignDialog::AlignDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AlignDialog) {
	ui->setupUi(this);

	scene = new QGraphicsScene(this);
	ui->graphicsView->setScene(scene);
	ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setInteractive(true);
	ui->graphicsView->installEventFilter(this);


	connect(ui->openButton, SIGNAL(clicked()), this, SLOT(open()));
	connect(ui->alignButton, SIGNAL(clicked()), this, SLOT(align()));
	connect(ui->inspectButton, SIGNAL(clicked()), this, SLOT(inspect()));
	connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(save()));

//	connect(ui->processButton, SIGNAL(clicked()), this, SLOT(process()));

}

AlignDialog::~AlignDialog() {
	delete ui;
}

void AlignDialog::open() {
	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder");
	if(dir.isNull()) return;
	init(dir);
}

bool AlignDialog::init(QString dirname) {
	dir = QDir(dirname);
	if(!dir.exists()) {
		cerr << "Could not find " << qPrintable(dirname) << " folder.\n";
		return false;
	}
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";
	images = dir.entryList(img_ext);
//	while(images.size() > 5)
//		images.pop_back();
	if(!images.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + dirname);
		return false;
	}
	loadImage(dir.filePath(images[0]));
	return true;
}

bool AlignDialog::loadImage(QString filename) {
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

void AlignDialog::mouseDoubleClickEvent(QMouseEvent *event) {
	QPoint posso = ui->graphicsView->mapFromParent(event->pos());
	QPointF pos = ui->graphicsView->mapToScene(posso);
	QBrush blueBrush(QColor(255, 0, 0, 128));
	QPen outlinePen(Qt::black);

	if(area) delete area;
	area = scene->addRect(pos.x()-side/2, pos.y()-side/2, side, side, outlinePen, blueBrush);
	area->setFlag(QGraphicsItem::ItemIsMovable);
	area->setData(0, 1);
	area->setCursor(Qt::CrossCursor);
}

bool AlignDialog::eventFilter(QObject *target, QEvent *event) {
	if (event->type() == QEvent::Wheel) {
		if(inspecting) {
			QWheelEvent *wheel = static_cast<QWheelEvent *>(event);
			int direction = (wheel->angleDelta().y()) < 0? 1 : -1;
			cerr << direction << endl;
			event->ignore();
		}
	}
	return QDialog::eventFilter(target, event);
}


void AlignDialog::align() {

	QRect rect = area->rect().translated(area->pos()).toRect();

	samples.resize(images.size());
	assert(samples.size() == images.size());

	for(int i = 0; i < images.size(); i++) {
		QString &filename = images[i];
		QImage img(dir.filePath(filename));
		if(img.isNull()) {
			cerr << "Failed loading image: " << qPrintable(filename) << endl;
			return;
		}
		samples[i] = img.copy(rect);
	}
	reference = samples.size()/2;
	for(size_t i = 0; i < samples.size(); i++) {
		if(i == reference) {
			offsets.push_back(QPoint(0, 0));
			continue;
		}
		QPoint p = alignSample(samples[reference], samples[i]);
		cout << "Img: " << i << ": " << p.x() << " " << p.y() << endl << endl;
		offsets.push_back(p);
	}
}

void testBestInfo(double info, QPoint point, double &best_info, QPoint &best) {
	if(info > best_info) {
		best_info = info;
		best = point;
	}
}

QPoint AlignDialog::alignSample(QImage a, QImage b) {
	//start with a rectangle of side 2. if max is 2 in the inside we bail out.
	double best_info = mutualInformation(a, b, 0, 0);
	QPoint best(0, 0);

	for(int ring = 1; ring < max_shift; ring++) {
		for(int dx = -ring; dx <= ring; dx++) {
			double i0 = mutualInformation(a, b, dx, -ring);
			double i1 = mutualInformation(a, b, dx, ring);
			testBestInfo(i0, QPoint(dx, -ring), best_info, best);
			testBestInfo(i1, QPoint(dx, ring), best_info, best);
		}
		for(int dy = -ring+1; dy < ring; dy++) {
			double i0 = mutualInformation(a, b, -ring, dy);
			double i1 = mutualInformation(a, b,  ring, dy);
			testBestInfo(i0, QPoint(-ring, dy), best_info, best);
			testBestInfo(i1, QPoint( ring, dy), best_info, best);
		}
		if(std::max(abs(best.x()), abs(best.y())) <= ring - 2)
			break;
	}


	return best;
}

/*QPoint AlignDialog::alignSample(QImage a, QImage b) {
	int max = max_shift;

	vector<double> infos((2*max+1)*(2*max+1), 0.0);
	double best_info = 0.0;
	QPoint best(0, 0);
	double imin = 1e20;
	double imax = -1e20;
	for(int dy = -max; dy <= max; dy++) {
		for(int dx = -max; dx <= max; dx++) {
			double info = mutualInformation(a, b, dx, dy);
			int i = dx + max + (dy + max)*(2*max+1);
			assert(i < infos.size() && i >= 0);
			infos[i] = info;
			imin = std::min(imin, info);
			imax = std::max(imax, info);
			//cout << info << " ";
			if(info > best_info) {
				best_info = info;
				best.rx() = dx;
				best.ry() = dy;
			}
		}
		//cout << endl;
	}
	//scale info betwee 0 and 255
	for(double &i: infos) {
		i = floor(255.0*(i - imin)/(imax - imin));
	}
	static int count = 0;
	QImage img(2*max+1, 2*max+1, QImage::Format_RGB32);
	for(int dy = 0; dy <= 2*max; dy++) {
		for(int dx = 0; dx <= 2*max; dx++) {
			int g = (int)infos[dx + dy*(2*max+1)];
			img.setPixel(dx, dy, qRgb(g, g, g));
		}
	}
	img.save(dir.filePath(QString("heights_%1.png").arg(count++)));
	return best;
}*/

double AlignDialog::mutualInformation(QImage a, QImage b, int dx, int dy) {
	int max = max_shift;
	std::vector<int> histo(256*256, 0);
	std::vector<int> aprob(256, 0);
	std::vector<int> bprob(256, 0);
	int width = a.width();
	int height = a.height();
	//x and y refer to the image pixels!
	for(int y = max; y < height - max; y++) {
		for(int x = max; x < width - max; x++) {
			QRgb ca = qGray(a.pixel(x, y));
			QRgb cb = qGray(b.pixel(x + dx, y + dy));
			histo[ca + 256*cb]++;
			aprob[ca]++;
			bprob[cb]++;
		}
	}
	double tot = (height - 2*max)*(width - 2*max);
	double info = 0.0;

	for(int y = 0; y < 256; y++) {
		for(int x = 0; x < 256; x++) {
			double p = histo[x + 256*y]/tot;
			if(p == 0) continue;
			double pa = aprob[x]/tot;
			double pb = bprob[y]/tot;
			info += p * log(p/(pa*pb));
		}
	}
	return info;
}
//TODO change reference image!.
void AlignDialog::inspect() {
	if(!area) return;
	if(samples.size() < 2) return;


	AlignInspector *inspector = new AlignInspector(offsets, samples, reference, this);
	inspector->show();
	inspector->exec();
	offsets = inspector->offsets;
	return;
//hide area, zoom to area
//disable panning
//show semitransparent //tell which image in some manner.
//wheel event
//button for panning + keys

	area->hide();
	//ui->graphicsView->setDragMode(QGraphicsView::NoDrag);
	QRectF rect = area->rect();
	rect.translate(area->pos());
	//ui->graphicsView->setHorizontalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
	//ui->graphicsView->setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
	//ui->graphicsView->fitInView(rect, Qt::KeepAspectRatio);

	//ensure we have at leasst 2 images
	auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(samples[1]));

	item->setPos(rect.topLeft());

	scene->addItem(item);


	//ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	//area->show();

	inspecting = true;
}

void AlignDialog::save() {
	//find min and max

	QPoint min(0, 0);
	QPoint max(0, 0);
	for(QPointF &p: offsets) {
		if((int)p.x() < min.x()) min.rx() = (int)p.x();
		if((int)p.y() < min.y()) min.ry() = (int)p.y();
		if((int)p.x() > max.x()) max.rx() = (int)p.x();
		if((int)p.y() > max.y()) max.ry() = (int)p.y();
	}
	dir.mkdir("aligned");
	for(int i = 0; i < images.size(); i++) {
		QPointF &o = offsets[i];
		QImage img(dir.filePath(images[i]));
	//	QImage img = samples[i];
		QPoint topleft(-min.x() + (int)o.x(), -min.y() + (int)o.y());
		QPoint margin((max.x() - min.x()), (max.y() - min.y()));

		QImage tmp = img.copy(topleft.x(), topleft.y(), img.width() - margin.x(), img.height() - margin.y());
		if(tmp.isNull()) {
			cerr << "Failed copying img rect: " << topleft.x() << " " << topleft.y() << " " << margin.x() << " " <<  margin.y() << endl;
		}
		cout << qPrintable(dir.filePath("aligned/" + images[i])) << endl;
		tmp.save(dir.filePath("aligned/" + images[i]), "jpg", 98);
	}
}
