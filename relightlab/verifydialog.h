#ifndef SPHEREVERIFY_H
#define SPHEREVERIFY_H

#include <QDialog>

class FlowLayout;
class Sphere;


class VerifyDialog: public QDialog {
public:
	VerifyDialog(std::vector<QImage> &thumbs, std::vector<QPointF> &positions, QWidget *parent = nullptr);

private:
	FlowLayout *flowlayout = nullptr;
	std::vector<QImage> &thumbs;
	std::vector<QPointF> &positions;

};

#endif // SPHEREVERIFY_H
