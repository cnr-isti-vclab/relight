#ifndef DOMECALIBRATION_H
#define DOMECALIBRATION_H

#include <QDialog>

#include "../src/dome.h"

namespace Ui {
class DomeCalibration;
}

class DomeCalibration : public QDialog
{
	Q_OBJECT

public:
	Dome dome;
	explicit DomeCalibration(QWidget *parent, Dome &_dome);
	~DomeCalibration();

public slots:
	void loadConfig();
	void saveConfig();
	void update(); //set enabled and disabled depending on lightConfiguration

private:
	Ui::DomeCalibration *ui;
};

#endif // DOMECALIBRATION_H
