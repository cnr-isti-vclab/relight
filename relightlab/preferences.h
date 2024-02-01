#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>

class TabWidget;

class Preferences: public QDialog {
public:
	Preferences(QWidget *parent = nullptr);

private:
	TabWidget *tabs;
	QWidget *buildAppearance();
};

#endif // PREFERENCES_H
