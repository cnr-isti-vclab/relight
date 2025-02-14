#ifndef DOMEPANEL_H
#define DOMEPANEL_H

#include "../src/dome.h"

#include <QFrame>
#include <QGraphicsScene>

class QLabel;
class QComboBox;
class QPushButton;

class DomePanel: public QFrame {
	Q_OBJECT
public:
	DomePanel(QWidget *parent = nullptr);
	void init();
	void loadDomeFile(QString path);
	void setSphereSelected();

public slots:
	void loadDomeFile();
	void exportDome();
	void setDome(int);
	void setSpheres();
	void updateDomeList(QString path = QString());

signals:
	void updated();
	void useSpheres();

private:
	Dome dome;
	QStringList dome_labels;
	QStringList dome_paths;

	QPushButton *sphere_button = nullptr;
	QComboBox *dome_list = nullptr;

	QFrame *sphere_frame = nullptr;
	QFrame *dome_frame = nullptr;


	void loadLP(QString filename);
	void loadDome(QString filename);
};

#endif // DOMEPANEL_H
