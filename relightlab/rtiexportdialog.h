#ifndef RTIEXPORTDIALOG_H
#define RTIEXPORTDIALOG_H

#include <QDialog>

class QSpinBox;

class RtiExportDialog: public QDialog {
public:
	RtiExportDialog(QWidget *parent = nullptr);
	QSpinBox *quality = nullptr;
};

class LegacyExportDialog: public QDialog {
public:
	LegacyExportDialog(QWidget *parent = nullptr);
	QSpinBox *quality = nullptr;
};

#endif // RTIEXPORTDIALOG_H
