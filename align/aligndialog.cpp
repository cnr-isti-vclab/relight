#include "aligndialog.h"
#include "ui_aligndialog.h"

AlignDialog::AlignDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AlignDialog)
{
	ui->setupUi(this);
}

AlignDialog::~AlignDialog()
{
	delete ui;
}
