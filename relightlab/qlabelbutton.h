#ifndef QLABELBUTTON_H
#define QLABELBUTTON_H

#include <QPushButton>
#include <QLabel>

class QLabelButton: public QPushButton {
public:
	QLabelButton(QString text, QWidget *parent = nullptr): QPushButton(parent) {
		init(text, QRect(10, 10, 130, 80));
	}
	QLabelButton(QString text, QRect rect, QWidget *parent = nullptr): QPushButton(parent) {
		init(text, rect);
	}
	void init(QString text, QRect rect) {
		setGeometry(rect);
		setFixedSize(150, 100);
		label = new QLabel(this);
		label->setGeometry(rect);
		label->setText(text);
	}
private:
	QLabel *label = nullptr;
};

#endif // QLABELBUTTON_H
