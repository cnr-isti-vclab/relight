#ifndef QLABELBUTTON_H
#define QLABELBUTTON_H

#include <QCommandLinkButton>

class QLabelButton: public QCommandLinkButton {
public:
	QLabelButton(QString text, QString description = "", QWidget *parent = nullptr): QCommandLinkButton(text, description, parent) {
		setCheckable(true);
		setIcon(QIcon());
		setMinimumWidth(200);
		setMinimumHeight(60);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	}
};

#endif // QLABELBUTTON_H
