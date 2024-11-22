#ifndef QLABELBUTTON_H
#define QLABELBUTTON_H

#include <QCommandLinkButton>
#include <QLabel>
#include <QDebug>

class QLabelButton: public QCommandLinkButton {
public:
	QLabelButton(QString text, QString description = "", QWidget *parent = nullptr): QCommandLinkButton(text, description, parent) {
		setCheckable(true);
		setIcon(QIcon());
		setMinimumWidth(200);
	}
};

#endif // QLABELBUTTON_H
