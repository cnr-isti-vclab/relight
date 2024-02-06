#ifndef LPFRAME_H
#define LPFRAME_H

#include <QFrame>
#include <QGraphicsScene>


class QLabel;
class QGraphicsView;
class PropertyBox;

class LpFrame: public QFrame {
	Q_OBJECT
public:
	LpFrame(QWidget *parent = nullptr);
	void init();
	void loadLP(QString filename);

public slots:
	void loadLP();

private:
	PropertyBox *properties;
	QGraphicsView *lights;
	QGraphicsScene scene;

	void initLights();
};

#endif // LPFRAME_H
