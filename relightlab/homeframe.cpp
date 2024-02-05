#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QFile>
#include <QAction>

#include "homeframe.h"
#include "relightapp.h"
#include "recentprojects.h"

void setDefaultAction(QPushButton *button, QAction *action) {
	QObject::connect(button, SIGNAL(clicked(bool)), action, SLOT(trigger()));
	button->setText(action->text());
	button->setIcon(action->icon());
}

HomeFrame::HomeFrame() {
	setObjectName("home");

	//setStyleSheet(".home { background:red; padding-top:100px }");
	QHBoxLayout *contentLayout = new QHBoxLayout(this);
	contentLayout->addStretch(1);

	// Left column
	QVBoxLayout *leftColumnLayout = new QVBoxLayout();

	// Title label
	QLabel *titleLabel = new QLabel("<h1>RelightLab</h1>");
	titleLabel->setMinimumWidth(200);

	leftColumnLayout->addWidget(titleLabel);

	QPushButton *new_project = new QPushButton(this);
	new_project->setProperty("class", "large");
	setDefaultAction(new_project, qRelightApp->action("new_project"));
	leftColumnLayout->addWidget(new_project);

	QPushButton *open_project = new QPushButton(this);
	open_project->setProperty("class", "large");
	setDefaultAction(open_project, qRelightApp->action("open_project"));
	leftColumnLayout->addWidget(open_project);

	QLabel *recentLabel = new QLabel("<h2>Recent projects:</h2>");
	leftColumnLayout->addSpacing(20);
	leftColumnLayout->addWidget(recentLabel);


	for(QString filename: recentProjects()) {
		QLabel *label = new QLabel("<a href='" + filename + "'>" + filename + "</a>");
		label->setProperty("class", "recent");
		label->setWordWrap(true);
		leftColumnLayout->addWidget(label);
		connect(label, SIGNAL(linkActivated(QString)), qRelightApp, SLOT(openProject(QString)));
	}
	leftColumnLayout->addStretch();

	// Add columns to the content layout
	contentLayout->addLayout(leftColumnLayout, 2);

	// Right column
	QTextBrowser *browser = new QTextBrowser(this);
	browser->setStyleSheet("margin-left:40px; margin-top: 40px; background:transparent;");
	browser->setAlignment(Qt::AlignTop);
	QFile file(":/docs/home.txt");
	file.open(QIODevice::ReadOnly);
	browser->setText(file.readAll());
	browser->setMinimumWidth(400);
	contentLayout->addWidget(browser, 2);

	contentLayout->addStretch(1);

	// Set layout margins and spacing
	contentLayout->setContentsMargins(20, 20, 20, 20);
	contentLayout->setSpacing(20);

}
