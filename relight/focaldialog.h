#ifndef FOCALDIALOG_H
#define FOCALDIALOG_H

#include <QDialog>

namespace Ui {
class FocalDialog;
}

class Project;

class FocalDialog : public QDialog {
	Q_OBJECT

public:
	Project *project;

	explicit FocalDialog(Project *project, QWidget *parent = 0);
	~FocalDialog();
	virtual void accept();
public slots:
	void setAsReal();
	void setAsEquivalent();

private:
	Ui::FocalDialog *ui;
};

#endif // FOCALDIALOG_H
