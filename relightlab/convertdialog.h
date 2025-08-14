#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>


/* This dialogs allows for conversion between .ptm and .rti to .relight and viceversa.
The dialog is divided in two sections for conversion from .ptm to .relight and from .rti to .relight.
*/
class QLabelButton;
class QLineEdit;
class QPushButton;

class ConvertDialog: public QDialog {
	Q_OBJECT
public:
	ConvertDialog();
	void relightToRti(QString path);
	void relightToRelight(QString path);
	void rtiToRelight(QString path);

public slots:
	void selectInput();
	void verifyPath();
	void convert();
	

private:
	QLineEdit *input_path = nullptr;
	QLabelButton *rti = nullptr, *web = nullptr, *iip = nullptr;
	QLabelButton *img = nullptr, *deepzoom = nullptr, *tarzoom = nullptr, *itarzoom = nullptr;
	QPushButton *convert_button = nullptr;
};

#endif // CONVERTDIALOG_H
