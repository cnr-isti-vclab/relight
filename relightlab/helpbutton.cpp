#include "helpbutton.h"

#include <QWidget>
#include <QAction>

#include <QDialogButtonBox>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QToolButton>
#include <QRadioButton>
#include <QTextBrowser>
#include <QLabel>
#include <QSettings>
#include <QUrl>
#include <QFile>

#include <QDebug>


HelpDialog* HelpDialog::m_instance = nullptr; // Initialize static instance to nullptr


HelpedButton::HelpedButton(QAction *action, QString url, QWidget *parent): QWidget(parent) {
	init(url);
	button->setText(action->text());
	button->setIcon(action->icon());
	connect(button, SIGNAL(clicked(bool)), action, SLOT(trigger()));
}

HelpedButton::HelpedButton(QString id, QIcon icon, QString text, QWidget *parent): QWidget(parent) {
	init(id);
	setToolTip(id);

	button->setIcon(icon);
	button->setText(text);
}

void HelpedButton::init(QString id) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	button = new QPushButton;
	button->setProperty("class", "large");

	help = new HelpButton(id);

	layout->addWidget(button,1);
	layout->addWidget(help);
	layout->addStretch(1);
}


HelpButton::HelpButton(QString id, QWidget *parent): QToolButton(parent) {
	setCursor(Qt::PointingHandCursor);
	setIcon(QIcon::fromTheme("help-circle"));
	//setStyleSheet("QToolButton { color: #ff00ff; }");
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

HelpLabel::HelpLabel(QString txt, QString help_id, QWidget *parent): QWidget(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	QLabel *label = new QLabel(txt);
	layout->addWidget(label);
	layout->addStretch();
	HelpButton *help = new HelpButton(help_id);
	layout->addWidget(help);
}

HelpRadio::HelpRadio(QString txt, QString help_id, QWidget *parent): QWidget(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	radio = new QRadioButton(txt);
	layout->addWidget(radio);
	layout->addStretch();
	HelpButton *help = new HelpButton(help_id);
	layout->addWidget(help);
	layout->setContentsMargins(0, 0, 0, 0);
}
		
HelpDialog::HelpDialog(QWidget *parent): QDialog(parent) {

	QSettings settings;
	QRect region = settings.value("help_position", QRect(100, 100, 600, 600)).toRect();
	setGeometry(region);

	QVBoxLayout *content = new QVBoxLayout(this);

	QToolBar *toolbar = new QToolBar;
	toolbar->addAction(QIcon::fromTheme("home"), "Home", this, SLOT(home()));
	toolbar->addSeparator();
	toolbar->addAction(QIcon::fromTheme("chevron-left"), "Back", this, SLOT(backward()));
	toolbar->addAction(QIcon::fromTheme("chevron-right"), "Forward", this, SLOT(forward()));

	content->addWidget(toolbar);

	browser = new QTextBrowser;
	content->addWidget(browser);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

	content->addWidget(buttonBox);
}

HelpDialog& HelpDialog::instance() {
	if (!m_instance) {
		m_instance = new HelpDialog(); // Create the instance if it doesn't exist
	}
	return *m_instance;
}

void HelpDialog::accept() {
	QSettings().setValue("help_position", geometry());
	QDialog::accept();
}

void HelpDialog::home() {
	showPage("index");
}

void HelpDialog::forward() {
	browser->forward();
}

void HelpDialog::backward() {
	browser->backward();
}

void HelpDialog::showPage(QString id) {
	QUrl url("qrc:/docs/" + id + ".md");
#if QT_VERSION > QT_VERSION_CHECK(5, 15, 0)
	browser->setSource(url, QTextDocument::MarkdownResource);
#else
	browser->setSource(url);
#endif
	setGeometry(QSettings().value("help_position", QRect(100, 100, 600, 600)).toRect());
	show();
}

