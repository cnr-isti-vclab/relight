#ifndef NORMALSFRAME_H
#define NORMALSFRAME_H

#include <QFrame>

class QCheckBox;
class QRadioButton;

class NormalsFrame: public QFrame {
	Q_OBJECT
public:
	NormalsFrame(QWidget *parent = nullptr);

public slots:
	void save();

private:
	QRadioButton *jpg = nullptr;
	QRadioButton *png = nullptr;
	QCheckBox *tif = nullptr;
	QCheckBox *ply = nullptr;
};

#endif // NORMALSFRAME_H
