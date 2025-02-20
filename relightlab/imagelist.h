#ifndef IMAGELIST_H
#define IMAGELIST_H

#include <QListWidget>

class ImageList: public QListWidget {
	Q_OBJECT
public:
	int icon_size = 16;
	ImageList(QWidget *parent = nullptr): QListWidget(parent) {}

	void init();

public slots:
	void setSkipped(int image);
	void verifyItem(QListWidgetItem *item);

signals:
	void skipChanged(int image);

protected:
	void mousePressEvent(QMouseEvent *event) override ;
};

#endif // IMAGELIST_H
