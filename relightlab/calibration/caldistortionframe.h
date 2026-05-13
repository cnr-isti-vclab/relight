#ifndef CALDISTORTIONFRAME_H
#define CALDISTORTIONFRAME_H

#include <QFrame>
#include <QPixmap>

class QRadioButton;
class QStackedWidget;
class QLineEdit;
class QLabel;
class QSpinBox;

class CalDistortionFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalDistortionFrame(QWidget *parent = nullptr);

public slots:
	void browseLensFile();
	void loadLensFile();
	void saveLensFile();
	void browseSamplePhoto();
	void browseGridPhoto();
	void runGridCalibration();

private:
	void detectFromExif();
	void updatePreview(const QString &path, QLabel *target);
	static bool isRawPath(const QString &path);
	static QPixmap loadPreviewPixmap(const QString &path, int maxDim = 480);

	QRadioButton   *radio_none    = nullptr;
	QRadioButton   *radio_file    = nullptr;
	QRadioButton   *radio_sample  = nullptr;
	QRadioButton   *radio_grid    = nullptr;

	QStackedWidget *param_stack   = nullptr;

	// file page
	QLineEdit      *lens_file_path  = nullptr;

	// sample-photo page
	QLineEdit      *sample_path     = nullptr;
	QLabel         *sample_preview  = nullptr;
	QLabel         *exif_result     = nullptr;

	// grid-photo page
	QLineEdit      *grid_path       = nullptr;
	QLabel         *grid_preview    = nullptr;
	QSpinBox       *grid_cols       = nullptr;  // inner corners horizontally
	QSpinBox       *grid_rows       = nullptr;  // inner corners vertically
	QLabel         *grid_result     = nullptr;
};

#endif // CALDISTORTIONFRAME_H
