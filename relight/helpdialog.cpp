#include "helpdialog.h"
#include "ui_helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::HelpDialog)
{
	ui->setupUi(this);
	ui->textBrowser->setSource(QUrl("qrc:/docs/docs/help.html"));
}

HelpDialog::~HelpDialog()
{
	delete ui;
}
