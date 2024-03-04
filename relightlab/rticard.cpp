#include "rticard.h"
#include "helpbutton.h"

#include <QVBoxLayout>
#include <QLabel>

RtiCard::RtiCard(QString title, QString text, QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(title));
	//QImage thumb = qRelightApp->thumbnails()[0];
	QLabel *img = new QLabel();
	img->setMinimumSize(200, 200);
	layout->addWidget(img);
	//img->setPixmap(QPixmap::fromImage(thumb.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	layout->addWidget(new HelpLabel(text, title));
	setFrameStyle(QFrame::StyledPanel);
}

void RtiCard::mousePressEvent(QMouseEvent *event) {
	selected = !selected;
	setFrameStyle(selected ? QFrame::Panel : QFrame::StyledPanel);
	QFrame::mousePressEvent(event);
}
