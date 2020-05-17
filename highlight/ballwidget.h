#ifndef BALLWIDGET_H
#define BALLWIDGET_H

#include <QDialog>

namespace Ui {
class BallWidget;
}

class BallWidget : public QDialog
{
	Q_OBJECT

public:
	explicit BallWidget(QWidget *parent = 0);
	~BallWidget();

private:
	Ui::BallWidget *ui;
};

#endif // BALLWIDGET_H
