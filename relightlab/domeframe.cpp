#include "domeframe.h"

#include <QVBoxLayout>
#include <QLabel>

DomeFrame::DomeFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *content = new QVBoxLayout(this);
	content->setContentsMargins(31, 31, 31, 31);
	this->setLayout(content);

	QLabel *title = new QLabel("<h2>Dome light directions</h2>");
	content->addWidget(title);
	content->addSpacing(30);




	QHBoxLayout *columns = new QHBoxLayout();
	content->addLayout(columns);


/*	lights = new QGraphicsView(&scene);
	lights->setBackgroundBrush(Qt::black);
	lights->setMinimumSize(300, 300);
	lights->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(lights);
	columns->setAlignment(lights, Qt::AlignTop);


	properties = new PropertyBox(QStringList() << "Filename:" << "Lights:");
	columns->addWidget(properties);
	columns->addStretch();

	content->addLayout(columns);

	content->addStretch();

	QPushButton *cancel = new QPushButton(QIcon::fromTheme("cancel"), "Cancel");
	cancel->setProperty("class", "large");
	content->addWidget(cancel);

	connect(cancel, SIGNAL(clicked()), this->parent(), SLOT(showChoice()));

	connect(load, SIGNAL(clicked()), this, SLOT(loadLP())); */

}

void DomeFrame::init() {
}
