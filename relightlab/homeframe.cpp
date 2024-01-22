#include "homeframe.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QFile>
#include "actions.h"

void setDefaultAction(QPushButton *button, QAction *action) {
	QObject::connect(button, SIGNAL(clicked(bool)), action, SLOT(trigger()));
	button->setText(action->text());
	button->setIcon(action->icon());
}

HomeFrame::HomeFrame() {
	setObjectName("home");

	//setStyleSheet(".home { background:red; padding-top:100px }");
	QHBoxLayout *contentLayout = new QHBoxLayout(this);
	contentLayout->addStretch();

	// Left column
	QVBoxLayout *leftColumnLayout = new QVBoxLayout();

	// Title label
	QLabel *titleLabel = new QLabel("RelightLab");
	titleLabel->setFont(QFont("Arial", 16, QFont::Bold));
	titleLabel->setMinimumWidth(200);

	leftColumnLayout->addWidget(titleLabel);

	QPushButton *new_project = new QPushButton(this);
	setDefaultAction(new_project, Action::new_project);
	leftColumnLayout->addWidget(new_project);

	QPushButton *open_project = new QPushButton(this);
	setDefaultAction(open_project, Action::open_project);
	leftColumnLayout->addWidget(open_project);

	QLabel *recentLabel = new QLabel("Recent projects:");
	leftColumnLayout->addWidget(recentLabel);
	leftColumnLayout->addStretch();

	// Add columns to the content layout
	contentLayout->addLayout(leftColumnLayout);

	// Right column
	QTextBrowser *browser = new QTextBrowser(this);
	browser->setStyleSheet("margin-left:40px; margin-top: 40px; background:transparent;");
	browser->setAlignment(Qt::AlignTop);
	QFile file(":/docs/home.txt");
	file.open(QIODevice::ReadOnly);
	browser->setText(file.readAll());
	browser->setMinimumWidth(400);
	contentLayout->addWidget(browser);

	contentLayout->addStretch();

	// Set layout margins and spacing
	contentLayout->setContentsMargins(20, 20, 20, 20);
	contentLayout->setSpacing(20);

}
