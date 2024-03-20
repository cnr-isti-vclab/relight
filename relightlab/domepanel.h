#ifndef DOMEPANEL_H
#define DOMEPANEL_H

#include "../src/dome.h"

#include <QFrame>
#include <QGraphicsScene>

class QGraphicsView;
class QLabel;
class QLineEdit;
class QListWidget;
class DirectionsView;

class DomePanel: public QFrame {
	Q_OBJECT
public:
	DomePanel(QWidget *parent = nullptr);
	void init();
	void loadDomeFile(QString path);


public slots:
	void loadDomeFile();
	void setSelectedDome();
	void updateDomeList();

signals:
	void accept(Dome dome);

private:
	Dome dome;
	QStringList dome_labels;
	QStringList dome_paths;

	QLabel *filename;
	QListWidget *dome_list;

	void loadLP(QString filename);
	void loadDome(QString filename);
};

#endif // DOMEPANEL_H
