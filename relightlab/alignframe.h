#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>
#include <QPointF>
#include <QRect>
#include <vector>
#include "../src/task.h"

class ImageViewer;
class QGraphicsRectItem;
class Align;
class AlignRow;
class MarkerDialog;
class QVBoxLayout;
class QStackedWidget;
class QPushButton;
class QProgressBar;

class AutoAlignTask: public Task {
public:
	std::vector<QPointF> offsets; //for all the images, 0 for the skipped ones
	QRect region;

	AutoAlignTask();
	virtual void run() override;
};

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
	void autoAlign();
	void autoAlignProgress(QString msg, int percent);
	void autoAlignFinished();

private:
	QStackedWidget *stack = nullptr;
	MarkerDialog *marker_dialog = nullptr;

	Align *provisional_align = nullptr;
	QVBoxLayout *aligns = nullptr;
	QPushButton *auto_button = nullptr;
	QProgressBar *auto_progress = nullptr;
	AutoAlignTask *auto_align = nullptr;

	AlignRow *findRow(Align *align);
};

#endif // ALIGNFRAME_H
