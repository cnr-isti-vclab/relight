#ifndef SPHEREROW_H
#define SPHEREROW_H

#include "../relight/task.h"
#include <QWidget>



class Sphere;
class QLabel;
class QProgressBar;
class ReflectionView;
class PositionView;

class DetectHighlights: public Task {
public:
	Sphere *sphere;
	DetectHighlights(Sphere *sphere);
	virtual void run() override;

};

class SphereRow: public QWidget {
	Q_OBJECT
public:
	Sphere *sphere;
	int rowHeight = 92;
	PositionView *position;
	ReflectionView *reflections;
	QLabel *status = nullptr;
	QProgressBar *progress = nullptr;
	DetectHighlights *detect_highlights = nullptr;

	SphereRow(Sphere *sphere, QWidget *parent = nullptr);
	void detectHighlights();

public slots:
	void edit();
	void updateStatus(QString msg, int percent);
};

#endif // SPHEREROW_H
