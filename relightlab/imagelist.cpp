#include "imagelist.h"
#include "relightapp.h"

#include <QFileInfo>
#include <assert.h>

void ImageList::init() {
	Project &project = qRelightApp->project();
	clear();
	int count =0;
	for(Image &img: project.images) {
		QFileInfo info(img.filename);
		QListWidgetItem *item = new QListWidgetItem(QString("%1 - %2").arg(count+1).arg(info.fileName()));
		item->setData(Qt::UserRole, count);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		item->setCheckState(img.skip? Qt::Unchecked : Qt::Checked);
		addItem(item);
		count++;
	}

	connect(this, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(verifyItem(QListWidgetItem *)));
}

void ImageList::verifyItem(QListWidgetItem *item) {
	int img_number = item->data(Qt::UserRole).toInt();
	bool skip = item->checkState() != Qt::Checked;

	Project &project = qRelightApp->project();
	assert(img_number >= 0 && size_t(img_number) < project.images.size());

	project.images[img_number].skip = skip;

	emit skipChanged(img_number, skip);

}

void ImageList::setSkipped(int img_number, bool skip) {
	QListWidgetItem *item = this->item(img_number);
	item->setCheckState(skip? Qt::Unchecked : Qt::Checked);
}
