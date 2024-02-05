#ifndef LIGHTSFRAME_H
#define LIGHTSFRAME_H

#include <QStackedWidget>

class LightsFrame: public QStackedWidget {
	Q_OBJECT
public:
	LightsFrame();

public slots:
	void showLp();
	void showSphere();
	void showDome();

private:
	QFrame *createLightsChoice();
	QFrame *createLp();
	QFrame *createSphere();
	QFrame *createDome();
};

class Card: public QFrame {
	Q_OBJECT
public:
	Card(QString title, QString subtitle, QWidget *parent = nullptr);
public slots:
	void click() { emit clicked(); }
signals:
	void clicked();
};


#endif // LIGHTSFRAME_H
