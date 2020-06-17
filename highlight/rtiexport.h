#ifndef RTIEXPORT_H
#define RTIEXPORT_H

#include <QDialog>

namespace Ui {
class RtiExport;
}

class RtiExport : public QDialog
{
	Q_OBJECT

public:
	explicit RtiExport(QWidget *parent = 0);
	~RtiExport();

public slots:
	void changeBasis(int n);
	void changePlanes(int n);
private:
	Ui::RtiExport *ui;
};

#endif // RTIEXPORT_H
