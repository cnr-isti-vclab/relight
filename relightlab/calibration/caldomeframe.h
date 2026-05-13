#ifndef CALDOMEFRAME_H
#define CALDOMEFRAME_H

#include <QFrame>

class CalibrationSession;
class QLineEdit;
class QTextEdit;

class CalDomeFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalDomeFrame(QWidget *parent = nullptr);
	void setSession(CalibrationSession *session);

public slots:
	void sessionToUi();
	void uiToSession();

private:
	CalibrationSession *session = nullptr;

	QLineEdit      *label_edit  = nullptr;
	QTextEdit      *notes_edit  = nullptr;
};

#endif // CALDOMEFRAME_H
