#ifndef DOMEPANEL_H
#define DOMEPANEL_H

#include "../src/dome.h"

#include <QFrame>
#include <QGraphicsScene>

class QLabel;
class QComboBox;

class DomePanel: public QFrame {
	Q_OBJECT
public:
	DomePanel(QWidget *parent = nullptr);
	void init();
	void loadDomeFile(QString path);

public slots:
	void loadDomeFile();
	void exportDome();
	void setDome(int);
	void updateDomeList();

signals:
	void accept(Dome dome);

private:
	Dome dome;
	QStringList dome_labels;
	QStringList dome_paths;
	QComboBox *dome_list;


	//QListWidget *dome_list;

	void loadLP(QString filename);
	void loadDome(QString filename);
};

#endif // DOMEPANEL_H
