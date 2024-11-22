#ifndef IMAGEGRID_H
#define IMAGEGRID_H

#include<QScrollArea>
#include <QWidget>


class FlowLayout;


class ImageThumb : public QWidget {
	Q_OBJECT
public:
	ImageThumb(QImage img, const QString& text, bool skip, QWidget* parent = nullptr);
	void setSkipped(bool skip);
	void setThumbnail(QImage thumb);
signals:
	void skipChanged(int state);


};


class ImageGrid: public QScrollArea {
	Q_OBJECT
public:
	ImageGrid(QWidget *parent = nullptr);

	void clear();
	void init();

public slots:
	void setSkipped(int image, bool skip);
	void updateThumbnail(int pos);

signals:
	void skipChanged(int image, bool skip);

private:
	FlowLayout *flowlayout = nullptr;
};

#endif // IMAGEGRID_H
