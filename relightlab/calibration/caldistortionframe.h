#ifndef CALDISTORTIONFRAME_H
#define CALDISTORTIONFRAME_H

#include <QFrame>

class QRadioButton;
class QStackedWidget;
class QLineEdit;
class QComboBox;
class QDoubleSpinBox;

class CalDistortionFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalDistortionFrame(QWidget *parent = nullptr);

public slots:
	void sourceChanged();
	void browseFile();
	void loadFile();
	void saveFile();

private:
	QRadioButton   *radio_none    = nullptr;
	QRadioButton   *radio_file    = nullptr;
	QRadioButton   *radio_lensfun = nullptr;
	QRadioButton   *radio_manual  = nullptr;

	QStackedWidget *param_stack   = nullptr;

	// file page
	QLineEdit      *lens_file_path = nullptr;

	// lensfun page
	QComboBox      *lf_maker      = nullptr;
	QComboBox      *lf_model      = nullptr;

	// manual page
	QDoubleSpinBox *sb_k1 = nullptr;
	QDoubleSpinBox *sb_k2 = nullptr;
	QDoubleSpinBox *sb_k3 = nullptr;
	QDoubleSpinBox *sb_p1 = nullptr;
	QDoubleSpinBox *sb_p2 = nullptr;
};

#endif // CALDISTORTIONFRAME_H
