#ifndef HELPBUTTON_H
#define HELPBUTTON_H

#include <QToolButton>
#include <QDialog>
#include "homeframe.h"

class QPushButton;
class QIcon;
class QAction;
class QRadioButton;
class QCheckBox;

class HelpedButton: public QWidget {
	Q_OBJECT
public:
	HelpedButton(QAction *action, QString url, QWidget *parent = nullptr);
	HelpedButton(QString id, QIcon icon, QString text, QWidget *parent = nullptr);
	//void setDefaultAction(QAction &a);
signals:
	void clicked();

private:
	QPushButton *button = nullptr;
	QToolButton *help = nullptr;

	void init(QString id);
};

class HelpButton: public QToolButton {
	Q_OBJECT
public:
	HelpButton(QString id, QWidget *parent = nullptr);
	void setId(QString id);

public slots:
	void showHelp();
};

class QLabel;

class HelpLabel: public QWidget {
	public:
		HelpLabel(QString txt, QString help_id, QWidget *parent = nullptr);
		QLabel *label = nullptr;
		HelpButton *help = nullptr;
};

class HelpRadio: public QWidget {
	public:
		HelpRadio(QString txt, QString help_id, QWidget *parent = nullptr);
		QRadioButton *radioButton() { return radio; }
	private:
		QRadioButton *radio = nullptr;
};

class HelpCheckBox: public QWidget {
	public:
		HelpCheckBox(QString txt, QString help_id, QWidget *parent = nullptr);
		QCheckBox *checkBoxButton() { return checkBox; }
	private:
		QCheckBox *checkBox = nullptr;
};


class QTextBrowser;

class HelpDialog: public QDialog {
	Q_OBJECT
public:
	void showPage(QString id);
	static HelpDialog& instance(); // Static method to get the instance

public slots:
	void accept();
	void home(); //get to initial documentation page
	void forward();
	void backward();
private:
	HelpBrowser *browser = nullptr;

	explicit HelpDialog(QWidget *parent = nullptr);
	HelpDialog(const HelpDialog&) = delete; // Disable copy constructor
	HelpDialog& operator=(const HelpDialog&) = delete; // Disable assignment operator

	static HelpDialog* m_instance; // Static instance of Dialog
};
#endif // HELPBUTTON_H
