#ifndef LPPANEL_H
#define LPPANEL_H

#include <QFrame>
#include <QGraphicsScene>

#include <vector>
#include "../src/relight_vector.h"
#include "../src/dome.h"

class QLabel;
class DirectionsView;
class QListWidget;

class LpPanel: public QFrame {
	Q_OBJECT
public:
	LpPanel();
	void loadLP(QString filename);

signals:
	void accept(Dome dome);


public slots:
	void loadLP();

private:
	QLabel *lp_filename = nullptr;
	QLabel *csv_filename = nullptr;
	DirectionsView *directions_view = nullptr;
	QLabel *lights_number = nullptr;
	QListWidget *images = nullptr;
};

#endif // LPPANEL_H
