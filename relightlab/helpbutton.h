#ifndef HELPBUTTON_H
#define HELPBUTTON_H

#include <QToolButton>
#include <QDialog>

class QPushButton;
class QIcon;
class QAction;

class HelpedButton: public QWidget {
	Q_OBJECT
public:
	HelpedButton(QAction *action, QWidget *parent = nullptr);
	HelpedButton(QString id, QIcon icon, QString text, QWidget *parent = nullptr);
	void setDefaultAction(QAction &a);
signals:
	void clicked();
public slots:
	void showHelp();
private:
	QPushButton *button = nullptr;
	QToolButton *help = nullptr;
	QString id;

	void init();
};

class HelpButton: public QToolButton {
	Q_OBJECT
public:
	HelpButton(QString id, QWidget *parent = nullptr);

public slots:
	void showHelp();
};


class QTextBrowser;

class HelpDialog: public QDialog {
public:
	HelpDialog(QWidget *parent = nullptr);
	void showPage(QString id);
private:
	QTextBrowser *browser = nullptr;
};
#endif // HELPBUTTON_H
