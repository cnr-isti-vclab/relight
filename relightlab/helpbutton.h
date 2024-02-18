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
	HelpedButton(QAction *action, QString url, QWidget *parent = nullptr);
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
	QTextBrowser *browser = nullptr;

	explicit HelpDialog(QWidget *parent = nullptr);
	HelpDialog(const HelpDialog&) = delete; // Disable copy constructor
	HelpDialog& operator=(const HelpDialog&) = delete; // Disable assignment operator

	static HelpDialog* m_instance; // Static instance of Dialog
};
#endif // HELPBUTTON_H
