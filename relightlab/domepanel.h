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

public slots:
	void loadDomeFile();
	void setSelectedDome();

signals:
	void accept(Dome dome);

private:
	Dome dome;
	QStringList dome_labels;
	QStringList dome_paths;

	QLabel *filename;
	QLineEdit *label;
	QLabel *number;
	QLabel *notes;
	QListWidget *dome_list;
	QListWidget *images;
	DirectionsView *directions_view;

	void loadLP(QString filename);
	void loadDome(QString filename);
	void update(QString filename);
};

#endif // DOMEPANEL_H
