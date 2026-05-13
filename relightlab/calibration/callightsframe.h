#ifndef CALLIGHTSFRAME_H
#define CALLIGHTSFRAME_H

#include <QFrame>

class CalibrationSession;
class QRadioButton;
class QStackedWidget;
class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QGroupBox;

class CalLightsFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalLightsFrame(QWidget *parent = nullptr);
	void setSession(CalibrationSession *session);

public slots:
	void sessionToUi();
	void uiToSession();
	void modelChanged();
	void browseLP();
	void loadLP();
	void exportLP();

private:
	CalibrationSession *session = nullptr;

	// light-position model
	QRadioButton   *radio_dir    = nullptr;  // Directional
	QRadioButton   *radio_sph    = nullptr;  // Spherical
	QRadioButton   *radio_3d     = nullptr;  // 3-D (positional LP)

	// light source (LP file vs. sphere)
	QRadioButton   *radio_lp     = nullptr;
	QRadioButton   *radio_sphere = nullptr;

	QStackedWidget *source_stack = nullptr;   // page 0: LP file, page 1: sphere

	// LP path (on source page 0)
	QLineEdit      *lp_file_path = nullptr;

	// sphere info (on source page 1)
	QLabel         *sphere_info  = nullptr;

	// geometry – shown conditionally below the source selector
	QGroupBox      *geom_box     = nullptr;
	QDoubleSpinBox *sb_radius    = nullptr;  // Spherical only
	QDoubleSpinBox *sb_offset    = nullptr;  // Spherical + 3-D
	QDoubleSpinBox *sb_width     = nullptr;  // Spherical + 3-D
};

#endif // CALLIGHTSFRAME_H
