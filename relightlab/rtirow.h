#ifndef RTIROW_H
#define RTIROW_H

#include <QFrame>

class QHBoxLayout;

class RtiRow: public QFrame {
public:
	RtiRow(QWidget *parent = nullptr);

protected:
	QHBoxLayout *row = nullptr;
};

class PtmRow: public RtiRow {
public:
	PtmRow(QWidget *parent = nullptr);

};

class HshRow: public RtiRow {
public:
	HshRow(QWidget *parent = nullptr);

};

class RbfRow: public RtiRow {
public:
	RbfRow(QWidget *parent = nullptr);
};
#endif // RTIROW_H
