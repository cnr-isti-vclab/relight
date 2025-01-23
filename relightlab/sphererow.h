#ifndef SPHEREROW_H
#define SPHEREROW_H

#include "task.h"
#include <QWidget>



class Sphere;
class QLabel;
class QProgressBar;
class QPushButton;
class ReflectionOverview;
class SphereOverview;

class DetectHighlights: public Task {
public:
	Sphere *sphere;
	bool update_positions;

	DetectHighlights(Sphere *sphere, bool update = true);
	virtual void run() override;

};

class SphereRow: public QWidget {
	Q_OBJECT
public:
	int rowHeight = 92;
	Sphere *sphere = nullptr;
	SphereOverview *position = nullptr;
	ReflectionOverview *reflections = nullptr;
	QLabel *status = nullptr;
	QProgressBar *progress = nullptr;
	QPushButton *verify_button = nullptr;
	DetectHighlights *detect_highlights = nullptr;

	SphereRow(Sphere *sphere, QWidget *parent = nullptr);
	void detectHighlights(bool update = true);
	void stopDetecting();
	
signals:
	void removeme(SphereRow *row);
	void updated(); //emit when status changes

public slots:
	void edit();
	void remove();
	void verify();
	void updateStatus(QString msg, int percent);
};

#endif // SPHEREROW_H
