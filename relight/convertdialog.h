#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>

namespace Ui {
class ConvertDialog;
}

class ConvertDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConvertDialog(QWidget *parent = 0);
	~ConvertDialog();

public slots:
	void close() { QDialog::close(); }
	void convert();
	void selectInput();

private:
	Ui::ConvertDialog *ui;
};

#endif // CONVERTDIALOG_H
