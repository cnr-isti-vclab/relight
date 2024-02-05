#ifndef LIGHTSFRAME_H
#define LIGHTSFRAME_H

#include <QStackedWidget>

class QLabel;

class Card: public QFrame {
	Q_OBJECT
public:
	Card(QString title, QString subtitle, QWidget *parent = nullptr);
public slots:
	void click() { emit clicked(); }
signals:
	void clicked();
};

class LpFrame: public QFrame {
	Q_OBJECT
public:
	LpFrame(QWidget *parent = nullptr);
	void init();
	void loadLP(QString filename);
public slots:
	void loadLP();
private:
	QLabel *filename;
};

class SphereFrame: public QFrame {
public:
	SphereFrame(QWidget *parent = nullptr);
	void init();
};

class DomeFrame: public QFrame {
public:
	DomeFrame(QWidget *parent = nullptr);
	void init();
};

class LightsFrame: public QStackedWidget {
	Q_OBJECT
public:
	LightsFrame();

public slots:
	void init();
	void showChoice();
	void showLp();
	void showSphere();
	void showDome();

private:
	QFrame *createChoiceFrame();
	LpFrame *lp = nullptr;
	SphereFrame *sphere = nullptr;
	DomeFrame *dome = nullptr;
};



#endif // LIGHTSFRAME_H
