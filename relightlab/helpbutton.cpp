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
#include <QCheckBox>
#include <QTextBrowser>
#include <QLabel>
#include <QSettings>
#include <QUrl>
#include <QFile>


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
	layout->setSizeConstraint(QHBoxLayout::SetMinimumSize);


	button = new QPushButton;
	//button->setProperty("class", "large");
	button->setMinimumSize(200, 40);
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	help = new HelpButton(id);

	layout->addWidget(button,1);
	layout->addWidget(help);
	layout->addStretch(1);
}


HelpButton::HelpButton(QString id, QWidget *parent): QToolButton(parent) {
	setCursor(Qt::PointingHandCursor);
	setIcon(QIcon::fromTheme("help-circle"));
	setId(id);
	//setStyleSheet("QToolButton { color: #ff00ff; }");
	connect(this, SIGNAL(clicked()), this, SLOT(showHelp()));

	setStyleSheet("QToolButton { border: none; outline: none; }");
}
void HelpButton::setId(QString id) {
	setObjectName(id);
	setToolTip(id);
}

void HelpButton::showHelp() {
	QString id = objectName();
	HelpDialog &dialog = HelpDialog::instance();
	dialog.showPage(id);
}

HelpLabel::HelpLabel(QString txt, QString help_id, QWidget *parent): QWidget(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	label = new QLabel(txt);
	layout->addWidget(label);
	layout->addStretch();
	help = new HelpButton(help_id);
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

HelpCheckBox::HelpCheckBox(QString txt, QString help_id, QWidget *parent): QWidget(parent) {
	QHBoxLayout *layout = new QHBoxLayout(this);
	checkBox = new QCheckBox(txt);
	layout->addWidget(checkBox);
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

	browser = new HelpBrowser;
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
	QUrl url(id + ".html");
#if QT_VERSION > QT_VERSION_CHECK(5, 15, 0)
	browser->setSource(url, QTextDocument::MarkdownResource);
#else
	browser->setSource(url);
#endif
	setGeometry(QSettings().value("help_position", QRect(100, 100, 600, 600)).toRect());
	show();
}

