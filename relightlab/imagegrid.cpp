#include "imagegrid.h"
#include "relightapp.h"
#include "flowlayout.h"

#include <QLabel>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>


ImageThumb::ImageThumb(const QString& imagePath, const QString& text, QWidget* parent): QWidget(parent) {

	QVBoxLayout* layout = new QVBoxLayout(this);

	QLabel* thumb = new QLabel;
	QImage img(imagePath);
	img = img.scaledToHeight(256);
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

	for(Image &img: project.images) {
		QFileInfo info(img.filename);
		ImageThumb *thumb = new ImageThumb(img.filename, info.fileName());
		flowlayout->addWidget(thumb);
	}
}

