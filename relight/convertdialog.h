#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>
class Project;

namespace Ui {
class ConvertDialog;
}

class ConvertDialog : public QDialog
{
	Q_OBJECT

public:
	Project *project = nullptr;
	explicit ConvertDialog(QWidget *parent = 0);
	~ConvertDialog();
	void exec(Project *_project) {
		project = _project;
		QDialog::exec();
	}

public slots:
	void close() { project = nullptr; QDialog::close(); }
	void convert();
	void selectInput();

private:
	Ui::ConvertDialog *ui;
};

#endif // CONVERTDIALOG_H
