#include "progress.h"
#include "ui_progress.h"

Progress::Progress(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Progress)
{
	ui->setupUi(this);
}

Progress::~Progress()
{
	delete ui;
}
