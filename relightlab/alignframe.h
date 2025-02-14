#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>

class ImageViewer;
class QGraphicsRectItem;
class Align;
class AlignRow;
class MarkerDialog;
class QVBoxLayout;
class QStackedWidget;

class AlignFrame: public QFrame {
Q_OBJECT
public:
	AlignFrame(QWidget *parent = nullptr);
	void clear();
	void init();
	AlignRow *addAlign(Align *align);

public slots:
	void projectUpdate();
	void newAlign();
	void editAlign(AlignRow *align);
	void removeAlign(AlignRow *align);
	void okMarker();
	void cancelMarker();

private:
	QStackedWidget *stack = nullptr;
	MarkerDialog *marker_dialog = nullptr;

	Align *provisional_align = nullptr;
	QVBoxLayout *aligns = nullptr;

	AlignRow *findRow(Align *align);
};

#endif // ALIGNFRAME_H
