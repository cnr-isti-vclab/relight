#ifndef RTIFRAME_H
#define RTIFRAME_H

#include <QFrame>

class QPushButton;

class RtiFrame: public QFrame {
public:
	enum Basis { PTM = 0, HSH = 1, RBF = 2, BNL = 3, NEURAL = 4 };
	enum Colorspace { RGB = 0, LRGB = 1, MRGB = 2, YCC = 3};

	RtiFrame(QWidget *parent = nullptr);
	void init();

private:
	QPushButton *basis[5];
	QPushButton *spaces[4];
};

#endif // RTIFRAME_H
