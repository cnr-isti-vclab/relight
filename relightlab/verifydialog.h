#ifndef SPHEREVERIFY_H
#define SPHEREVERIFY_H

#include <QDialog>

class FlowLayout;
class Sphere;


class VerifyDialog: public QDialog {
public:
	VerifyDialog(Sphere *sphere, QWidget *parent = nullptr);
private:
	Sphere *sphere;
	FlowLayout *flowlayout = nullptr;
};

#endif // SPHEREVERIFY_H
