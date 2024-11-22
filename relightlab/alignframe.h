#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>

class ImageViewer;
class QGraphicsRectItem;
class Align;
class AlignRow;
class MarkerDialog;
class QVBoxLayout;

class AlignFrame: public QFrame {
Q_OBJECT
public:
	AlignFrame(QWidget *parent = nullptr);
	void clear();
	void init();
	AlignRow *addAlign(Align *align);

public slots:
	void newAlign();
	void removeAlign(AlignRow *align);


private:
	MarkerDialog *marker_dialog = nullptr;
	QVBoxLayout *aligns = nullptr;
};

#endif // ALIGNFRAME_H
