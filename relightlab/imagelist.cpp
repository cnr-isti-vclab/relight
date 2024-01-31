#include "imagelist.h"
#include "relightapp.h"

#include <QFileInfo>

void ImageList::init() {
	Project &project = qRelightApp->project();
	clear();
	int count =0;
	for(Image &img: project.images) {
		QFileInfo info(img.filename);
		QListWidgetItem *item = new QListWidgetItem(QString("%1 - %2").arg(count+1).arg(info.fileName()));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		addItem(item);

	}
}
