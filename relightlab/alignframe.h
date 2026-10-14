#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>
#include <QRect>
#include "../src/align/align_task.h"

class ImageViewer;
class QGraphicsRectItem;
class Align;
class AlignRow;
class MarkerDialog;
class QVBoxLayout;
class QStackedWidget;
class QPushButton;
class QProgressBar;

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
	bool selecting_assisted_region = false;
	QVBoxLayout *aligns = nullptr;
	QPushButton *auto_button = nullptr;
	QProgressBar *auto_progress = nullptr;
	AlignTask *auto_align = nullptr;

	AlignRow *findRow(Align *align);
	void startAutoAlign(QRect region);
};

#endif // ALIGNFRAME_H
