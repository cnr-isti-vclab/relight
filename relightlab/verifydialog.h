#ifndef SPHEREVERIFY_H
#define SPHEREVERIFY_H

#include <QDialog>

class FlowLayout;
class Sphere;
class VerifyView;


class VerifyDialog: public QDialog {
	Q_OBJECT
public:
	enum Markers { REFLECTION, ALIGN };
	VerifyDialog(std::vector<QImage> &thumbs, std::vector<QPointF> &positions, Markers marker, QWidget *parent = nullptr);

public slots:
	void alignSamples();
	void resetAligns();
	void update();

private:
	FlowLayout *flowlayout = nullptr;
	std::vector<VerifyView *> views;
	std::vector<QImage> &thumbs;
	std::vector<QPointF> &positions;

};

#endif // SPHEREVERIFY_H
