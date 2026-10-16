#ifndef ALIGNFRAME_H
#define ALIGNFRAME_H

#include <QFrame>
#include <QPointF>

#include <vector>

class Align;
class AlignRow;
class MarkerDialog;
class QGraphicsScene;
class QGraphicsView;
class QVBoxLayout;
class QStackedWidget;
class QResizeEvent;
class QLabel;
class QProgressBar;
class QPushButton;
class AutoAlignTask;

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
	void automaticAlign();

private:
	void updateSummary();
	void finishAutomaticAlign(AutoAlignTask *task);
	void stopAutomaticAlign();

	QStackedWidget *stack = nullptr;
	MarkerDialog *marker_dialog = nullptr;
	QGraphicsScene *summary_scene = nullptr;
	QGraphicsView *summary_view = nullptr;
	QPushButton *automatic_align_button = nullptr;
	QLabel *automatic_align_status = nullptr;
	QProgressBar *automatic_align_progress = nullptr;
	AutoAlignTask *automatic_align_task = nullptr;
	std::vector<QPointF> automatic_offsets;

	Align *provisional_align = nullptr;
	QVBoxLayout *aligns = nullptr;
};

#endif // ALIGNFRAME_H
