#ifndef IMAGELIST_H
#define IMAGELIST_H

#include <QListWidget>

class ImageList: public QListWidget {
	Q_OBJECT
public:
	ImageList(QWidget *parent = nullptr): QListWidget(parent) {}

	void init();

public slots:
	void setSkipped(int image, bool skip);
	void verifyItem(QListWidgetItem *item);

signals:
	void skipChanged(int image, bool skip);
};

#endif // IMAGELIST_H
