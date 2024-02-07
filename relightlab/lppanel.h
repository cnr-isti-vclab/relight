#ifndef LPPANEL_H
#define LPPANEL_H

#include <QFrame>
#include <QGraphicsScene>

#include <vector>
#include "../src/relight_vector.h"
#include "../src/dome.h"
class QLineEdit;
class QGraphicsView;
class QTextBrowser;

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
	QLineEdit *lp_filename = nullptr;
	QLineEdit *csv_filename = nullptr;
};

#endif // LPPANEL_H
