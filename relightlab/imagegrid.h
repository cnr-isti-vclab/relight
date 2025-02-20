#ifndef IMAGEGRID_H
#define IMAGEGRID_H

#include<QScrollArea>
#include <QWidget>


class FlowLayout;
class QCheckBox;
class QLabel;


class ImageThumb : public QWidget {
	Q_OBJECT
public:
	ImageThumb(QImage img, const QString& text, bool skip, bool visible, QWidget* parent = nullptr);
	void setSkipped(bool skip, bool visible);
	void setThumbnail(QImage thumb);
signals:
	void skipChanged(int state);

private:
	QCheckBox *skipbox = nullptr;
	QLabel *visibleicon = nullptr;
};


class ImageGrid: public QScrollArea {
	Q_OBJECT
public:
	ImageGrid(QWidget *parent = nullptr);

	void clear();
	void init();

public slots:
	void setSkipped(int image);
	void updateThumbnail(int pos);

signals:
	void skipChanged(int image);

private:
	FlowLayout *flowlayout = nullptr;
};

#endif // IMAGEGRID_H
