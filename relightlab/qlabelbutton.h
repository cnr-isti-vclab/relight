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
		// Allow the button to expand vertically so wrapped text is fully visible
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
};

#endif // QLABELBUTTON_H
