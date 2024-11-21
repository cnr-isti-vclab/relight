#ifndef NORMALSFRAME_H
#define NORMALSFRAME_H

#include <QFrame>

class QCheckBox;
class QRadioButton;
class QSpinBox;
class QDoubleSpinBox;

class NormalsFrame: public QFrame {
	Q_OBJECT
public:
	NormalsFrame(QWidget *parent = nullptr);

public slots:
	void save();

signals:
	void processStarted();

private:
	QRadioButton *jpg = nullptr;
	QRadioButton *png = nullptr;
	QCheckBox *tif = nullptr;
	QCheckBox *ply = nullptr;
	QDoubleSpinBox *discontinuity = nullptr;
	QCheckBox *radial = nullptr;
	QCheckBox *fourier = nullptr;
	QSpinBox *fourier_radius = nullptr;
};

#endif // NORMALSFRAME_H
