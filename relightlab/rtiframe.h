#ifndef RTIFRAME_H
#define RTIFRAME_H

#include <QFrame>
//TODO we should separate RTI definitions from actual implementation (materials etc).
#include "../src/rti.h"

class RtiCard;

class RtiFrame: public QFrame {
	Q_OBJECT
public:
	RtiFrame(QWidget *parent = nullptr);
	void init();

public slots:
	void setBasis(Rti::Type basis);

private:
	Rti::Type current_basis;
	Rti::ColorSpace current_colorspace;
	std::vector<RtiCard *> basis_cards;
};

#endif // RTIFRAME_H
