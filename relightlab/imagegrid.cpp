#include "imagegrid.h"
#include "relightapp.h"
#include "flowlayout.h"

#include <QLabel>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>

#include <assert.h>

ImageThumb::ImageThumb(QImage img, const QString& text, QWidget* parent): QWidget(parent) {

	QVBoxLayout* layout = new QVBoxLayout(this);

	QLabel* thumb = new QLabel;
	thumb->setPixmap(QPixmap::fromImage(img));
	layout->addWidget(thumb);

	QCheckBox *checkbox = new QCheckBox(text);
	layout->addWidget(checkbox);

	layout->setSpacing(5);
	layout->setContentsMargins(5, 5, 5, 5);
}

ImageGrid::ImageGrid(QWidget *parent): QScrollArea(parent) {

	flowlayout = new FlowLayout();
	setWidgetResizable(true);
	setWidget(new QWidget);
	widget()->setLayout(flowlayout);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void ImageGrid::init() {
	Project &project = qRelightApp->project();
	std::vector<QImage> &thumbnails = qRelightApp->thumbnails();

	assert(project.images.size() == thumbnails.size());

	for(size_t i = 0; i < project.images.size(); i++) {
		QImage thumbnail = thumbnails[i];
		Image &image = project.images[i];
		QFileInfo info(image.filename);
		ImageThumb *thumb = new ImageThumb(thumbnail, info.fileName());
		flowlayout->addWidget(thumb);
	}
}

