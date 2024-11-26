#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QFile>
#include <QAction>

#include "homeframe.h"
#include "relightapp.h"
#include "recentprojects.h"
#include "helpbutton.h"

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
	leftColumnLayout->setSizeConstraint(QVBoxLayout::SetMinimumSize);

	// Title label
	QLabel *titleLabel = new QLabel("<h1>RelightLab</h1>");
	titleLabel->setMinimumWidth(200);

	leftColumnLayout->addWidget(titleLabel);

	HelpedButton *new_project = new HelpedButton(qRelightApp->action("new_project"), "interface/new_project");
	leftColumnLayout->addWidget(new_project);

	HelpedButton *open_project = new HelpedButton(qRelightApp->action("open_project"), "interface/open_project");
	leftColumnLayout->addWidget(open_project);

	QLabel *recentLabel = new QLabel("<h2>Recent projects:</h2>");
	leftColumnLayout->addSpacing(20);
	leftColumnLayout->addWidget(recentLabel);


	for(QString path: recentProjects()) {
		QFileInfo fileInfo(path);
		QString filename = fileInfo.fileName();
		QString directory = fileInfo.absolutePath();

		QLabel *label = new QLabel("<p style='line-height:150%'><a href='" + path + "'>" + filename + "</a><br/><span style='color:grey; font-size:80%;'>" + directory + "</span></p>");
		label->setProperty("class", "recent");
		label->setWordWrap(true);
		leftColumnLayout->addWidget(label);
		connect(label, SIGNAL(linkActivated(QString)), qRelightApp, SLOT(openProject(QString)));
	}
	leftColumnLayout->addStretch(1);

	// Add columns to the content layout
	contentLayout->addLayout(leftColumnLayout, 2);

	// Right column
	QTextBrowser *browser = new QTextBrowser(this);
	browser->setStyleSheet("margin-left:40px; margin-top: 40px; background:transparent;");
	browser->setAlignment(Qt::AlignTop);
#if QT_VERSION > QT_VERSION_CHECK(5, 15, 0)
	browser->setSource(QUrl("qrc:/docs/home.md"), QTextDocument::MarkdownResource);
#else
	browser->setSource(QUrl("qrc:/docs/home.md"));
#endif
	browser->setMinimumWidth(400);
	contentLayout->addWidget(browser, 2);

	contentLayout->addStretch(1);

	// Set layout margins and spacing
	contentLayout->setContentsMargins(20, 20, 20, 20);
	contentLayout->setSpacing(20);

}
