#include "spherepanel.h"
#include "spheredialog.h"
#include "spherepicking.h"
#include "../src/sphere.h"
#include "relightapp.h"
#include "../relight/processqueue.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QProgressBar>

DetectHighlights::DetectHighlights(Sphere *_sphere) {
	sphere = _sphere;
}

void DetectHighlights::run() {
	mutex.lock();
	status = RUNNING;
	mutex.unlock();

	Project &project = qRelightApp->project();
	for(size_t i = 0; i < project.images.size(); i++) {

		Image &image = project.images[i];
		if(image.skip) continue;

		QImage img(image.filename);
		sphere->findHighlight(img, i);

		progressed(QString("Detecting highlights"), 100*(i+1) / project.images.size());
	}
	mutex.lock();
	status = DONE;
	mutex.unlock();
}


SphereRow::SphereRow(Sphere *_sphere, QWidget *parent) {
	sphere = _sphere;
	QHBoxLayout *columns = new QHBoxLayout(this);
	columns->setSpacing(20);
	//get thumbnail for first image.
	QLabel *thumb = new QLabel;
	QPixmap pic = QPixmap::fromImage(qRelightApp->thumbnails()[0]);
	thumb->setPixmap(pic.scaledToHeight(rowHeight));
	columns->addWidget(thumb);


	QPixmap pix = QIcon::fromTheme("loader").pixmap(rowHeight, rowHeight);
	QLabel *highlights = new QLabel;
	highlights->setPixmap(pix);
	columns->addWidget(highlights);

	QVBoxLayout *status_layout = new QVBoxLayout;
	columns->addLayout(status_layout, 2);
	status_layout->addStretch();
	status = new QLabel("Locating highlights...");
	status_layout->addWidget(status);
	progress = new QProgressBar;
	progress->setValue(50);
	status_layout->addWidget(progress);
	status_layout->addStretch();

	QPushButton *edit = new QPushButton(QIcon::fromTheme("edit"), "Edit...");
	columns->addWidget(edit, 1);
	QPushButton *verify = new QPushButton(QIcon::fromTheme("check"), "Verify...");
	columns->addWidget(verify, 1);

	QPushButton *remove = new QPushButton(QIcon::fromTheme("trash-2"), "Delete");
	columns->addWidget(remove, 1);

}
void SphereRow::updateStatus(QString msg, int percent) {
	status->setText(msg);
	progress->setValue(percent);

}
void SphereRow::detectHighlights() {
	if(sphere->center.isNull()) {
		status->setText("Needs at least 3 points.");
		return;
	}
	detect_highlights = new DetectHighlights(sphere);
	connect(detect_highlights, &DetectHighlights::progress, this, &SphereRow::updateStatus); //, Qt::QueuedConnection);

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(detect_highlights);
	queue.start();
}


SpherePanel::SpherePanel(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(new QLabel("<h3>Mark a reflective sphere:</h3>"));
	content->addSpacing(30);

	QPushButton *new_sphere = new QPushButton("New sphere...");
	content->addWidget(new_sphere);
	new_sphere->setMinimumWidth(200);
	new_sphere->setMaximumWidth(300);

	QFrame *spheres_frame = new QFrame;
	content->addWidget(spheres_frame);
	spheres = new QVBoxLayout(spheres_frame);

	content->addStretch(1);
	connect(new_sphere, SIGNAL(clicked()), this, SLOT(newSphere()));
}

void SpherePanel::init() {
	//QVBoxLayout
}

void SpherePanel::newSphere() {
	if(!sphere_dialog)
		sphere_dialog = new SphereDialog(this);

	//TODO ACTUALLY images might be skipped!
	Sphere *sphere = new Sphere(qRelightApp->project().images.size());
	sphere_dialog->setSphere(sphere);
	int answer = sphere_dialog->exec();
	if(answer == QDialog::Rejected) {
		delete sphere;
		return;
	}
	qRelightApp->project().spheres.push_back(sphere);
	SphereRow *row = addSphere(sphere);
	row->detectHighlights();
}

SphereRow *SpherePanel::addSphere(Sphere *sphere) {
	/*Row is: 1) thumbnail of the first image where the sphere is rendered.
			  2) detail thumb with all the reflections.
			  3) status
			  4) edit and delete button

	*/
	SphereRow *row = new SphereRow(sphere);
	spheres->addWidget(row);
	return row;
}

void SpherePanel::removeSphere(Sphere *sphere) {

}
