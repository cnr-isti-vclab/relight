#ifndef CALLIGHTSFRAME_H
#define CALLIGHTSFRAME_H

#include <QFrame>

class QRadioButton;
class QStackedWidget;
class QLineEdit;
class QLabel;
class QDoubleSpinBox;

class CalLightsFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalLightsFrame(QWidget *parent = nullptr);

public slots:
	void sourceChanged();
	void browseLp();
	void loadLp();
	void exportLp();

private:
	QRadioButton   *radio_lp     = nullptr;
	QRadioButton   *radio_sphere = nullptr;

	QStackedWidget *lp_stack     = nullptr;

	// LP file page
	QLineEdit      *lp_file_path = nullptr;
	QDoubleSpinBox *sb_radius    = nullptr;
	QDoubleSpinBox *sb_offset    = nullptr;

	// sphere page
	QLabel         *sphere_info  = nullptr;
};

#endif // CALLIGHTSFRAME_H
