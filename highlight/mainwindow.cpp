#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "imagedialog.h"
#include "graphics_view_zoom.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QSettings>
#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	settings = new QSettings("VCG", "Relight", this);

	ui->setupUi(this);
	connect(ui->actionOpen,  SIGNAL(triggered(bool)), this, SLOT(open()));
	connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openImage(QListWidgetItem *)));

	scene = new QGraphicsScene(this);
	ui->graphicsView->setScene(scene);
	ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setInteractive(true);
	auto *gvz = new Graphics_view_zoom(ui->graphicsView);
}

MainWindow::~MainWindow() {	delete ui; }

void MainWindow::open() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();
	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder", lastDir);
	if(dir.isNull()) return;
	init(dir);
}

bool MainWindow::init(QString dirname) {
	dir = QDir(dirname);
	if(!dir.exists()) {
		cerr << "Could not find " << qPrintable(dirname) << " folder.\n";
		return false;
	}
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";
	images = dir.entryList(img_ext);
	if(!images.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + dirname);
		return false;
	}

	settings->setValue("LastDir", dir.path());

	ui->listWidget->clear();
	imgsize = QSize();
	valid.clear();
	valid.resize(images.size(), true);

	//create the items (name and TODO thumbnail
	int count = 0;
	for(QString a: images) {
		auto *item = new QListWidgetItem(a, ui->listWidget);
		item ->setData(Qt::UserRole, count++);
	}

	openImage(ui->listWidget->item(0));
	//TODO: in background load and process the images

	return true;
}

void MainWindow::openImage(QListWidgetItem *item) {
	QString filename = item->text();

	QImage img(dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	auto *pixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	scene->addItem(pixmap);
	if(!imgsize.isValid())
		imgsize = img.size();
	if(imgsize != img.size()) {
		int n = item->data(Qt::UserRole).toInt();
		valid[n] = false;
	}
	//ui->graphicsView->scale(5, 5);
}

void MainWindow::exit() {
}
