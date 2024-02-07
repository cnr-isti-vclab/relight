#include "lppanel.h"
#include "relightapp.h"
#include "../src/lp.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QFileDialog>
#include <QTextBrowser>


LpPanel::LpPanel() {
	QVBoxLayout *content = new QVBoxLayout(this);

	QLabel *title = new QLabel("<h2>Load .lp file</h2>");
	content->addWidget(title);
	content->addSpacing(30);

	QFrame *lp_file = new QFrame;
	content->addWidget(lp_file);

	QHBoxLayout *lp_layout = new QHBoxLayout(lp_file);

	QPushButton *lp_load = new QPushButton(QIcon::fromTheme("folder"), "Load LP file...");
	lp_load->setProperty("class", "large");
	lp_load->setMinimumWidth(200);
	lp_load->setMaximumWidth(300);
	connect(lp_load, SIGNAL(clicked()), this, SLOT(loadLP()));

	lp_layout->addWidget(lp_load);

	lp_filename = new QLineEdit();
	lp_filename->setEnabled(false);
	lp_layout->addWidget(lp_filename);
}



void LpPanel::loadLP() {
	QString lp = QFileDialog::getOpenFileName(this, "Load an LP file", QString(), "Light directions (*.lp)");
	if(lp.isNull())
		return;
	std::vector<QString> filenames;

	Dome dome;
	dome.lightConfiguration = Dome::DIRECTIONAL;

	try {
		parseLP(lp, dome.directions, filenames);
	} catch(QString error) {
		QMessageBox::critical(this, "Loading .lp file failed", error);
		return;
	}

	emit accept(dome);
}

