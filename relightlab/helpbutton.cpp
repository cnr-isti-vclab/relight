#include "helpbutton.h"

#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QWidget>
#include <QAction>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QSettings>

HelpDialog* HelpDialog::m_instance = nullptr; // Initialize static instance to nullptr


HelpedButton::HelpedButton(QAction *action, QWidget *parent): QWidget(parent) {
	init();
	id = action->objectName();
	setObjectName(id);
	setToolTip(id);

	button->setText(action->text());
	button->setIcon(action->icon());
	connect(button, SIGNAL(clicked(bool)), action, SLOT(trigger()));
}

HelpedButton::HelpedButton(QString id, QIcon icon, QString text, QWidget *parent): QWidget(parent) {
	init();
	setObjectName(id);
	setToolTip(id);

	button->setIcon(icon);
	button->setText(text);
}

void HelpedButton::init() {
	QHBoxLayout *layout = new QHBoxLayout(this);
	button = new QPushButton;
	button->setProperty("class", "large");


	help = new QToolButton;
	help->setIcon(QIcon::fromTheme("help-circle"));
	help->setProperty("class", "large");
	connect(help, SIGNAL(clicked()), this, SLOT(showHelp()));

	setStyleSheet("QToolButton { border: none; outline: none; }");
	layout->addWidget(button,1);
	layout->addWidget(help);
	layout->addStretch(1);
}


void HelpedButton::showHelp() {
	QString id = objectName();
	HelpDialog &dialog = HelpDialog::instance();
	dialog.showPage(id);
	//dialog.raise();
	//dialog.exec();
}


HelpButton::HelpButton(QString id, QWidget *parent): QToolButton(parent) {
	setIcon(QIcon::fromTheme("help-circle"));
	setObjectName(id);
	connect(this, SIGNAL(clicked()), this, SLOT(showHelp()));
	setToolTip(id);
	setStyleSheet("QToolButton { border: none; outline: none; }");
}

void HelpButton::showHelp() {
	QString id = objectName();
	HelpDialog &dialog = HelpDialog::instance();
	dialog.showPage(id);
	//dialog.raise();
	//dialog.exec();
}


HelpDialog::HelpDialog(QWidget *parent): QDialog(parent) {

	QSettings settings;
	QRect region = settings.value("help_position", QRect(100, 100, 600, 600)).toRect();
	setGeometry(region);

	QVBoxLayout *content = new QVBoxLayout(this);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

	browser = new QTextBrowser;
	content->addWidget(browser);
	content->addWidget(buttonBox);
}

HelpDialog& HelpDialog::instance() {
	if (!m_instance) {
		qDebug() << "recreate";
		m_instance = new HelpDialog(); // Create the instance if it doesn't exist
	}
	return *m_instance;
}

void HelpDialog::accept() {
	QSettings().setValue("help_position", geometry());
	QDialog::accept();
}

void HelpDialog::showPage(QString id) {
	QUrl url("qrc:/docs/interface/" + id + ".md");
#if QT_VERSION > QT_VERSION_CHECK(5, 15, 0)
	browser->setSource(url, QTextDocument::MarkdownResource);
#else
	browser->setSource(url);
#endif
	setGeometry(QSettings().value("help_position", QRect(100, 100, 600, 600)).toRect());
	show();
}

