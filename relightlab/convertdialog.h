#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>


/* This dialogs allows for conversion between .ptm and .rti to .relight and viceversa.
The dialog is divided in two sections for conversion from .ptm to .relight and from .rti to .relight.
*/

class ConvertDialog: public QDialog {
	Q_OBJECT
public:
	ConvertDialog();
	void convertToRelight();
	void convertToRti();
};

#endif // CONVERTDIALOG_H
