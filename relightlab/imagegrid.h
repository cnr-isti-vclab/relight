#ifndef IMAGEGRID_H
#define IMAGEGRID_H

#include<QScrollArea>
#include <QWidget>


class FlowLayout;


class ImageThumb : public QWidget {
public:
	ImageThumb(const QString& imagePath, const QString& text, QWidget* parent = nullptr);
};


class ImageGrid: public QScrollArea {
public:
	ImageGrid(QWidget *parent = nullptr);

	void init();
private:
	FlowLayout *flowlayout = nullptr;
};

#endif // IMAGEGRID_H
