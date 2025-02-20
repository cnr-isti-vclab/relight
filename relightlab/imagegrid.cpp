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

ImageThumb::ImageThumb(QImage img, const QString& text, bool skip, bool visible, QWidget* parent): QWidget(parent) {

	QVBoxLayout* layout = new QVBoxLayout(this);
	{
		QLabel *label = new QLabel;
		label->setPixmap(QPixmap::fromImage(img));
		layout->addWidget(label);
	}
	{
		QHBoxLayout *checkline = new QHBoxLayout;
		layout->addLayout(checkline);
		{
			skipbox = new QCheckBox(text);
			skipbox->setChecked(!skip);
			connect(skipbox, SIGNAL(stateChanged(int)), this, SIGNAL(skipChanged(int)));
			checkline->addWidget(skipbox);
		}
		{
			visibleicon = new QLabel();
			visibleicon->setPixmap(QIcon::fromTheme(visible ?"eye":"eye-off").pixmap(20, 20));
			checkline->addWidget(visibleicon);

		}

	}
	layout->setSpacing(5);
	layout->setContentsMargins(5, 5, 5, 5);
}

void ImageThumb::setSkipped(bool skip, bool visible) {
	skipbox->setChecked(!skip);
	visibleicon->setPixmap(QIcon::fromTheme(visible ?"eye":"eye-off").pixmap(20, 20));

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
		ImageThumb *thumb = new ImageThumb(thumbnail, info.fileName(), image.skip, image.visible);
		connect(thumb, &ImageThumb::skipChanged, [this, i, &image](int state){
			image.skip = (state==0);
			this->emit skipChanged(i);
		});
		flowlayout->addWidget(thumb);
	}
}

void ImageGrid::setSkipped(int img_number) {
	auto &images = qRelightApp->project().images;
	assert(img_number >= 0 && img_number < images.size());
	Image &img = images[img_number];

	ImageThumb *thumb = dynamic_cast<ImageThumb *>(flowlayout->itemAt(img_number)->widget());
	thumb->setSkipped(img.skip, img.visible);
}

void ImageGrid::updateThumbnail(int pos) {
	QLayoutItem *item = flowlayout->itemAt(pos);
	if(!item) //thumbnail updated before init.
		return;

	ImageThumb *thumb = dynamic_cast<ImageThumb *>(item->widget());

	QMutexLocker lock(&qRelightApp->thumbails_lock);
	thumb->setThumbnail(qRelightApp->thumbnails()[pos]);
}
