#ifndef SPHEREROW_H
#define SPHEREROW_H

#include "../src/task.h"
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
	void cleanCache();
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
	virtual ~SphereRow();
	void detectHighlights(bool update = true);
	void stopDetecting();

signals:
	void editme(SphereRow *row);
	void removeme(SphereRow *row);
	void updated(); //emit when status changes

public slots:
	void remove();
	void verify();
	void updateStatus(QString msg, int percent);
};

#endif // SPHEREROW_H
