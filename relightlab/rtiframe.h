#ifndef RTIFRAME_H
#define RTIFRAME_H

#include <QFrame>

class RtiFrame: public QFrame {
public:
	RtiFrame(QWidget *parent = nullptr);
	void init();
};

#endif // RTIFRAME_H
