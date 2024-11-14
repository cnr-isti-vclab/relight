#include "imagegrid.h"
#include "relightapp.h"
#include "flowlayout.h"

#include <QLabel>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>

#include <assert.h>

#include <iostream>
using namespace std;

ImageThumb::ImageThumb(QImage img, const QString& text, bool skip, QWidget* parent): QWidget(parent) {

	QVBoxLayout* layout = new QVBoxLayout(this);

	QLabel *label = new QLabel;
	label->setPixmap(QPixmap::fromImage(img));
	layout->addWidget(label);

	QCheckBox *checkbox = new QCheckBox(text);

	checkbox->setChecked(!skip);
	connect(checkbox, SIGNAL(stateChanged(int)), this, SIGNAL(skipChanged(int)));
	layout->addWidget(checkbox);

	layout->setSpacing(5);
	layout->setContentsMargins(5, 5, 5, 5);
}

void ImageThumb::setSkipped(bool skip) {
	findChild<QCheckBox *>()->setChecked(!skip);
}

void ImageThumb::setThumbnail(QImage thumb) {
	findChild<QLabel *>()->setPixmap(QPixmap::fromImage(thumb));

}

ImageGrid::ImageGrid(QWidget *parent): QScrollArea(parent) {

	flowlayout = new FlowLayout();
	setWidgetResizable(true);
	setWidget(new QWidget);
	widget()->setLayout(flowlayout);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void ImageGrid::clear() {
	flowlayout->clear();
}

void ImageGrid::init() {
	Project &project = qRelightApp->project();
	std::vector<QImage> &thumbnails = qRelightApp->thumbnails();

	assert(project.images.size() == thumbnails.size());

	for(size_t i = 0; i < project.images.size(); i++) {
		QImage thumbnail = thumbnails[i];
		Image &image = project.images[i];
		QFileInfo info(image.filename);
		ImageThumb *thumb = new ImageThumb(thumbnail, info.fileName(), image.skip);
		connect(thumb, &ImageThumb::skipChanged, [this, i, &image](int state){
			image.skip = (state==0);
			this->emit skipChanged(i, image.skip);
		});
		flowlayout->addWidget(thumb);
	}
}

void ImageGrid::setSkipped(int image, bool skip) {
	ImageThumb *thumb = dynamic_cast<ImageThumb *>(flowlayout->itemAt(image)->widget());
	thumb->setSkipped(skip);
}

void ImageGrid::updateThumbnail(int pos) {
	ImageThumb *thumb = dynamic_cast<ImageThumb *>(flowlayout->itemAt(pos)->widget());

	QMutexLocker lock(&qRelightApp->thumbails_lock);
	thumb->setThumbnail(qRelightApp->thumbnails()[pos]);
	cout << "Img pos: " << pos << endl;
}
