#include "ballwidget.h"
#include "ui_ballwidget.h"

BallWidget::BallWidget(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::BallWidget)
{
	ui->setupUi(this);
	connect(ui->close_button, SIGNAL(clicked(bool)), this, SLOT(close()));
}

BallWidget::~BallWidget()
{
	delete ui;
}
