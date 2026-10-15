#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>

class Align;
class AlignRow;
class MarkerDialog;
class QGraphicsScene;
class QGraphicsView;
class QVBoxLayout;
class QStackedWidget;
class QResizeEvent;

class AlignFrame: public QFrame {
Q_OBJECT
public:
	AlignFrame(QWidget *parent = nullptr);
	void clear();
	void init();
	AlignRow *addAlign(Align *align);
	void resizeEvent(QResizeEvent *event) override;

public slots:
	void projectUpdate();
	void newAlign();
	void removeAlign(AlignRow *align);
	void okMarker();
	void cancelMarker();

private:
	void updateSummary();

	QStackedWidget *stack = nullptr;
	MarkerDialog *marker_dialog = nullptr;
	QGraphicsScene *summary_scene = nullptr;
	QGraphicsView *summary_view = nullptr;

	Align *provisional_align = nullptr;
	QVBoxLayout *aligns = nullptr;
};

#endif // ALIGNFRAME_H
