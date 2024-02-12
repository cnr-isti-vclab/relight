#ifndef SPHEREPANEL_H
#define SPHEREPANEL_H

#include "../src/dome.h"
#include "../relight/task.h"
#include <QFrame>
#include <qdialog.h>


class SphereDialog;
class Sphere;
class QLabel;
class QProgressBar;
class QVBoxLayout;


class DetectHighlights: public Task {
public:
	Sphere *sphere;
	DetectHighlights(Sphere *sphere);
	virtual void run() override;
};

class SphereRow: public QWidget {
public:
	Sphere *sphere;
	int rowHeight = 100;
	QLabel *status = nullptr;
	QProgressBar *progress = nullptr;
	DetectHighlights *detect_highlights = nullptr;

	SphereRow(Sphere *sphere, QWidget *parent = nullptr);
	void detectHighlights();
};


class SpherePanel: public QFrame {
	Q_OBJECT
public:
	SpherePanel(QWidget *parent = nullptr);
	void init();
	SphereRow *addSphere(Sphere *sphere);
	void removeSphere(Sphere *sphere);

public slots:
	void newSphere();

signals:
	void accept(Dome &dome);


private:
	Dome dome;
	SphereDialog *sphere_dialog = nullptr;
	QVBoxLayout *spheres = nullptr;
};



#endif // SPHEREPANEL_H
