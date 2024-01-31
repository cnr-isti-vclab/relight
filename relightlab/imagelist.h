#ifndef IMAGELIST_H
#define IMAGELIST_H

#include <QListWidget>

class ImageList: public QListWidget {
public:
	ImageList(QWidget *parent = nullptr): QListWidget(parent) {}

	void init();
};

#endif // IMAGELIST_H
