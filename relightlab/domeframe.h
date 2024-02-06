#ifndef DOMEFRAME_H
#define DOMEFRAME_H

#include <QFrame>

class DomeFrame: public QFrame {
public:
	DomeFrame(QWidget *parent = nullptr);
	void init();
};

#endif // DOMEFRAME_H
